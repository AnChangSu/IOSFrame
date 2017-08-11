/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sqliterk_btree.h"
#include "SQLiteRepairKit.h"
#include "sqliterk_column.h"
#include "sqliterk_os.h"
#include "sqliterk_pager.h"
#include "sqliterk_util.h"
#include "sqliterk_values.h"
#include <string.h>

/*
  Btree页头格式
 
 偏移量  大小  说明
 
 0       1    Flags. 1: intkey, 2: zerodata, 4: leafdata, 8: leaf
 
 1       2    byte offset to the first freeblock
 
 3       2    number of cells on this page
 
 5       2    first byte of the cell content area
 
 7       1    number of fragmented free bytes
 
 8       4    Right child (the Ptr(N) value).  Omitted on leaves.
*/

// Declarations
//btree解析页
static int sqliterkBtreeParsePage(sqliterk_btree *btree, int pageno);
//btree解析数条
static int sqliterkBtreeParseCell(sqliterk_btree *btree,
                                  sqliterk_page *page,
                                  const int *cellPointerArray,
                                  const int cellsCount);
//btree解析指定payload
static int sqliterkBtreeParsePayload(sqliterk_btree *btree,
                                     sqliterk_page *page,
                                     int offset,
                                     int payloadSize,
                                     sqliterk_column *column);
//返回btree串行长度
static int sqliterkBtreeGetLengthForSerialType(int serialType);

//b-tree结构体
//Btree页结构
/*
文件头（只有page 1有）

页头

单元指针数组

未分配空间

单元内容区
 */
struct sqliterk_btree {
    //数据库结构体
    sqliterk *rk;
    //名字
    char *name;
    //btree类型
    sqliterk_btree_type type;
    //数据库页管理者
    sqliterk_pager *pager;
    //数据库根页
    sqliterk_page *rootpage;
    // For leaf-table. See https://www.sqlite.org/fileformat2.html#btree
    int maxLocal;
    int minLocal;
    //叶节点最大容量
    int maxLeaf;
    int minLeaf;

    //解析相关的通知
    sqliterk_btree_notify notify;
    //用户信息
    void *userInfo;
};

//传入数据库rk，页管理者pager，根页编码，返回btree
//打开btree
int sqliterkBtreeOpen(sqliterk *rk,
                      sqliterk_pager *pager,
                      int rootPageno,
                      sqliterk_btree **btree)
{
    //pager==nil || btree == nil则返回
    //为什么btree不能为空?
    if (!pager || !btree) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;
    //为btree申请内存
    sqliterk_btree *theBtree = sqliterkOSMalloc(sizeof(sqliterk_btree));
    if (!theBtree) {
        rc = SQLITERK_NOMEM;
        goto sqliterkBtreeOpen_Failed;
    }\
    //设置btree的页管理者pager
    theBtree->pager = pager;

    //从文件获取页的全部数据，并初始化页
    rc = sqliterkPageAcquire(theBtree->pager, rootPageno, &theBtree->rootpage);
    if (rc != SQLITERK_OK) {
        goto sqliterkBtreeOpen_Failed;
    }
    if (rootPageno == 1) {
        //表sqlite_master的根页为page 1.设置master页
        rc = sqliterkBtreeSetMeta(theBtree, "sqlite_master",
                                  sqliterk_btree_type_master);
        if (rc != SQLITERK_OK) {
            goto sqliterkBtreeOpen_Failed;
        }
    } else {
        //根据页面的类型，设置btree类型
        switch (sqliterkPageGetType(theBtree->rootpage)) {
            case sqliterk_page_type_interior_index:
            case sqliterk_page_type_leaf_index:
                theBtree->type = sqliterk_btree_type_index;
                break;
            case sqliterk_page_type_interior_table:
            case sqliterk_page_type_leaf_table:
                theBtree->type = sqliterk_btree_type_table;
                break;
            default:
                rc = SQLITERK_DAMAGED;
                goto sqliterkBtreeOpen_Failed;
        }
    }
    // Save memory
    //获取到btree类型后，释放根页
    sqliterkPageClearData(theBtree->rootpage);

    //？？？？？？？
    theBtree->maxLocal =
        (sqliterkPagerGetUsableSize(theBtree->pager) - 12) * 64 / 255 - 23;
    theBtree->minLocal =
        (sqliterkPagerGetUsableSize(theBtree->pager) - 12) * 32 / 255 - 23;
    theBtree->maxLeaf = sqliterkPagerGetUsableSize(theBtree->pager) - 35;
    theBtree->minLeaf =
        (sqliterkPagerGetUsableSize(theBtree->pager) - 12) * 32 / 255 - 23;

    //指定btree的数据库
    theBtree->rk = rk;

    *btree = theBtree;
    return SQLITERK_OK;

sqliterkBtreeOpen_Failed:
    if (theBtree) {
        sqliterkBtreeClose(theBtree);
    }
    *btree = NULL;
    return rc;
}

