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

#include "sqliterk_pager.h"
#include "SQLiteRepairKit.h"
#include "sqliterk_crypto.h"
#include "sqliterk_os.h"
#include "sqliterk_util.h"
#include <errno.h>
#include <string.h>

//解析数据库头信息
static int sqliterkPagerParseHeader(sqliterk_pager *pager, int forcePageSize);
//获取数据库单页信息
static int sqliterkPageAcquireOne(sqliterk_pager *pager,
                                  int pageno,
                                  sqliterk_page **page,
                                  sqliterk_page_type type);
//sqliterk_page 是数据库单一页的结构体，包含页的数据。为了减少内存，页的数据可能会提前释放
struct sqliterk_page {
    //页码
    int pageno;
    //页数据
    unsigned char *data; // page data
    //类型
    sqliterk_page_type type;
};

//打开指定路径数据库
int sqliterkPagerOpen(const char *path,
                      const sqliterk_cipher_conf *cipher,
                      sqliterk_pager **pager)
{
    // Workaround page size cannot be specified for plain-text
    // databases. For that case, pass non-null cipher_conf with
    // null key and non-zero page size.
    //页大小(page size)不能指定纯文本数据库。如果发生这种情况，传入非空chiper和空的key值，和非0页大小
    int forcePageSize = 0;
    //检测是否纯文本数据库
    if (cipher && !cipher->key) {
        forcePageSize = cipher->page_size;
        cipher = NULL;
    }

    if (!pager) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;
    //分配页管理者内存
    sqliterk_pager *thePager = sqliterkOSMalloc(sizeof(sqliterk_pager));
    if (!thePager) {
        rc = SQLITERK_NOMEM;
        sqliterkOSError(rc, "Not enough memory, required %u bytes.",
                        sizeof(sqliterk_pager));
        goto sqliterkPagerOpen_Failed;
    }

    ////使用只读方式打开数据库文件
    rc = sqliterkOSReadOnlyOpen(path, &thePager->file);
    if (rc != SQLITERK_OK) {
        goto sqliterkPagerOpen_Failed;
    }

    //如果是非纯文本数据库
    if (cipher) {
        // Try KDF salt in SQLite file first.
        //拷贝cipher到C
        sqliterk_cipher_conf c;
        memcpy(&c, cipher, sizeof(c));
        c.kdf_salt = NULL;

        //设置加密对象和加密方法?
        rc = sqliterkCryptoSetCipher(thePager, thePager->file, &c);
        if (rc != SQLITERK_OK)
            goto sqliterkPagerOpen_Failed;

        // Try parsing header.
        //解析页管理者的头
        sqliterkPagerParseHeader(thePager, 0);

        if (thePager->integrity & SQLITERK_INTEGRITY_HEADER) {
            // If header is parsed successfully, original KDF salt is also correct.
            //如果头信息完整，则最初的kdf salt也是完整的
            thePager->integrity |= SQLITERK_INTEGRITY_KDF_SALT;
            //如果头信息不完整
        } else if (cipher->kdf_salt) {
            // If anything goes wrong, use KDF salt specified in cipher config.
            sqliterkOSWarning(SQLITERK_DAMAGED, "Header cannot be decoded "
                                                "correctly. Trying to apply "
                                                "recovery data.");
            //设置加密上下文
            rc = sqliterkCryptoSetCipher(thePager, thePager->file, cipher);
            if (rc != SQLITERK_OK)
                goto sqliterkPagerOpen_Failed;

            //解析头信息
            rc = sqliterkPagerParseHeader(thePager, 0);
            if (rc != SQLITERK_OK)
                goto sqliterkPagerOpen_Failed;
        }
    } else {
        //解析头信息
        rc = sqliterkPagerParseHeader(thePager, forcePageSize);
        if (rc != SQLITERK_OK)
            goto sqliterkPagerOpen_Failed;

        // For plain-text databases, just mark KDF salt correct.
        //如果是纯文本数据库，保证kdf_salt正确
        if (thePager->integrity & SQLITERK_INTEGRITY_HEADER)
            thePager->integrity |= SQLITERK_INTEGRITY_KDF_SALT;
    }
    if (!(thePager->integrity & SQLITERK_INTEGRITY_HEADER))
        sqliterkOSWarning(SQLITERK_DAMAGED, "Header corrupted.");
    else
        sqliterkOSInfo(SQLITERK_OK, "Header checksum OK.");

    //为所有页的状态申请内存
    int pageCount = thePager->pagecount;
    size_t len = sizeof(sqliterk_status) * (pageCount + 1);
    thePager->pagesStatus = sqliterkOSMalloc(len);
    if (!thePager->pagesStatus) {
        rc = SQLITERK_NOMEM;
        sqliterkOSError(rc, "Not enough memory, required %u bytes.", len);
        goto sqliterkPagerOpen_Failed;
    }

    *pager = thePager;

    return SQLITERK_OK;

sqliterkPagerOpen_Failed:
    if (thePager) {
        sqliterkPagerClose(thePager);
    }
    *pager = NULL;
    return rc;
}

