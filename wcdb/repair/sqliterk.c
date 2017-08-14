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

#include "sqliterk.h"
#include "SQLiteRepairKit.h"
#include "sqliterk_btree.h"
#include "sqliterk_column.h"
#include "sqliterk_os.h"
#include "sqliterk_pager.h"
#include "sqliterk_util.h"
#include "sqliterk_values.h"
#include <string.h>

//数据库结构体
struct sqliterk {
    //数据库页
    sqliterk_pager *pager;
    //监听
    sqliterk_btree_notify listen;
    //通知
    sqliterk_notify notify;
    //用户信息 可能是sqliterk_output_ctx对象
    void *userInfo;
    //保留字段
    char recursive;
};

//declaration
//开始解析btree通知
static void sqliterkNotify_onBeginParseBtree(sqliterk *rk,
                                             sqliterk_btree *btree);
//结束解析btree通知
static void
sqliterkNotify_onEndParseBtree(sqliterk *rk, sqliterk_btree *btree, int result);
//开始解析列通知
static void sqliterkNotify_onParseColumn(sqliterk *rk,
                                         sqliterk_btree *btree,
                                         sqliterk_page *page,
                                         sqliterk_column *column);
//开始解析页通知
static int sqliterkNotify_onBeginParsePage(sqliterk *rk,
                                           sqliterk_btree *btree,
                                           int pageno);
//结束解析页通知
static void sqliterkNotify_onEndParsePage(sqliterk *rk,
                                          sqliterk_btree *btree,
                                          int pageno,
                                          int result);
//解析btree通知
static int sqliterkParseBtree(sqliterk *rk, sqliterk_btree *btree);

//打开数据库
int sqliterkOpen(const char *path,
                 const sqliterk_cipher_conf *cipher,
                 sqliterk **rk)
{
    if (!rk) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;
    //数据库结构体分配内存
    sqliterk *therk = sqliterkOSMalloc(sizeof(sqliterk));
    if (!therk) {
        rc = SQLITERK_NOMEM;
        sqliterkOSError(rc, "Not enough memory, required: %u bytes",
                        sizeof(sqliterk));
        goto sqliterkOpen_Failed;
    }
    //打开指定路径数据库到therk->pager
    rc = sqliterkPagerOpen(path, cipher, &therk->pager);
    if (rc != SQLITERK_OK) {
        goto sqliterkOpen_Failed;
    }

    //设置通知函数
    therk->listen.onBeginParsePage = sqliterkNotify_onBeginParsePage;
    therk->listen.onEndParsePage = sqliterkNotify_onEndParsePage;
    therk->listen.onBeginParseBtree = sqliterkNotify_onBeginParseBtree;
    therk->listen.onEndParseBtree = sqliterkNotify_onEndParseBtree;
    therk->listen.onParseColumn = sqliterkNotify_onParseColumn;
    therk->recursive = 1;

    *rk = therk;
    sqliterkOSInfo(SQLITERK_OK, "RepairKit on '%s' opened, %s.", path,
                   cipher ? "encrypted" : "plain-text");
    return SQLITERK_OK;

sqliterkOpen_Failed:
    if (therk) {
        sqliterkClose(therk);
    }
    *rk = NULL;
    return rc;
}

//设置数据库保留字段
void sqliterk_set_recursive(sqliterk *rk, int recursive)
{
    rk->recursive = (char) recursive;
}

//解析数据库
int sqliterkParse(sqliterk *rk)
{
    if (!rk) {
        return SQLITERK_MISUSE;
    }

    int i;
    //获取数据库所有的页，并解析
    for (i = 0; i < sqliterkPagerGetPageCount(rk->pager); i++) {
        int pageno = i + 1;
        sqliterkParsePage(rk, pageno);
    }
    return SQLITERK_OK;
}

//解析数据库页
int sqliterkParsePage(sqliterk *rk, int pageno)
{
    if (!rk) {
        return SQLITERK_MISUSE;
    }
    //如果pager的状态不是未检查，则返回数据库完成。
    if (sqliterkPagerGetStatus(rk->pager, pageno) !=
        sqliterk_status_unchecked) {
        return SQLITERK_OK;
    }
    //如果是未检查，则打开btree
    int rc = SQLITERK_OK;
    sqliterk_btree *btree = NULL;
    rc = sqliterkBtreeOpen(rk, rk->pager, pageno, &btree);
    if (rc != SQLITERK_OK) {
        goto sqliterkParsePage_End;
    }
    //解析btree
    rc = sqliterkParseBtree(rk, btree);
sqliterkParsePage_End:
    if (btree) {
        //关闭btree(二叉树)
        sqliterkBtreeClose(btree);
    }
    return rc;
}