//解析btree
int sqliterkBtreeParse(sqliterk_btree *btree)
{
    if (!btree) {
        return SQLITERK_MISUSE;
    }
    //如果有通知，调用开始解析通知
    if (btree->notify.onBeginParseBtree) {
        btree->notify.onBeginParseBtree(btree->rk, btree);
    }
    //解析btree
    int rc =
        sqliterkBtreeParsePage(btree, sqliterkPageGetPageno(btree->rootpage));
    //如果有通知，调用结束解析通知
    if (btree->notify.onEndParseBtree) {
        btree->notify.onEndParseBtree(btree->rk, btree, rc);
    }
    return rc;
}

// If the page is an interior-btree, no matter is an interior-table btree
// or an interior-index btree, this function will recursively parse the page
// until it find the leaf page or any error occur.
// A leaf-index btree will only be found but not parse, since its data make
// no sense.
//如果是内部btree的页，不用关心是内部表btree或是内部索引btree，这个函数会递归解析页，直到
//找到最终叶子节点或发生错误。
//一个叶索引btree可能只被找到但不被解析，因为它的数据没有意义
//解析页是一个递归过程，基于二叉树，依次找到叶节点，解析叶节点数据。然后返回叶节点的父节点，解析数据...依次类推。
static int sqliterkBtreeParsePage(sqliterk_btree *btree, int pageno)
{
    int i;

    //btree为空，或传入的页码大于总页数。则返回错误
    if (!btree || pageno > sqliterkPagerGetPageCount(btree->pager)) {
        return SQLITERK_MISUSE;
    }

    int rc = SQLITERK_OK;

    //如果有页解析通知，则调用页解析通知
    if (btree->notify.onBeginParsePage) {
        int rc = btree->notify.onBeginParsePage(btree->rk, btree, pageno);
        if (rc != SQLITERK_OK) {
            return rc;
        }
    }

    int *cellPointerArray = NULL;
    sqliterk_page *page = NULL;

    // ahead checking type to fast up parsing
    //从文件获取页的全部数据，并初始化页
    rc = sqliterkPageAcquire(btree->pager, pageno, &page);
    if (rc != SQLITERK_OK) {
        goto sqliterkBtreeParsePage_End;
    }
    //获取页的类型信息
    sqliterk_page_type type = sqliterkPageGetType(page);
    //如果是如下类型，则跳到解析结束
    if (type != sqliterk_page_type_interior_index &&
        type != sqliterk_page_type_interior_table &&
        type != sqliterk_page_type_leaf_index &&
        type != sqliterk_page_type_leaf_table) {
        rc = sqliterkOSWarning(SQLITERK_DAMAGED, "Page %d has invalid type",
                               pageno);
        goto sqliterkBtreeParsePage_End;
    }

    //sqliterkOSDebug(SQLITERK_OK, "Page %d is %s", pageno, sqliterkPageGetTypeName(type));

    // Parse cell pointer array. For further information, see [cell pointer]
    // at https://www.sqlite.org/fileformat2.html#btree
    //获取页中的数据
    const unsigned char *pagedata = sqliterkPageGetData(page);
    //条数偏移指针队列。如果是内部表类型，数值为12，不是则为8
    int offsetCellPointerArray =
        (type == sqliterk_page_type_interior_table) ? 12 : 8;
    //读取data中offset偏移，length长度的int。
    int cellsCount;
    //参照最上面的btree页头格式，3为行数。读取行数信息
    sqliterkParseInt(pagedata, 3 + sqliterkPageHeaderOffset(page), 2,
                     &cellsCount);
    if (cellsCount <= 0 || cellsCount * 2 + offsetCellPointerArray >
                               sqliterkPagerGetSize(btree->pager)) {
        rc = SQLITERK_DAMAGED;
        goto sqliterkBtreeParsePage_End;
    }
    //给条数指针数组分配内存
    cellPointerArray = sqliterkOSMalloc(sizeof(int) * (cellsCount + 1));
    if (!cellPointerArray) {
        rc = SQLITERK_NOMEM;
        goto sqliterkBtreeParsePage_End;
    }

    //初始化条数指针数组
    for (i = 0; i < cellsCount; i++) {
        int cellPointer;
        //读取data中offset偏移，length长度的int
        sqliterkParseInt(pagedata,
                         sqliterkPageHeaderOffset(page) +
                             offsetCellPointerArray + i * 2,
                         2, &cellPointer);
        cellPointerArray[i] = cellPointer;
    }

    switch (type) {
            //如果页的类型为内部表或内部索引
        case sqliterk_page_type_interior_table:
        case sqliterk_page_type_interior_index: {
            //????像是内部表多一个页
            int hasRightMostPageno =
                (type == sqliterk_page_type_interior_table);
            //???条数？
            int pagenosCount = cellsCount + hasRightMostPageno;
            //页码信息数组
            int *pagenos = sqliterkOSMalloc(sizeof(int) * (pagenosCount + 1));
            if (!pagenos) {
                rc = SQLITERK_NOMEM;
                goto sqliterkBtreeParsePage_End;
            }
            //初始化页码信息pagenos
            for (i = 0; i < cellsCount; i++) {
                sqliterkParseInt(pagedata, cellPointerArray[i], 4, pagenos + i);
            }
            //如果是sqliterk_page_type_interior_table类型，多读取一页
            if (hasRightMostPageno) {
                sqliterkParseInt(pagedata, 8, 4, pagenos + cellsCount);
            }
            // All done for page data. Ahead release the page data to avoid memory overflow
            //页信息读取完，释放条指针队列
            sqliterkOSFree(cellPointerArray);
            cellPointerArray = NULL;
            //释放数据
            sqliterkPageClearData(page);
            // Recursively decode the page
            //递归解析页
            for (i = 0; i < pagenosCount; i++) {
                sqliterkBtreeParsePage(btree, pagenos[i]);
            }
            //释放数组
            sqliterkOSFree(pagenos);
            break;
        }
            //如果是页节点表
        case sqliterk_page_type_leaf_table:
            //如果btree的类型是系统表，并且不是master表，跳转到解析结束
            if (sqliterkBtreeIsSystemType(sqliterkBtreeGetType(btree)) &&
                btree->type != sqliterk_btree_type_master) {
                //skip a non-master system table, since its column is generated.
                goto sqliterkBtreeParsePage_End;
            }
            //解析条数据
            rc = sqliterkBtreeParseCell(btree, page, cellPointerArray,
                                        cellsCount);
            break;
        case sqliterk_page_type_leaf_index:
            // Just skip it since the column in leaf index make no sense.
            break;
        default:
            break;
    }

sqliterkBtreeParsePage_End:
    //释放条指针队列
    if (cellPointerArray) {
        sqliterkOSFree(cellPointerArray);
    }
    //如果有解析结束通知，则调用解析结束通知
    if (btree->notify.onEndParsePage) {
        btree->notify.onEndParsePage(btree->rk, btree, pageno, rc);
    }
    //释放页信息
    if (page) {
        sqliterkPageRelease(page);
    }
    if (rc != SQLITERK_OK) {
        sqliterkOSDebug(rc, "Failed to parse page %d.", pageno, rc);
    }
    return rc;
}