// Get the meta from header and set it into pager.
// For further information, see https://www.sqlite.org/fileformat2.html
//解析pager头信息
static int sqliterkPagerParseHeader(sqliterk_pager *pager, int forcePageSize)
{
    // For encrypted databases, assume default page size, decode the first
    // page, and we have the plain-text header.

    if (!pager) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;

    // Overwrite pager page size if forcePageSize is specified.
    //如果指定了页大小，则更新页大小
    if (forcePageSize) {
        pager->pagesize = forcePageSize;
    }

    //数据库的第一页btree页，前100个字节是数据库文件的描述
    size_t size = pager->codec ? pager->pagesize : 100;

    // Read data
    //为buffer申请内存
    unsigned char *buffer = sqliterkOSMalloc(size);
    if (!buffer) {
        rc = SQLITERK_NOMEM;
        sqliterkOSError(rc, "Not enough memory, required %u bytes.", size);
        goto sqliterkPagerParseHeader_End;
    }

    //读取pager-file中一页的数据大小到buffer.
    //数据库第一页的信息是btree页，Page 1的前100个字节是一个对数据库文件进行描述的“文件头”。它包括数据库的版本、格式的版本、页大小、编码等所有创建数据库时设置的永久性参数。
    rc = sqliterkOSRead(pager->file, 0, buffer, &size);
    if (rc != SQLITERK_OK) {
        if (rc == SQLITERK_SHORT_READ)
            sqliterkOSError(rc, "File truncated.");
        else
            sqliterkOSError(rc, "Cannot read file '%s': %s",
                            sqliterkOSGetFilePath(pager->file),
                            strerror(errno));
        //读取失败，设置完整性标志位
        pager->integrity &= ~SQLITERK_INTEGRITY_HEADER;
        goto sqliterkPagerParseHeader_End;
    }

    //读取成功，设置完整性标志位
    pager->integrity |= SQLITERK_INTEGRITY_HEADER;

    //如果有加密上下文
    if (pager->codec) {
        //解密信息
        rc = sqliterkCryptoDecode(pager->codec, 1, buffer);
        if (rc != SQLITERK_OK) {
            sqliterkOSWarning(SQLITERK_DAMAGED,
                              "Failed to decode page 1, header corrupted.");
            pager->integrity &= ~SQLITERK_INTEGRITY_HEADER;
        }
    }

    //如果头部信息是完整的，则向下执行
    //http://blog.csdn.net/zzmgood/article/details/48209977
    if (pager->integrity & SQLITERK_INTEGRITY_HEADER) {
        //确定buffer读取的数据库的第一页，btree页面
        if (memcmp(buffer, "SQLite format 3\000", 16) == 0) {
            //parse pagesize
            int pagesize;
            //读取data中offset偏移，length长度的int
            sqliterkParseInt(buffer, 16, 2, &pagesize);
            if (pager->codec || forcePageSize) {
                // Page size is predefined, check whether it matches the header.
                //页面大小是余定义的，检测是否匹配
                if (pagesize != pager->pagesize) {
                    sqliterkOSWarning(
                        SQLITERK_DAMAGED,
                        "Invalid page size: %d expected, %d returned.",
                        pager->pagesize, pagesize);
                    pager->integrity &= ~SQLITERK_INTEGRITY_HEADER;
                }
                //数据库文件包括一个或多个页。页的大小可以是512B到64KB之间的2的任意次方。
            } else if (((pagesize - 1) & pagesize) != 0 || pagesize < 512) {
                // Page size is not predefined and value in the header is invalid,
                // use the default page size.
                //页大小不是预定义的，并且失效，使用默认页大小
                sqliterkOSWarning(SQLITERK_DAMAGED,
                                  "Page size field is corrupted. Default page "
                                  "size %d is used",
                                  SQLITRK_CONFIG_DEFAULT_PAGESIZE);
                pager->pagesize = SQLITRK_CONFIG_DEFAULT_PAGESIZE;
                pager->integrity &= ~SQLITERK_INTEGRITY_HEADER;
            } else {
                // Page size is not predefined and value in the header is valid,
                // use the value in header.
                //页大小不是预定义，但是头中的大小有效，使用头中定义的大小
                pager->pagesize = pagesize;
            }

            // parse free page count
            //获取未使用的页数量
            sqliterkParseInt(buffer, 36, 4, &pager->freepagecount);

            // parse reserved bytes
            int reservedBytes;
            //读取页的最后部分保留的字节数
            //Bytes of unused "reserved" space at the end of each page. Usually 0.
            sqliterkParseInt(buffer, 20, 1, &reservedBytes);
            if (pager->codec) {
                //如果读取出的保留字节数和pager中的不一致，则说明头信息错误
                if (reservedBytes != pager->reservedBytes) {
                    sqliterkOSWarning(SQLITERK_DAMAGED,
                                      "Reserved bytes field doesn't match. %d "
                                      "expected, %d returned.",
                                      pager->reservedBytes, reservedBytes);
                    //头信息错误
                    pager->integrity &= ~SQLITERK_INTEGRITY_HEADER;
                }
                //保留字节数错误，设置为0.设置头信息错误
            } else if (reservedBytes < 0 || reservedBytes > 255) {
                sqliterkOSWarning(
                    SQLITERK_DAMAGED,
                    "The [reserved bytes] field is corrupted. 0 is used");
                pager->reservedBytes = 0;
                pager->integrity &= ~SQLITERK_INTEGRITY_HEADER;
            } else
                pager->reservedBytes = reservedBytes;
            //头信息错误，设置为默认数据
        } else {
            // Header is corrupted. Defaults the config
            sqliterkOSWarning(SQLITERK_DAMAGED,
                              "SQLite format magic corrupted.");
            if (!pager->codec) {
                pager->pagesize = SQLITRK_CONFIG_DEFAULT_PAGESIZE;
                pager->reservedBytes = 0;
            }
            pager->freepagecount = 0;
            pager->integrity &= ~SQLITERK_INTEGRITY_HEADER;
        }
    }

    // Assign page count
    size_t filesize;
    //读取数据库文件大小
    rc = sqliterkOSFileSize(pager->file, &filesize);
    if (rc != SQLITERK_OK) {
        sqliterkOSError(rc, "Failed to get size of file '%s': %s",
                        sqliterkOSGetFilePath(pager->file), strerror(errno));
        goto sqliterkPagerParseHeader_End;
    }

    //计算pager中的页数
    pager->pagecount =
        (int) ((filesize + pager->pagesize - 1) / pager->pagesize);
    if (pager->pagecount < 1) {
        rc = SQLITERK_DAMAGED;
        sqliterkOSError(rc, "File truncated.");
        goto sqliterkPagerParseHeader_End;
    }

    // Check free page
    //如果未用页小于0，或者未用页比总页数多，则认为为用页数据错误。设置默认数据
    if (pager->freepagecount < 0 || pager->freepagecount > pager->pagecount) {
        sqliterkOSWarning(
            SQLITERK_DAMAGED,
            "The [free page count] field is corrupted. 0 is used");
        pager->freepagecount = 0;
        pager->integrity &= ~SQLITERK_INTEGRITY_HEADER;
    }

    // Assign usableSize
    //页数据真实可用大小
    pager->usableSize = pager->pagesize - pager->reservedBytes;

sqliterkPagerParseHeader_End:
    if (buffer) {
        sqliterkOSFree(buffer);
    }
    return rc;
}