//解析btree
static int sqliterkParseBtree(sqliterk *rk, sqliterk_btree *btree)
{
    if (!rk) {
        return SQLITERK_MISUSE;
    }
    //获取btree的根页
    sqliterk_page *page = sqliterkBtreeGetRootPage(btree);
    //获取页的页码
    int pageno = sqliterkPageGetPageno(page);
    if (!page || sqliterkPagerIsPagenoValid(rk->pager, pageno) != SQLITERK_OK) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;
    //设置btree监听
    sqliterkBtreeSetNotify(btree, &rk->listen);
    //解析btree
    rc = sqliterkBtreeParse(btree);
    return rc;
}

//解析master页
//所有表或索引的根页编号都存储在系统表sqlite_master中，表sqlite_master的根页为page 1
int sqliterkParseMaster(sqliterk *rk)
{
    // The page 1 is always sqlite_master. See [B-tree Pages] at
    // https://www.sqlite.org/fileformat2.html
    return sqliterkParsePage(rk, 1);
}

//关闭数据库
int sqliterkClose(sqliterk *rk)
{
    if (!rk) {
        return SQLITERK_MISUSE;
    }
    if (rk->pager) {
        sqliterkPagerClose(rk->pager);
        rk->pager = NULL;
    }
    sqliterkOSFree(rk);
    return SQLITERK_OK;
}

//开始解析btree通知函数
static void sqliterkNotify_onBeginParseBtree(sqliterk *rk,
                                             sqliterk_btree *btree)
{
    //调用解析表通知
    if (rk->notify.onBeginParseTable) {
        rk->notify.onBeginParseTable(rk, (sqliterk_table *) btree);
    }
    //获取btree的根页
    sqliterk_page *rootpage = sqliterkBtreeGetRootPage(btree);
    //打印信息
    sqliterkOSDebug(
        SQLITERK_OK, "Parsing B-tree -> [root: %d, name: %s, type: %s]",
        sqliterkPageGetPageno(rootpage), sqliterkBtreeGetName(btree),
        sqliterkBtreeGetTypeName(sqliterkBtreeGetType(btree)));
}

//结束解析btree通知函数
static void
sqliterkNotify_onEndParseBtree(sqliterk *rk, sqliterk_btree *btree, int result)
{
    //调用结束解析表通知
    if (rk->notify.onEndParseTable) {
        rk->notify.onEndParseTable(rk, (sqliterk_table *) btree);
    }
}

//解析列通知
static void sqliterkNotify_onParseColumn(sqliterk *rk,
                                         sqliterk_btree *btree,
                                         sqliterk_page *page,
                                         sqliterk_column *column)
{
    if (!rk) {
        return;
    }
    //调用解析列通知
    if (rk->notify.onParseColumn) {
        //获取解析列结果
        int result =
            rk->notify.onParseColumn(rk, (sqliterk_table *) btree, column);
        //获取页码
        int pageno = sqliterkPageGetPageno(page);
        //如果数据库返回错误，则设置页和所有溢出页状态为丢弃 sqliterk_status_discarded
        if (result != SQLITERK_OK) {
            sqliterkPagerSetStatus(rk->pager, pageno,
                                   sqliterk_status_discarded);
            sqliterk_values *overflowPages =
                sqliterkColumnGetOverflowPages(column);

            int i;
            for (i = 0; i < sqliterkValuesGetCount(overflowPages); i++) {
                sqliterkPagerSetStatus(
                    rk->pager, sqliterkValuesGetInteger(overflowPages, i),
                    sqliterk_status_discarded);
            }
        }
    }
    //如果是master btree并且预留字段不为空
    if (sqliterkBtreeGetType(btree) == sqliterk_btree_type_master &&
        rk->recursive) {
        // Recursively decode the page since the mapping of [table]->[rootPageno] is known
        //递归解码页信息
        sqliterk_values *values = sqliterkColumnGetValues(column);
        const char *type = sqliterkValuesGetText(values, 0);
        const char *name = sqliterkValuesGetText(values, 1);
        int rootPageno = sqliterkValuesGetInteger(values, 3);
        int rc = SQLITERK_OK;
        if (type && name) {
            sqliterk_btree *subbtree;
            //打开btree
            rc = sqliterkBtreeOpen(rk, rk->pager, rootPageno, &subbtree);
            //设置btree（二叉树）name
            if (rc == SQLITERK_OK) {
                if (memcmp("table", type, 5) == 0) {
                    sqliterkBtreeSetMeta(subbtree, name,
                                         sqliterk_btree_type_table);
                } else if (memcmp("index", type, 5) == 0) {
                    sqliterkBtreeSetMeta(subbtree, name,
                                         sqliterk_btree_type_index);
                } else {
                    sqliterkBtreeSetMeta(subbtree, name,
                                         sqliterk_btree_type_unknown);
                }
                rc = sqliterkParseBtree(rk, subbtree);
            }
            if (rc != SQLITERK_OK) {
                sqliterk_page *rootpage = sqliterkBtreeGetRootPage(subbtree);
                sqliterkOSError(
                    rc,
                    "sqliterkNotify_onParseColumn: failed to parse known "
                    "table with root page no. %d, name %s, type %s",
                    sqliterkPageGetPageno(rootpage),
                    sqliterkBtreeGetName(subbtree),
                    sqliterkBtreeGetTypeName(sqliterkBtreeGetType(subbtree)));
            }
            if (subbtree) {
                sqliterkBtreeClose(subbtree);
            }
        }
    }
}