// Parse the payload data. see [B-tree Cell Format]
// at https://www.sqlite.org/fileformat2.html#btree
//解析Payload信息
//Btree页内部以单元为单位来组织数据，一个单元包含一个(或部分，当使用溢出页时)payload(也称为Btree记录)。
static int sqliterkBtreeParseCell(sqliterk_btree *btree,
                                  sqliterk_page *page,
                                  const int *cellPointerArray,
                                  const int cellsCount)
{
    if (!btree || !page || !cellPointerArray || cellsCount < 0) {
        return SQLITERK_MISUSE;
    }
    //获取页中的数据
    const unsigned char *pagedata = sqliterkPageGetData(page);
    int rc = SQLITERK_OK;
    sqliterk_column *column;
    //为列结构体对象申请内存
    rc = sqliterkColumnAlloc(&column);
    if (rc != SQLITERK_OK) {
        goto sqliterkBtreeParsePayload_End;
    }

    int i;
    for (i = 0; i < cellsCount; i++) {
        //清空列结构体数据
        sqliterkColumnClear(column);
        int offset = cellPointerArray[i];
        // Find payload
        int payloadSizeLength;
        int payloadSize;
        //读取payload大小
        rc = sqliterkParseVarint(pagedata, offset, &payloadSizeLength,
                                 &payloadSize);
        if (rc != SQLITERK_OK) {
            goto sqliterkBtreeParsePayload_End;
        }
        offset += payloadSizeLength;

        int rowidLength;
        int rowid;
        //读取行id
        rc = sqliterkParseVarint(pagedata, offset, &rowidLength, &rowid);
        if (rc != SQLITERK_OK) {
            goto sqliterkBtreeParsePayload_End;
        }
        offset += rowidLength;
        //设置行id
        sqliterkColumnSetRowId(column, rowid);

        //解析payload
        rc =
            sqliterkBtreeParsePayload(btree, page, offset, payloadSize, column);
        if (rc != SQLITERK_OK) {
            goto sqliterkBtreeParsePayload_End;
        }
    }
sqliterkBtreeParsePayload_End:
    if (column) {
        sqliterkColumnFree(column);
    }
    if (rc != SQLITERK_OK) {
        sqliterkOSDebug(rc, "Failed to parse payload.");
    }
    return rc;
}