//关闭数据库
int sqliterkPagerClose(sqliterk_pager *pager)
{
    if (!pager) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;
    //关闭数据库文件
    if (pager->file) {
        rc = sqliterkOSClose(pager->file);
        pager->file = NULL;
    }
    //释放数据库页状态数组
    if (pager->pagesStatus) {
        sqliterkOSFree(pager->pagesStatus);
        pager->pagesStatus = NULL;
    }
    pager->pagesize = 0;
    pager->pagecount = 0;

    //释放加密上下文内存
    sqliterkCryptoFreeCodec(pager);

    //释放pager内存
    sqliterkOSFree(pager);
    return rc;
}

//读取总页数
int sqliterkPagerGetPageCount(sqliterk_pager *pager)
{
    if (!pager) {
        return 0;
    }
    return pager->pagecount;
}

//验证指定页码是否存在
int sqliterkPagerIsPagenoValid(sqliterk_pager *pager, int pageno)
{
    if (!pager || pageno < 1 || pageno > pager->pagecount) {
        return SQLITERK_MISUSE;
    }
    return SQLITERK_OK;
}

// Get the page type from file at page [pageno]
//使用pager获取指定页的类型 
int sqliterkPageAcquireType(sqliterk_pager *pager,
                            int pageno,
                            sqliterk_page_type *type)
{
    // TODO: for encrypted databases, decode the whole page.
    // Use sqliterkPageAcquire instead.

    if (!pager || sqliterkPagerIsPagenoValid(pager, pageno) != SQLITERK_OK ||
        !type) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;
    unsigned char typedata;
    size_t typesize = 1;
    //读取数据库文件数据
    rc = sqliterkOSRead(pager->file,
                        sqliterkPagenoHeaderOffset(pageno) +
                            (pageno - 1) * pager->pagesize,
                        &typedata, &typesize);
    if (rc != SQLITERK_OK) {
        goto sqliterkPageAcquireType_Failed;
    }

    //读取typedata中文件开头1字节的数据，type数据
    int theType;
    sqliterkParseInt(&typedata, 0, 1, &theType);
    //设置type
    switch (theType) {
        case sqliterk_page_type_interior_index:
        case sqliterk_page_type_interior_table:
        case sqliterk_page_type_leaf_index:
        case sqliterk_page_type_leaf_table:
            *type = theType;
            break;
        default:
            *type = sqliterk_page_type_unknown;
            break;
    }
    return SQLITERK_OK;

sqliterkPageAcquireType_Failed:
    *type = sqliterk_page_type_unknown;
    return rc;
}

