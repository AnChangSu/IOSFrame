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

#ifndef sqliterk_btree_h
#define sqliterk_btree_h


// About sqliterk_btree, see https://www.sqlite.org/fileformat2.html#btree
//B-tree的主要功能就是索引，它维护着各个页面之间的复杂的关系，便于快速找到所需数据。
/*
 B-Tree使得VDBE可以在O(logN)下查询，插入和删除数据，以及O(1)下双向遍历结果集。B-Tree不会直接读写磁盘，它仅仅维护着页面 (pages)之间的关系。当B-TREE需要页面或者修改页面时，它就会调用Pager。当修改页面时，pager保证原始页面首先写入日志文件，当它完成写操作时，pager根据事务状态决定如何做。B-tree不直接读写文件，而是通过page cache这个缓冲模块读写文件对于性能是有重要意义的。
 */
//SQLite中使用了两种B-tree。其中一种是b+tree，将所用的数据存储在叶子节点，在SQLite中被称为table b-tree。另一种是最初未经修改的b-tree，枝干节点和叶子节点都含有关键字和数据，在SQLite中被称为index b-tree。
//数据库b-tree结构体
typedef struct sqliterk_btree sqliterk_btree;
//sqliterk_pager 管理单一数据库所有页的结构体
typedef struct sqliterk_pager sqliterk_pager;
//定义values结构体，包含数组大小，容量和value数组
//数据队列
typedef struct sqliterk_values sqliterk_values;
//数据库列结构体
typedef struct sqliterk_column sqliterk_column;
//sqliterk_page 是数据库单一页的结构体，包含页的数据。为了减少内存，页的数据可能会提前释放
typedef struct sqliterk_page sqliterk_page;
//数据库结构体
typedef struct sqliterk sqliterk;
//数据库btree通知结构体
typedef struct sqliterk_btree_notify sqliterk_btree_notify;

//b-tree(二叉树)类型
typedef enum {
    //索引类型
    sqliterk_btree_type_index = -2,
    //表格类型
    sqliterk_btree_type_table = -1,
    //未知类型
    sqliterk_btree_type_unknown = 0,
    // About SQLite reserved btree, see [Storage Of The SQL Database Schema]
    // chapter at https://www.sqlite.org/fileformat2.html#Schema
    //系统开始
    sqliterk_btree_type_system_begin = 1,
    //序列类型
    sqliterk_btree_type_sequence = 1,
    //编索引类型
    sqliterk_btree_type_autoindex = 2,
    //
    sqliterk_btree_type_stat = 3,
    //管理类型
    sqliterk_btree_type_master = 4,
    //系统结束
    sqliterk_btree_type_system_end = 5,
} sqliterk_btree_type;

//数据库btree通知结构体
struct sqliterk_btree_notify {
    //开始解析btree回调
    void (*onBeginParseBtree)(sqliterk *rk, sqliterk_btree *btree);
    //结束解析btree回调
    void (*onEndParseBtree)(sqliterk *rk, sqliterk_btree *btree, int result);
    //开始解析列回调
    void (*onParseColumn)(sqliterk *rk,
                          sqliterk_btree *btree,
                          sqliterk_page *page,
                          sqliterk_column *column);

    // return SQLITE_OK to continue parsing the page. All other return
    // value will skip the parsing phase of this page.
    //开始解析页回调。当返回SQLITE_OK时，继续解析页，返回其他值则会跳过当前页的解析
    int (*onBeginParsePage)(sqliterk *rk, sqliterk_btree *btree, int pageno);

    //结束解析页回调
    void (*onEndParsePage)(sqliterk *rk,
                           sqliterk_btree *btree,
                           int pageno,
                           int result);
};

//传入数据库rk，页管理者pager，根页编码，返回btree
//打开btree
int sqliterkBtreeOpen(sqliterk *rk,
                      sqliterk_pager *pager,
                      int rootPageno,
                      sqliterk_btree **btree);
//解析btree(二叉树)
int sqliterkBtreeParse(sqliterk_btree *btree);
//关闭btree(二叉树)
int sqliterkBtreeClose(sqliterk_btree *btree);

//设置btree（二叉树）name
int sqliterkBtreeSetMeta(sqliterk_btree *btree,
                         const char *name,
                         sqliterk_btree_type type);
//获取btree名字
const char *sqliterkBtreeGetName(sqliterk_btree *btree);
//获取btree类型
sqliterk_btree_type sqliterkBtreeGetType(sqliterk_btree *btree);
//判断btree是否系统类型
int sqliterkBtreeIsSystemType(sqliterk_btree_type type);
//返回btree的根页
sqliterk_page *sqliterkBtreeGetRootPage(sqliterk_btree *btree);

//设置btree通知
void sqliterkBtreeSetNotify(sqliterk_btree *btree,
                            sqliterk_btree_notify *notify);
//设置btree的userinfo
void sqliterkBtreeSetUserInfo(sqliterk_btree *btree, void *userInfo);
//获取btree的userinfo
void *sqliterkBtreeGetUserInfo(sqliterk_btree *btree);
//获取btree 类型的描述字符串
const char *sqliterkBtreeGetTypeName(sqliterk_btree_type type);

#endif /* sqliterk_btree_h */