// Parse the payload for leaf-table page only. We don't implement the parse
// method for index page, since we are not concerned about the data in an
// index page. See [Record Format] at https://www.sqlite.org/fileformat2.html
//只有叶节点表解析payload.我们并不提供解析索引页的方法，因为我们认为索引页的数据是安全的。
//解析payload
static int sqliterkBtreeParsePayload(sqliterk_btree *btree,
                                     sqliterk_page *page,
                                     int offset,
                                     int payloadSize,
                                     sqliterk_column *column)
{
    if (!btree || payloadSize <= 0 || !column) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;
    //申请内存
    unsigned char *payloadData = sqliterkOSMalloc(payloadSize);
    if (!payloadData) {
        rc = SQLITERK_NOMEM;
        goto sqliterkBtreeParseColumn_End;
    }

    // Check overflow
    int local = 0;
    //检测是否有溢出数据
    if (payloadSize <= btree->maxLeaf) {
        local = payloadSize;
    } else {
        // Since it is a leaf-table page, the max local should be equal to max leaf
        //???
        int maxPageLocal = btree->maxLeaf;
        int minPageLocal = btree->minLocal;
        int surplus =
            minPageLocal + (payloadSize - minPageLocal) %
                               (sqliterkPagerGetUsableSize(btree->pager) - 4);
        if (surplus <= maxPageLocal) {
            local = surplus;
        } else {
            local = minPageLocal;
        }
    }

    // Read data
    int payloadPointer = 0;
    //获取页中的数据
    const unsigned char *pagedata = sqliterkPageGetData(page);
    if (offset + local > sqliterkPagerGetSize(btree->pager)) {
        rc = SQLITERK_DAMAGED;
        goto sqliterkBtreeParseColumn_End;
    }
    //拷贝数据到payloadData
    memcpy(payloadData, pagedata + offset, local);
    payloadPointer += local;

    if (payloadPointer < payloadSize) {
        //获取溢出页信息
        sqliterk_values *overflowPages = sqliterkColumnGetOverflowPages(column);
        int overflowPageno;
        const unsigned char *pagedata = sqliterkPageGetData(page);
        //读取溢出页码
        sqliterkParseInt(pagedata, offset + local, 4, &overflowPageno);
        while (sqliterkPagerIsPagenoValid(btree->pager, overflowPageno) ==
               SQLITERK_OK) {
            //为values数组添加int类型value数据。添加溢出页码
            sqliterkValuesAddInteger(overflowPages, overflowPageno);
            //发送开始解析页通知
            if (btree->notify.onBeginParsePage) {
                btree->notify.onBeginParsePage(btree->rk, btree,
                                               overflowPageno);
            }
            sqliterk_page *page;
            //从文件获取溢出页的全部数据
            rc = sqliterkPageAcquireOverflow(btree->pager, overflowPageno,
                                             &page);
            //发送结束解析页通知
            if (btree->notify.onEndParsePage) {
                btree->notify.onEndParsePage(btree->rk, btree, overflowPageno,
                                             rc);
            }
            if (rc != SQLITERK_OK) {
                break;
            }
            // Read data
            int overflowSize = payloadSize - payloadPointer;
            //获取数据库中页实际能大小(页大小减去保留字段)
            int maxSize = sqliterkPagerGetUsableSize(btree->pager) - 4;
            if (overflowSize > maxSize) {
                overflowSize = maxSize;
            }

            //获取页中的数据
            const unsigned char *pageData = sqliterkPageGetData(page);
            //拷贝数据到payloadData
            memcpy(payloadData + payloadPointer, pageData + 4, overflowSize);
            payloadPointer += overflowSize;
            // Iterate
            //解析下个溢出页码
            sqliterkParseInt(pageData, 0, 4, &overflowPageno);
            // Clear
            //清除页信息
            sqliterkPageRelease(page);
        }
    }

    int columnOffsetValue = 0;
    int columnOffsetValueLength = 0;
    //读取列便宜数据
    rc = sqliterkParseVarint(payloadData, 0, &columnOffsetValueLength,
                             &columnOffsetValue);
    if (rc != SQLITERK_OK) {
        goto sqliterkBtreeParseColumn_End;
    }

    //串行类型 列偏移数据
    int offsetSerialType = columnOffsetValueLength;
    int offsetValue = columnOffsetValue;
    //串行类型 结束偏移量
    const int endSerialType = offsetValue;
    //结束大小
    const int endValue = payloadSize;

    int serialTypeLength = 0;
    int serialType = 0;
    int valueLength = 0;

    //获取列对象的数据数组sqliterk_values
    sqliterk_values *values = sqliterkColumnGetValues(column);
    while (offsetValue < endValue || offsetSerialType < endSerialType) {
        //读取serialType
        rc = sqliterkParseVarint(payloadData, offsetSerialType,
                                 &serialTypeLength, &serialType);
        if (rc != SQLITERK_OK) {
            goto sqliterkBtreeParseColumn_End;
        }
        //获取串行类型长度
        valueLength = sqliterkBtreeGetLengthForSerialType(serialType);
        if (serialType == 0) {
            //为values数组添加空数据
            rc = sqliterkValuesAddNull(values);
        } else if (serialType < 7) {
            int64_t value;
            //解析offsetValue位置数据
            sqliterkParseInt64(payloadData, offsetValue, valueLength, &value);
            //添加到values中
            rc = sqliterkValuesAddInteger64(values, value);
        } else if (serialType == 7) {
            double value;
            //解析offsetValue位置数据
            sqliterkParseNumber(payloadData, offsetValue, &value);
            //添加到values中
            rc = sqliterkValuesAddNumber(values, value);
        } else if (serialType == 8) {
            //添加0
            rc = sqliterkValuesAddInteger(values, 0);
        } else if (serialType == 9) {
            //添加1
            rc = sqliterkValuesAddInteger(values, 1);
        } else if (serialType >= 12) {
            if (serialType % 2 == 0) {
                //添加二进制块
                rc = sqliterkValuesAddBinary(values, payloadData + offsetValue,
                                             valueLength);
            } else {
                //添加text型数据
                rc = sqliterkValuesAddNoTerminatorText(
                    values, (const char *) payloadData + offsetValue,
                    valueLength);
            }
        } else {
            rc = SQLITERK_DAMAGED;
        }
        if (rc != SQLITERK_OK) {
            goto sqliterkBtreeParseColumn_End;
        }
        offsetValue += valueLength;
        offsetSerialType += serialTypeLength;
    }
    if (offsetSerialType != endSerialType || offsetValue != endValue) {
        rc = SQLITERK_DAMAGED;
        goto sqliterkBtreeParseColumn_End;
    }

sqliterkBtreeParseColumn_End:
    if (rc == SQLITERK_OK && btree->notify.onParseColumn) {
        btree->notify.onParseColumn(btree->rk, btree, page, column);
    }
    if (payloadData) {
        sqliterkOSFree(payloadData);
    }
    return rc;
}