// Get whole page data from file at page [pageno] and setup the [page].
//从文件获取页的全部数据，并初始化页
int sqliterkPageAcquire(sqliterk_pager *pager, int pageno, sqliterk_page **page)
{
    return sqliterkPageAcquireOne(pager, pageno, page,
                                  sqliterk_page_type_unknown);
}

//从文件获取溢出页的全部数据
int sqliterkPageAcquireOverflow(sqliterk_pager *pager,
                                int pageno,
                                sqliterk_page **page)
{
    return sqliterkPageAcquireOne(pager, pageno, page,
                                  sqliterk_page_type_overflow);
}

//获取数据库单页信息
static int sqliterkPageAcquireOne(sqliterk_pager *pager,
                                  int pageno,
                                  sqliterk_page **page,
                                  sqliterk_page_type type)
{
    if (!pager || !page ||
        sqliterkPagerIsPagenoValid(pager, pageno) != SQLITERK_OK) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;
    //为页申请内存
    sqliterk_page *thePage = sqliterkOSMalloc(sizeof(sqliterk_page));
    if (!thePage) {
        rc = SQLITERK_NOMEM;
        goto sqliterkPageAcquire_Failed;
    }

    thePage->pageno = pageno;

    //为页数据申请内存
    thePage->data = sqliterkOSMalloc(pager->pagesize);
    if (!thePage->data) {
        rc = SQLITERK_NOMEM;
        goto sqliterkPageAcquire_Failed;
    }

    size_t size = pager->pagesize;
    //从数据库文件中，读取pageno页的数据到data中
    rc = sqliterkOSRead(pager->file, (pageno - 1) * pager->pagesize,
                        thePage->data, &size);
    if (rc != SQLITERK_OK) {
        goto sqliterkPageAcquire_Failed;
    }

    // For encrypted databases, decode page.
    //如果是加密数据库，解密页数据
    if (pager->codec) {
        rc = sqliterkCryptoDecode(pager->codec, pageno, thePage->data);
        if (rc != SQLITERK_OK)
            goto sqliterkPageAcquire_Failed;
    }

    // Check type
    if (type == sqliterk_page_type_unknown) {
        //读取页的type数据
        sqliterkParseInt(thePage->data, sqliterkPageHeaderOffset(thePage), 1,
                         &type);
        switch (type) {
            case sqliterk_page_type_interior_index:
            case sqliterk_page_type_interior_table:
            case sqliterk_page_type_leaf_index:
            case sqliterk_page_type_leaf_table:
                thePage->type = type;
                break;
            default:
                thePage->type = sqliterk_page_type_unknown;
                break;
        }
    } else {
        thePage->type = type;
    }

    *page = thePage;
    return SQLITERK_OK;

sqliterkPageAcquire_Failed:
    if (thePage) {
        sqliterkPageRelease(thePage);
    }
    *page = NULL;
    return rc;
}

