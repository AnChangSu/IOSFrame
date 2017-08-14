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

#ifndef SQLiteRepairKit_h
#define SQLiteRepairKit_h

#ifdef __cplusplus
extern "C" {
#endif

//stdlib 头文件即standard library标准库头文件
// stdlib 头文件里包含了C、C++语言的最常用的系统函数
#include <stdlib.h>

//修复数据库结构体
typedef struct sqliterk sqliterk;
//修复数据库表，定义在哪？？
typedef struct sqliterk_table sqliterk_table;
//舒修复数据库列
typedef struct sqliterk_column sqliterk_column;
//修复数据库通知
typedef struct sqliterk_notify sqliterk_notify;
//数据库通知结构体
struct sqliterk_notify {
    //开始解析数据表回调函数
    void (*onBeginParseTable)(sqliterk *rk, sqliterk_table *table);
    // Only a column that make sense will trigger this callback,
    // which is the column in a non-system table or the "sqlite_master"
    // return SQLITERK_OK to tell sqliterk that you already know that
    // meaning of this column
    //开始解析数据表列回调函数
    int (*onParseColumn)(sqliterk *rk,
                         sqliterk_table *table,
                         sqliterk_column *column);
    //结束解析数据表回调函数
    void (*onEndParseTable)(sqliterk *rk, sqliterk_table *table);
};
//为指定的数据库注册通知
int sqliterk_register_notify(sqliterk *rk, sqliterk_notify notify);

//数据库加密信息结构体
typedef struct sqliterk_cipher_conf {
    //秘钥
    const void *key;
    //秘钥长度
    int key_len;
    //加密管理者名字
    const char *cipher_name;
    //页大小
    int page_size;
    //秘钥导出函数迭代器
    int kdf_iter;
    //HMAC是密钥相关的哈希运算消息认证码，HMAC运算利用哈希算法，以一个密钥和一个消息为输入，生成一个消息摘要作为输出。

    int use_hmac;
    //KDF导出添加佐料(加密添加字符串)
    const unsigned char *kdf_salt;
} sqliterk_cipher_conf;

//数据库对象
typedef struct sqlite3 sqlite3;
//修复数据库管理者信息类
typedef struct sqliterk_master_info sqliterk_master_info;

//数据库不生成表单
#define SQLITERK_OUTPUT_NO_CREATE_TABLES 0x0001
//输出所有表
#define SQLITERK_OUTPUT_ALL_TABLES 0x0002

//打开修复数据库。传入路径，密钥信息，数据无
int sqliterk_open(const char *path,
                  const sqliterk_cipher_conf *cipher,
                  sqliterk **rk);
//解析数据库
int sqliterk_parse(sqliterk *rk);
//传入数据库和页码，解析数据库指定的页
//数据库中一个io操作的基本单位叫做页
int sqliterk_parse_page(sqliterk *rk, int pageno);
//解析数据库管理者信息
int sqliterk_parse_master(sqliterk *rk);
//关闭数据库
int sqliterk_close(sqliterk *rk);
//获取数据库用户信息
void *sqliterk_get_user_info(sqliterk *rk);
//设置数据库用户信息
void sqliterk_set_user_info(sqliterk *rk, void *userInfo);
//设置是否可以使用递归sql.
//recursive SQL是执行一条SQL语句时，产生的对其他SQL语句的调用，主要是对数据字典的查询，通常发生在第一次执行时，第二次执行一般可显著降低。另外，存储过程、触发器内如果有SQL调用的话，也会产生recursive SQL。
void sqliterk_set_recursive(sqliterk *rk, int recursive);

//读取数据库信息?
int sqliterk_output(sqliterk *rk,
                    sqlite3 *db,
                    sqliterk_master_info *master,
                    unsigned int flags);
//为数据库配置管理者
int sqliterk_make_master(const char **tables,
                         int num_tables,
                         sqliterk_master_info **out_master);
//保存数据库管理者
int sqliterk_save_master(sqlite3 *db,
                         const char *path,
                         const void *key,
                         int key_len);
//读取数据库管理者
int sqliterk_load_master(const char *path,
                         const void *key,
                         int key_len,
                         const char **tables,
                         int num_tables,
                         sqliterk_master_info **out_master,
                         unsigned char *out_kdf_salt);
//释放数据库管理者
void sqliterk_free_master(sqliterk_master_info *master);

// A database may have many kind of tables or indexes, such as a customized
// index or a system-level table and so on. But you should be only concern
// about the listed types below.
// Since the system-level tables or indexes is generated. And you do know
// the index of a certain table (you make this table).
//数据库可能会有很多的表或类型。不过你可能只需要设计下面列表的类型
typedef enum {
    sqliterk_type_index = -2,
    sqliterk_type_table = -1,
    sqliterk_type_unknown = 0,
    sqliterk_type_sequence = 1,
    sqliterk_type_autoindex = 2,
    sqliterk_type_stat = 3,
    sqliterk_type_master = 4,
} sqliterk_type;

// This method may return NULL since SQLiteRepairKir may not understand
// a corrupted b-tree.
//当b-tree损坏时，这个方法返回NULL
const char *sqliterk_table_name(sqliterk_table *table);
//返回数据库表类型
sqliterk_type sqliterk_table_type(sqliterk_table *table);
//获取表的根节点
int sqliterk_table_root(sqliterk_table *table);
//设置表的用户信息
void sqliterk_table_set_user_info(sqliterk_table *table, void *userInfo);
//获取表的用户信息
void *sqliterk_table_get_user_info(sqliterk_table *table);

//数据库表的数据类型
typedef enum {
    sqliterk_value_type_null,
    sqliterk_value_type_integer,
    sqliterk_value_type_number,
    sqliterk_value_type_text,
    sqliterk_value_type_binary,
} sqliterk_value_type;

//返回当前列数据数量
int sqliterk_column_count(sqliterk_column *column);
//返回指定列数据类型
sqliterk_value_type sqliterk_column_type(sqliterk_column *column, int index);
//读取整数列信息
int sqliterk_column_integer(sqliterk_column *column, int index);
//读取64位整数列信息
int64_t sqliterk_column_integer64(sqliterk_column *column, int index);
//读取number列信息
double sqliterk_column_number(sqliterk_column *column, int index);
//读取text列信息
const char *sqliterk_column_text(sqliterk_column *column, int index);
//读取二进制块列信息
const void *sqliterk_column_binary(sqliterk_column *column, int index);
//读取字符列信息
int sqliterk_column_bytes(sqliterk_column *column, int index);
//返回列id
int sqliterk_column_rowid(sqliterk_column *column);

//数据库有完整的头
#define SQLITERK_INTEGRITY_HEADER 0x0001
//数据库有完整的数据
#define SQLITERK_INTEGRITY_DATA 0x0002
//数据库有完整的kdf_salt
#define SQLITERK_INTEGRITY_KDF_SALT 0x0004

//解析的页的数量
int sqliterk_parsed_page_count(sqliterk *rk);
//验证的页的数量
int sqliterk_valid_page_count(sqliterk *rk);
//数据库页的数量
int sqliterk_page_count(sqliterk *rk);
//数据库完整性验证
unsigned int sqliterk_integrity(sqliterk *rk);

//日志等级
typedef enum {
    sqliterk_loglevel_debug,
    sqliterk_loglevel_warning,
    sqliterk_loglevel_error,
    sqliterk_loglevel_info,
} sqliterk_loglevel;

//数据库常用工具结构体
typedef struct sqliterk_os sqliterk_os;

struct sqliterk_os {
    //log回调函数
    void (*xLog)(sqliterk_loglevel level, int result, const char *msg);
    //TODO
};
//注册工具结构体
int sqliterk_register(sqliterk_os os);

//成功
#define SQLITERK_OK 0
//不能打开
#define SQLITERK_CANTOPEN 1
//数据库使用错误
#define SQLITERK_MISUSE 2
//数据库io错误
#define SQLITERK_IOERR 3
//数据库内存不足
#define SQLITERK_NOMEM 4
//数据库读取短信息
#define SQLITERK_SHORT_READ 5
//数据库损坏
#define SQLITERK_DAMAGED 6
//数据库被丢弃
#define SQLITERK_DISCARD 7
const char *sqliterk_description(int result);

#ifndef SQLITRK_CONFIG_DEFAULT_PAGESIZE
//数据库默认的页大小
#define SQLITRK_CONFIG_DEFAULT_PAGESIZE 4096
#endif

#ifdef __cplusplus
}
#endif

#endif /* SQLiteRepairKit_h */