//关闭btree(二叉树)
int sqliterkBtreeClose(sqliterk_btree *btree)
{
    if (!btree) {
        return SQLITERK_MISUSE;
    }
    if (btree->name) {
        sqliterkOSFree(btree->name);
        btree->name = NULL;
    }
    if (btree->rootpage) {
        sqliterkPageRelease(btree->rootpage);
        btree->rootpage = NULL;
    }
    btree->pager = NULL;
    btree->userInfo = NULL;
    btree->rk = NULL;
    btree->type = 0;
    sqliterkOSFree(btree);
    return SQLITERK_OK;
}

//设置btree（二叉树）name
int sqliterkBtreeSetMeta(sqliterk_btree *btree,
                         const char *name,
                         sqliterk_btree_type type)
{
    if (!btree) {
        return SQLITERK_MISUSE;
    }
    //如果有name,释放
    if (btree->name) {
        sqliterkOSFree(btree->name);
        btree->name = NULL;
    }
    if (name) {
        //设置btree->name
        size_t length = strlen(name);
        btree->name = sqliterkOSMalloc(sizeof(char) * (length + 1));
        if (!btree->name) {
            return SQLITERK_NOMEM;
        }
        strncpy(btree->name, name, length);
        // If it's a system btree name, then setup its b-tree type.
        //如果是系统btree名字，设置btree类型
        sqliterk_btree_type i;
        for (i = sqliterk_btree_type_system_begin;
             i < sqliterk_btree_type_system_end; i++) {
            const char *typename = sqliterkBtreeGetTypeName(i);
            if (strncmp(btree->name, typename, strlen(typename)) == 0) {
                btree->type = i;
                break;
            }
        }
    } else {
        btree->name = NULL;
    }
    //设置btree类型
    if (!sqliterkBtreeIsSystemType(btree->type) &&
        type != sqliterk_btree_type_unknown) {
        btree->type = type;
    }
    return SQLITERK_OK;
}