//数据库释放页
int sqliterkPageRelease(sqliterk_page *page)
{
    if (!page) {
        return SQLITERK_MISUSE;
    }
    if (page->data) {
        sqliterkOSFree(page->data);
        page->data = NULL;
    }
    sqliterkOSFree(page);
    return SQLITERK_OK;
}

// Ahead release the page data to save memory
//释放页对象中保存的信息。
int sqliterkPageClearData(sqliterk_page *page)
{
    if (!page) {
        return SQLITERK_MISUSE;
    }
    if (page->data) {
        sqliterkOSFree(page->data);
        page->data = NULL;
    }
    return SQLITERK_OK;
}

//获取页中的数据
const unsigned char *sqliterkPageGetData(sqliterk_page *page)
{
    if (!page) {
        return NULL;
    }
    return page->data;
}

//获取数据库页大小
int sqliterkPagerGetSize(sqliterk_pager *pager)
{
    if (!pager) {
        return 0;
    }
    return pager->pagesize;
}

//获取数据库中页实际能大小(页大小减去保留字段)
int sqliterkPagerGetUsableSize(sqliterk_pager *pager)
{
    if (!pager) {
        return 0;
    }
    return pager->usableSize;
}

//返回页的页码
int sqliterkPageGetPageno(sqliterk_page *page)
{
    if (!page) {
        return 0;
    }
    return page->pageno;
}

//返回页的类型
sqliterk_page_type sqliterkPageGetType(sqliterk_page *page)
{
    if (!page) {
        return sqliterk_page_type_unknown;
    }
    return page->type;
}

//返回页数据的偏移。数据库第一页的前100个字符是对数据库进行描述的头
//http://blog.csdn.net/zzmgood/article/details/48209977
int sqliterkPagenoHeaderOffset(int pageno)
{
    if (pageno == 1) {
        return 100;
    }
    return 0;
}

//返回页数据的偏移
int sqliterkPageHeaderOffset(sqliterk_page *page)
{
    if (!page) {
        return 0;
    }
    return sqliterkPagenoHeaderOffset(page->pageno);
}

//返回页类型的描述字符串
const char *sqliterkPageGetTypeName(sqliterk_page_type type)
{
    char *name;
    switch (type) {
        case sqliterk_page_type_interior_index:
            name = "interior-index btree";
            break;
        case sqliterk_page_type_interior_table:
            name = "interior-table btree";
            break;
        case sqliterk_page_type_leaf_index:
            name = "leaf-index btree";
            break;
        case sqliterk_page_type_leaf_table:
            name = "leaf-table btree";
            break;
        default:
            name = "unknown page";
            break;
    }
    return name;
}

//设置指定页码的数据状态sqliterk_status
void sqliterkPagerSetStatus(sqliterk_pager *pager,
                            int pageno,
                            sqliterk_status status)
{
    if (!pager || !pager->pagesStatus ||
        sqliterkPagerIsPagenoValid(pager, pageno) != SQLITERK_OK) {
        return;
    }

    pager->pagesStatus[pageno - 1] = status;
    if (status == sqliterk_status_checked)
        pager->integrity |= SQLITERK_INTEGRITY_DATA;
}

//获取指定页码的数据状态
sqliterk_status sqliterkPagerGetStatus(sqliterk_pager *pager, int pageno)
{
    if (!pager || !pager->pagesStatus ||
        sqliterkPagerIsPagenoValid(pager, pageno) != SQLITERK_OK) {
        return sqliterk_status_invalid;
    }
    return pager->pagesStatus[pageno - 1];
}

//获取数据库中确认完好的页的数量
int sqliterkPagerGetParsedPageCount(sqliterk_pager *pager)
{
    if (!pager || !pager->pagesStatus) {
        return 0;
    }

    int i, count = 0;
    for (i = 0; i < pager->pagecount; i++) {
        if (pager->pagesStatus[i] == sqliterk_status_checked) {
            count++;
        }
    }
    return count;
}

//获取数据库中有效页的数量
int sqliterkPagerGetValidPageCount(sqliterk_pager *pager)
{
    if (!pager) {
        return 0;
    }
    return pager->pagecount - pager->freepagecount;
}

//获取数据库完整性标识
unsigned int sqliterkPagerGetIntegrity(sqliterk_pager *pager)
{
    if (!pager) {
        return 0;
    }
    return pager->integrity;
}