//开始解析页通知
static int
sqliterkNotify_onBeginParsePage(sqliterk *rk, sqliterk_btree *btree, int pageno)
{
    //sqliterkOSDebug(SQLITERK_OK, "sqliterkNotify_onBeginParsePage: %d", pageno);
    if (sqliterkPagerGetStatus(rk->pager, pageno) == sqliterk_status_checking) {
        return SQLITERK_MISUSE;
    }
    //设置页面状态为正在解析
    sqliterkPagerSetStatus(rk->pager, pageno, sqliterk_status_checking);
    return SQLITERK_OK;
}

//结束解析页通知
static void sqliterkNotify_onEndParsePage(sqliterk *rk,
                                          sqliterk_btree *btree,
                                          int pageno,
                                          int result)
{
    if (!rk) {
        return;
    }
    //设置页状态
    switch (result) {
        case SQLITERK_OK:
            sqliterkPagerSetStatus(rk->pager, pageno, sqliterk_status_checked);
            break;
        case SQLITERK_DAMAGED:
            sqliterkPagerSetStatus(rk->pager, pageno, sqliterk_status_damaged);
            break;
        default:
            sqliterkOSWarning(SQLITERK_MISUSE,
                              "Cannot parse page %d. Invalid type.", pageno);
            sqliterkPagerSetStatus(rk->pager, pageno, sqliterk_status_invalid);
            break;
    }
    //sqliterkOSDebug(result, "sqliterkNotify_onEndParsePage: %d", pageno);
}

//数据库设置通知
int sqliterkSetNotify(sqliterk *rk, sqliterk_notify notify)
{
    if (!rk) {
        return SQLITERK_MISUSE;
    }
    rk->notify = notify;
    return SQLITERK_OK;
}

//数据库设置用户信息
int sqliterkSetUserInfo(sqliterk *rk, void *userInfo)
{
    if (!rk) {
        return SQLITERK_MISUSE;
    }
    rk->userInfo = userInfo;
    return SQLITERK_OK;
}

//数据库获取用户信息
void *sqliterkGetUserInfo(sqliterk *rk)
{
    if (!rk) {
        return NULL;
    }
    return rk->userInfo;
}

//数据库获取解析的页面数
int sqliterkGetParsedPageCount(sqliterk *rk)
{
    if (!rk) {
        return 0;
    }
    return sqliterkPagerGetParsedPageCount(rk->pager);
}

//获取数据库验证过的页数
int sqliterkGetValidPageCount(sqliterk *rk)
{
    if (!rk) {
        return 0;
    }
    return sqliterkPagerGetValidPageCount(rk->pager);
}

//获取数据库总页数
int sqliterkGetPageCount(sqliterk *rk)
{
    if (!rk) {
        return 0;
    }
    return sqliterkPagerGetPageCount(rk->pager);
}

//获取数据库完整性
unsigned int sqliterkGetIntegrity(sqliterk *rk)
{
    if (!rk) {
        return 0;
    }
    return sqliterkPagerGetIntegrity(rk->pager);
}
