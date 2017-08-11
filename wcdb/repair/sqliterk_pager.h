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

#ifndef sqliterk_pager_h
#define sqliterk_pager_h

#include "sqliterk_crypto.h"
#include <stdio.h>


//Pager在连接中起着很重要的作用，因为它管理事务、锁、内存缓冲以及负责崩溃恢复(crash recovery)。
/*
 pager管理着三种页：
(1)已修改的页：包含被B-tree修改的记录，位于page cache中。
(2)未修改的页：包含没有被B-tree修改的记录。
(3)日志页：这是修改页以前的版本，日志页并不存储在page cache中，而是在B-tree修改页之前写入日志。
 */
// A legal state transfer is:
// unchecked --> checking
// checking --> invalid
// checking --> damaged
// checking --> checked
// checking --> discarded
// default status is unchecked
typedef enum {
    sqliterk_status_invalid = -1,   //无效
    sqliterk_status_unchecked = 0,   //未检测
    sqliterk_status_checking = 1,   //检测中
    sqliterk_status_damaged = 2,   //损毁
    sqliterk_status_discarded = 3,   //丢弃
    sqliterk_status_checked = 4,   //检测完成
} sqliterk_status;

// We are only concerned about the listed types. See https://www.sqlite.org/fileformat2.html#btree
typedef enum {
    sqliterk_page_type_interior_index = 2,  //内部索引
    sqliterk_page_type_interior_table = 5,  //内部表
    sqliterk_page_type_leaf_index = 10,     //叶索引
    sqliterk_page_type_leaf_table = 13,     //页表
    sqliterk_page_type_overflow = 1,        //溢出
    sqliterk_page_type_unknown = -1,        //不知道类型
} sqliterk_page_type;

// sqliterk_pager is the management class for all page in single db file
//sqliterk_pager 管理单一数据库所有页的结构体。数据库页管理者
typedef struct sqliterk_pager sqliterk_pager;
// sqliterk_page is single page in db file, including its page data. But sometime, page data will be ahead release to save memory.
//sqliterk_page 是数据库单一页的结构体，包含页的数据。为了减少内存，页的数据可能会提前释放
typedef struct sqliterk_page sqliterk_page;

//数据库文件结构体
typedef struct sqliterk_file sqliterk_file;
////数据库加密信息结构体
typedef struct sqliterk_cipher_conf sqliterk_cipher_conf;
//sqliterk_pager 管理单一数据库所有页的结构体
struct sqliterk_pager {
    //数据库文件
    sqliterk_file *file;
    //数据库状态
    sqliterk_status *pagesStatus;
    //页大小
    int pagesize;
    //未使用的页数
    int freepagecount;
    //页(尾部)保留的字节数
    int reservedBytes;
    //页数
    int pagecount;
    //页可用字节数
    int usableSize;         // pagesize-reservedBytes
    //完整性标识符
    unsigned int integrity; // integrity flags.

    //codec上下文（加密解密？）
    sqliterk_codec *codec; // Codec context, implemented in SQLCipher library.
};

//打开指定路径数据库
int sqliterkPagerOpen(const char *path,
                      const sqliterk_cipher_conf *cipher,
                      sqliterk_pager **pager);
//关闭数据库
int sqliterkPagerClose(sqliterk_pager *pager);
//读取总页数
int sqliterkPagerGetPageCount(sqliterk_pager *pager);
//验证指定页码是否存在
int sqliterkPagerIsPagenoValid(sqliterk_pager *pager, int pageno);
//返回数据库页大小
int sqliterkPagerGetSize(sqliterk_pager *pager);
//获取数据库中页实际能大小(页大小减去保留字段)
int sqliterkPagerGetUsableSize(sqliterk_pager *pager);

//设置指定页码数据状态
void sqliterkPagerSetStatus(sqliterk_pager *pager,
                            int pageno,
                            sqliterk_status status);
//获取指定页码的数据状态
sqliterk_status sqliterkPagerGetStatus(sqliterk_pager *pager, int pageno);
//获取数据库中确认完好的页的数量
int sqliterkPagerGetParsedPageCount(sqliterk_pager *pager);
//获取数据库中有效页的数量
int sqliterkPagerGetValidPageCount(sqliterk_pager *pager);
//获取数据库完整性标识
unsigned int sqliterkPagerGetIntegrity(sqliterk_pager *pager);

//从文件获取页的全部数据，并初始化页
int sqliterkPageAcquire(sqliterk_pager *pager,
                        int pageno,
                        sqliterk_page **page);
//从文件获取溢出页的全部数据
int sqliterkPageAcquireOverflow(sqliterk_pager *pager,
                                int pageno,
                                sqliterk_page **page);
//使用pager获取指定页的类型
int sqliterkPageAcquireType(sqliterk_pager *pager,
                            int pageno,
                            sqliterk_page_type *type);
//释放页对象中保存的信息。
int sqliterkPageClearData(sqliterk_page *page);
//释放页内存
int sqliterkPageRelease(sqliterk_page *page);

//获取指定页码的没有头信息偏移量
int sqliterkPagenoHeaderOffset(int pageno);
//获取指定页码有头信息的偏移量
int sqliterkPageHeaderOffset(sqliterk_page *page);

//获取页数据
const unsigned char *sqliterkPageGetData(sqliterk_page *page);

//获取指定页对象的页码
int sqliterkPageGetPageno(sqliterk_page *page);
//获取指定页对象的数据类型
sqliterk_page_type sqliterkPageGetType(sqliterk_page *page);

//获取指定类型的描述字符串
const char *sqliterkPageGetTypeName(sqliterk_page_type type);

#endif /* sqliterk_pager_h */