//获取btree名字
const char *sqliterkBtreeGetName(sqliterk_btree *btree)
{
    if (!btree) {
        return NULL;
    }
    return btree->name;
}

//获取btree类型
sqliterk_btree_type sqliterkBtreeGetType(sqliterk_btree *btree)
{
    if (!btree) {
        return sqliterk_btree_type_unknown;
    }
    return btree->type;
}

//设置btree类型
int sqliterkBtreeSetType(sqliterk_btree *btree, sqliterk_btree_type type)
{
    if (!btree) {
        return SQLITERK_MISUSE;
    }
    if (sqliterkBtreeIsSystemType(btree->type)) {
        // You can only set the type manually when the type is not a system type
        return SQLITERK_MISUSE;
    }
    btree->type = type;
    return SQLITERK_OK;
}

//判断btree是否系统类型
int sqliterkBtreeIsSystemType(sqliterk_btree_type type)
{
    if (type >= sqliterk_btree_type_system_begin &&
        type < sqliterk_btree_type_system_end) {
        return 1;
    }
    return 0;
}

//设置btree通知
void sqliterkBtreeSetNotify(sqliterk_btree *btree,
                            sqliterk_btree_notify *notify)
{
    if (!btree || !notify) {
        return;
    }
    btree->notify = *notify;
}

//设置btree的userinfo
void sqliterkBtreeSetUserInfo(sqliterk_btree *btree, void *userInfo)
{
    if (!btree) {
        return;
    }
    btree->userInfo = userInfo;
}

//获取btree的userinfo
void *sqliterkBtreeGetUserInfo(sqliterk_btree *btree)
{
    if (!btree) {
        return NULL;
    }
    return btree->userInfo;
}

//返回btree的根页
sqliterk_page *sqliterkBtreeGetRootPage(sqliterk_btree *btree)
{
    if (!btree) {
        return NULL;
    }
    return btree->rootpage;
}

//获取btree 类型的描述字符串
const char *sqliterkBtreeGetTypeName(sqliterk_btree_type type)
{
    char *name;
    switch (type) {
        case sqliterk_btree_type_autoindex:
            name = "sqlite_autoindex";
            break;
        case sqliterk_btree_type_sequence:
            name = "sqlite_sequence";
            break;
        case sqliterk_btree_type_stat:
            name = "sqlite_stat";
            break;
        case sqliterk_btree_type_master:
            name = "sqlite_master";
            break;
        case sqliterk_btree_type_table:
            name = "table";
            break;
        case sqliterk_btree_type_index:
            name = "index";
            break;
        default:
            name = "unknown";
            break;
    }
    return name;
}

// See [Serial Type Codes Of The Record Format]
// at https://www.sqlite.org/fileformat2.html
//返回btree串行长度
static int sqliterkBtreeGetLengthForSerialType(int serialType)
{
    if (serialType < 0) {
        return 0;
    }
    static int sqliterk_btree_serialtype_fixlengths[12] = {0, 1, 2, 3, 4, 6,
                                                           8, 8, 0, 0, 0, 0};
    if (serialType < 12) {
        return sqliterk_btree_serialtype_fixlengths[serialType];
    }
    return (serialType - 12 - serialType % 2) / 2;
}
