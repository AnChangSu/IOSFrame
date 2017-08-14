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

//修复数据库基础定义
#include "SQLiteRepairKit.h"
//该头文件的主要目的就是提供一个assert的宏定义
#include <assert.h>
//定义了通过错误码来回报错误资讯的宏
#include <errno.h>
//??像是用c++模板定义了map和map相关操作
#include <map>
//sqlite3数据库头文件
#include <sqlite3.h>
//C99中，<stdint.h>中定义了几种扩展的整数类型和宏
#include <stdint.h>
//stdio .h 头文件定义了三个变量类型、一些宏和各种函数来执行输入和输出
#include <stdio.h>
//stdlib.h里面定义了五种类型、一些宏和通用工具函数
#include <stdlib.h>
//简单介绍C语言里面关于字符数组的函数定义的头文件，常用函数有strlen、strcmp、strcpy等等
#include <string.h>
//??像是用c++模板定义string相关的操作
#include <string>
//??像是用c++模板定义vector相关的操作
#include <vector>
//用于数据压缩
#include <zlib.h>
#if defined(__APPLE__)
//加密库
#include <CommonCrypto/CommonCrypto.h>
/*
 Security.framework中的基本数据类型
 SecCertificateRef-X.509证书 .cer或者.der文件
 SecIdentityRef-同时包含私钥与公钥证书信息 .p12文件或者.pfx文件
 SecKeyRef-代表一个加密密钥，私钥或者公钥
 SecTrustRef-X.509证书策略
 SecPolicyRef-X.509证书信任服务
 SecAccessControlRef-某个对象的访问控制权限
 */
//首先，从非对称加密算法开始，在开发的过程中引用以下头文件就足够了
#include <Security/Security.h>
#else
#include <openssl/rc4.h>
#endif

extern "C" {
#include "sqliterk_os.h"
}

//加密上下文类
class CipherContext {
public:
    enum Op {
        Encrypt,    //加密
        Decrypt,    //解密
    };
    CipherContext(Op op)
#if defined(__APPLE__)
        : m_op(op), m_key(nullptr), m_keyLength(0), m_cryptor(nullptr)
#endif
    {
    }

    //设置key 和 key的长度
    void setKey(const void *key, unsigned int keyLength)
    {
#if defined(__APPLE__)
        m_keyLength = keyLength;
        //重新分配内存 如果新的大小大于原内存大小，则新分配部分不会被初始化；如果新的大小小于原内存大小，可能会导致数据丢失
        m_key = (unsigned char *) realloc(m_key, m_keyLength);
        //拷贝key的内容到m_key
        memcpy(m_key, key, m_keyLength);
#else
        RC4_set_key(&m_rc4Key, keyLength, (const unsigned char *) key);
#endif
    }

    //加密方法
    void cipher(unsigned char *data, unsigned int length)
    {
#if defined(__APPLE__)
        if (!m_cryptor) {
            //对称加密,rc4加密算法，传入Key,key长度，得到m_cryptor(CCCryptorRef)
            //生成加密上下文
            CCCryptorCreate(kCCEncrypt, kCCAlgorithmRC4, 0, m_key, m_keyLength,
                            nullptr, &m_cryptor);
        }

        size_t cryptBytes = 0;
        //执行加密过程
        CCCryptorUpdate(m_cryptor, data, length, data, length, &cryptBytes);
        //结束加密过程，并得到加密数据
        CCCryptorFinal(m_cryptor, data, length, &cryptBytes);
#else
        RC4(&m_rc4Key, length, data, data);
#endif
    }

    ~CipherContext()
    {
#if defined(__APPLE__)
        if (m_cryptor) {
            //释放加密类的对象
            CCCryptorRelease(m_cryptor);
        }
        if (m_key) {
            //释放加密秘钥
            free(m_key);
        }
#endif
    }

protected:
#if defined(__APPLE__)
    //加密秘钥
    unsigned char *m_key;
    //秘钥长度
    unsigned int m_keyLength;
    Op m_op;
    //加密类的对象
    CCCryptorRef m_cryptor;
#else
    RC4_KEY m_rc4Key;
#endif
};

//数据库管理者实体
struct sqliterk_master_entity {
    //数据库类型或表类型
    sqliterk_type type;
    //??sql语句？
    std::string sql;
    //根页
    int root_page;

    //构造函数
    sqliterk_master_entity() {}

    //构造函数，初始化对象
    sqliterk_master_entity(sqliterk_type type_,
                           const char *sql_,
                           int root_page_)
        : type(type_), sql(sql_), root_page(root_page_)
    {
    }
};

//数据库管理者实体地图
typedef std::map<std::string, sqliterk_master_entity> sqliterk_master_map;
//数据库管理者信息
struct sqliterk_master_info : public sqliterk_master_map {
};

//数据库输出上下文
struct sqliterk_output_ctx {
    sqlite3 *db;
    //sqlite3_stmt对象是已经被编译成二进制的sql语句，已经准备执行
    sqlite3_stmt *stmt;
    //真实的列
    int real_columns;
    //sqlite3_value可以表示所有数据库表中的数据类型
    //定义一个sqlite3_value类型的动态数组,默认数据
    std::vector<sqlite3_value *> dflt_values;

    //数据库管理者实体地图
    sqliterk_master_map tables;
    //实体地图迭代器。const_iterator 定义的迭代器，迭代器指向的对象可以变更，但是迭代器指向对象的内容不能变更
    sqliterk_master_map::const_iterator table_cursor;
    //标志位
    unsigned int flags;

    //成功数量
    unsigned int success_count;
    //失败数量
    unsigned int fail_count;
};

//样本   开始解析表
static void dummy_onBeginParseTable(sqliterk *rk, sqliterk_table *table)
{
}

//样本   结束解析表
static void dummy_onEndParseTable(sqliterk *rk, sqliterk_table *table)
{
}

//管理者 解析列
static int master_onParseColumn(sqliterk *rk,
                                sqliterk_table *table,
                                sqliterk_column *column)
{
    //？rk的userinfo是void *类型数据。可以强转为sqliterk_output_ctx数据。
    //猜测userinfo存储的是一个包含sqliterk_output_ctx对象，并且sqliterk_output_ctx对象在第一个的结构体
    //实际是userinfo中传入的就是sqliterk_output_ctx对象
    sqliterk_output_ctx *ctx =
        (sqliterk_output_ctx *) sqliterk_get_user_info(rk);

    // For master table, check whether we should ignore, or create table
    // and prepare for insertion.
    //检测是否拥有管理者权限
    if (sqliterk_table_type(table) != sqliterk_type_master)
        return SQLITERK_MISUSE;

    //获取0-4列的信息
    const char *typestr = sqliterk_column_text(column, 0);
    const char *name = sqliterk_column_text(column, 1);
    const char *tbl_name = sqliterk_column_text(column, 2);
    int root_page = sqliterk_column_integer(column, 3);
    const char *sql = sqliterk_column_text(column, 4);
    sqliterk_type type;

    //如果typestr的字符串是table,则设置类型为table类型
    if (strcmp(typestr, "table") == 0)
        type = sqliterk_type_table;
    //如果typestr的字符串是index，则设置类型为index
    else if (strcmp(typestr, "index") == 0)
        type = sqliterk_type_index;
    //typestr是其他字符串，则返回数据库解析完成
    else
        return SQLITERK_OK;

    // TODO: deal with system tables.
    //如果处理的是系统表，则返回数据库解析完成
    if (strncmp(name, "sqlite_", 7) == 0)
        return SQLITERK_OK;

    if (ctx->flags & SQLITERK_OUTPUT_ALL_TABLES) {
        // Recover ALL: add table to the list.
        //恢复所有:将表加入列表。
        ctx->tables[name] = sqliterk_master_entity(type, sql, root_page);
    } else {
        // PARTIAL Recover: check whether we are interested, update info.
        //部分恢复:检查哪里需要修改，更新信息
        //迭代器，找到表的名字
        sqliterk_master_map::iterator it = ctx->tables.find(tbl_name);
        //如果找到了
        if (it != ctx->tables.end())
            //更新表内容
            ctx->tables[name] = sqliterk_master_entity(type, sql, root_page);
    }

    return SQLITERK_OK;
}

//看函数名字意思是插入完成，函数做的工作是清除数据。
//插入完成后清除上下文中需要插入的数据
static void fini_insert(sqliterk_output_ctx *ctx)
{
    if (ctx->stmt) {
        //sqlite3_finalize销毁前面被sqlite3_prepare创建的准备语句，每个准备语句都必须使用这个函数去销毁以防止内存泄露。
        sqlite3_finalize(ctx->stmt);
        ctx->stmt = NULL;
    }

    //获取动态数组大小
    int n = (int) ctx->dflt_values.size();
    //销毁队列中的所有数据
    for (int i = 0; i < n; i++)
        sqlite3_value_free(ctx->dflt_values[i]);
    //清空队列,删除队列中所有对象
    ctx->dflt_values.clear();
    //设置真实的列数为0
    ctx->real_columns = 0;
}

//为数据库更新准备sql语句和数据信息
static int init_insert(sqliterk_output_ctx *ctx, const std::string &table)
{
    std::string sql;
    //准备执行的二进制sql语句
    sqlite3_stmt *table_info_stmt;

    assert(ctx->stmt == NULL && ctx->dflt_values.empty());

    //sql预留足够空间，拼接sql
    sql.reserve(512);
    //PRAGMA table_info(table-name);返回表的基本信息
    sql = "PRAGMA table_info(";
    sql += table;
    sql += ");";
    //使用sqlite3_prepare_v2或相关的函数创建table_info_stmt这个对象
    int rc =
        sqlite3_prepare_v2(ctx->db, sql.c_str(), -1, &table_info_stmt, NULL);
    //创建sql语句失败，返回错误
    if (rc != SQLITE_OK) {
        sqliterkOSWarning(rc, "Failed to prepare SQL: %s [SQL: %s]",
                          sqlite3_errmsg(ctx->db), sql.c_str());
        //清空需要插入的数据
        fini_insert(ctx);
        return -1;
    }

    sql = "REPLACE INTO ";
    sql += table;
    sql += " VALUES(";
    ctx->real_columns = 0;
    //sqlite3_step() has another row ready
    //执行sql语句，全部执行完成后，返回SQLITE_DONE
    while (sqlite3_step(table_info_stmt) == SQLITE_ROW) {
        ctx->real_columns++;

        //读取列信息
        sqlite3_value *value = sqlite3_column_value(table_info_stmt, 4);
        //The sqlite3_value_dup(V) interface makes a copy of the sqlite3_value object D and returns a pointer to that copy
        //复制一个value对象，并返回复制对象的指针
        ctx->dflt_values.push_back(sqlite3_value_dup(value));

        sql += "?,";
    }
    //sqlite3_finalize销毁前面被sqlite3_prepare创建的准备语句，每个准备语句都必须使用这个函数去销毁以防止内存泄露。
    rc = sqlite3_finalize(table_info_stmt);
    //如果数据库错误，或要插入的列为0则返回错误，销毁队列
    if (rc != SQLITE_OK || ctx->real_columns == 0) {
        sqliterkOSWarning(
            rc, "Failed to execute SQL: %s [SQL: PRAGMA table_info(%s);]",
            sqlite3_errmsg(ctx->db), table.c_str());
        fini_insert(ctx);
        return -1;
    }

    //拼接sql语句
    sql[sql.length() - 1] = ')';
    sql += ';';

    sqlite3_stmt *stmt;
    //准备sql语句
    rc = sqlite3_prepare_v2(ctx->db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqliterkOSWarning(rc, "Failed to prepare SQL: %s [SQL: %s]",
                          sqlite3_errmsg(ctx->db), sql.c_str());
        fini_insert(ctx);
        return -1;
    }
    //设置准备好的sql语句
    ctx->stmt = stmt;

    return ctx->real_columns;
}

//解析数据表的列
static int table_onParseColumn(sqliterk *rk,
                               sqliterk_table *table,
                               sqliterk_column *column)
{
    //获取输出上下文
    sqliterk_output_ctx *ctx =
        (sqliterk_output_ctx *) sqliterk_get_user_info(rk);
    //获取列的数量
    int columns = sqliterk_column_count(column);
    //获取上下文的sql语句
    sqlite3_stmt *stmt = ctx->stmt;
    int rc;

    if (!stmt) {
        // Invalid table_cursor means failed statement compilation.
        //如果迭代器无效，则返回
        if (ctx->table_cursor == ctx->tables.end()) {
            ctx->fail_count++;
            return SQLITERK_OK;
        }

        //初始化ctx上下文第一个对象的插入sql
        rc = init_insert(ctx, ctx->table_cursor->first);
        //如果初始化期间数据库报错，则返回
        if (rc <= 0) {
            ctx->table_cursor = ctx->tables.end();
            ctx->fail_count++;
            return SQLITERK_OK;
        }

        // Begin transaction.
        char *errmsg;
        //执行sql语句
        //这样SQLite将把BEGIN后全部要执行的SQL语句先缓存在内存当中，然后等到COMMIT的时候一次性的写入数据库
        rc = sqlite3_exec(ctx->db, "BEGIN;", NULL, NULL, &errmsg);
        if (errmsg) {
            sqliterkOSWarning(rc, "Failed to begin transaction: %s", errmsg);
            sqlite3_free(errmsg);
        }

        stmt = ctx->stmt;
    }

    int i;

    // Bind values provided by the repair kit.
    for (i = 0; i < columns; i++) {
        //获取列数据的类型
        sqliterk_value_type type = sqliterk_column_type(column, i);
        switch (type) {
            case sqliterk_value_type_binary:
                //绑定一个斑点值到指定sql
                sqlite3_bind_blob(stmt, i + 1,
                                  sqliterk_column_binary(column, i),
                                  sqliterk_column_bytes(column, i), NULL);
                break;
            case sqliterk_value_type_integer:
                //绑定一个int64值到指定sql
                sqlite3_bind_int64(stmt, i + 1,
                                   sqliterk_column_integer64(column, i));
                break;
            case sqliterk_value_type_null:
                //绑定一个null值到指定sql
                sqlite3_bind_null(stmt, i + 1);
                break;
            case sqliterk_value_type_number:
                //绑定一个double值到指定sql
                sqlite3_bind_double(stmt, i + 1,
                                    sqliterk_column_number(column, i));
                break;
            case sqliterk_value_type_text:
                //绑定一个text值到指定sql
                sqlite3_bind_text(stmt, i + 1, sqliterk_column_text(column, i),
                                  sqliterk_column_bytes(column, i), NULL);
                break;
        }
    }

    // Use defaults for remaining values.
    //剩下的部分绑定默认values
    for (; i < ctx->real_columns; i++) {
        sqlite3_bind_value(stmt, i, ctx->dflt_values[i]);
    }

    //执行sql语句，全部执行完成后，返回SQLITE_DONE
    while (sqlite3_step(stmt) == SQLITE_ROW) {
    }
    //重置stmt,可以重新绑定参数重新执行
    //sqlite3_prepare_v2代价昂贵，尽量重用stmt
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        sqliterkOSWarning(rc, "Failed to execute SQL: %s [SQL: %s]",
                          sqlite3_errmsg(ctx->db), sqlite3_sql(stmt));
        ctx->fail_count++;
        return SQLITERK_OK;
    }

    ctx->success_count++;
    //如果成功加入的sql为256条正数倍，则执行sql
    if (ctx->success_count % 256 == 0) {
        char *errmsg;
        rc = sqlite3_exec(ctx->db, "COMMIT; BEGIN;", NULL, NULL, &errmsg);
        if (errmsg) {
            sqliterkOSWarning(rc, "Failed to commit transaction: %s", errmsg);
            sqlite3_free(errmsg);
        }
    }

    return SQLITERK_OK;
}

//数据库输出
int sqliterk_output(sqliterk *rk,
                    sqlite3 *db,
                    sqliterk_master_info *master_,
                    unsigned int flags)
{
    if (!rk || !db)
        return SQLITERK_MISUSE;

    //将master_强转成sqliterk_master_map类型
    sqliterk_master_map *master = static_cast<sqliterk_master_map *>(master_);
    //定义输出上下文，并初始化
    sqliterk_output_ctx ctx;
    ctx.db = db;
    ctx.stmt = NULL;
    ctx.flags = flags;
    ctx.success_count = 0;
    ctx.fail_count = 0;

    //如果master为NULL,则扩展标志位。不为NULL，则给tables赋值
    if (!master)
        ctx.flags |= SQLITERK_OUTPUT_ALL_TABLES;
    else
        ctx.tables = *master;

    //设置上下文为数据库的userinfo
    sqliterk_set_user_info(rk, &ctx);
    //定义通知
    sqliterk_notify notify;
    //设置开始解析表格通知
    notify.onBeginParseTable = dummy_onBeginParseTable;
    //设置结束解析表格通知
    notify.onEndParseTable = dummy_onEndParseTable;
    //开始解析列通知
    notify.onParseColumn = master_onParseColumn;
    //为数据库注册通知
    sqliterk_register_notify(rk, notify);
    //设置ecursive
    sqliterk_set_recursive(rk, 0);

    //根据数据库连接，返回数据库名字
    const char *db_file = sqlite3_db_filename(db, "main");
    sqliterkOSInfo(SQLITERK_OK, "Output recovered data to '%s', flags 0x%04x",
                   db_file ? db_file : "unknown", flags);

    // Parse sqlite_master for table info.
    sqliterkOSDebug(SQLITERK_OK, "Begin parsing sqlite_master...");
    //解析数据库页
    int rc = sqliterk_parse_page(rk, 1);
    if (rc != SQLITERK_OK)
        sqliterkOSWarning(rc, "Failed to parse sqlite_master.");
    else
        sqliterkOSInfo(rc, "Parsed sqlite_master. [table/index: %u]",
                       ctx.tables.size());

    // Parse all tables.
    //解析数据表的列
    notify.onParseColumn = table_onParseColumn;
    //为数据库注册通知
    sqliterk_register_notify(rk, notify);

    //遍历地图，对地图中所有表进行操作更新。
    sqliterk_master_map::iterator it;
    for (it = ctx.tables.begin(); it != ctx.tables.end(); ++it) {
        //迭代器first为key,second为value
        if (it->second.type != sqliterk_type_table)
            continue;

        // Run CREATE TABLE statements if necessary.
        //如果标志位不是不生成table,并且当前对象的sql不为空的话，则执行内容
        if (!(ctx.flags & SQLITERK_OUTPUT_NO_CREATE_TABLES) &&
            !it->second.sql.empty()) {
            sqliterkOSDebug(SQLITERK_OK, ">>> %s", it->second.sql.c_str());
            char *errmsg = NULL;
            const char *sql = it->second.sql.c_str();
            //执行对象的sql
            rc = sqlite3_exec(ctx.db, sql, NULL, NULL, &errmsg);
            //失败打印错误消息，释放资源。成功则success_count+1
            if (errmsg) {
                sqliterkOSWarning(rc, "EXEC FAILED: %s [SQL: %s]", errmsg, sql);
                ctx.fail_count++;
                sqlite3_free(errmsg);
            } else
                ctx.success_count++;
        }

        //如果有root_page
        if (it->second.root_page != 0) {
            const char *name = it->first.c_str();
            int root_page = it->second.root_page;
            sqliterkOSInfo(SQLITERK_OK, "[%s] -> pgno: %d", name, root_page);
            ctx.table_cursor = it;
            //解析root page
            rc = sqliterk_parse_page(rk, root_page);
            if (rc != SQLITERK_OK)
                sqliterkOSWarning(rc,
                                  "Failed to parse B-tree with root page %d.",
                                  it->second.root_page);
            //如果有待执行的sql语句
            if (ctx.stmt) {
                // Commit transaction and free statement.
                char *errmsg;
                //执行sql语句
                rc = sqlite3_exec(ctx.db, "COMMIT;", NULL, NULL, &errmsg);
                if (errmsg) {
                    sqliterkOSWarning(rc, "Failed to commit transaction: %s",
                                      errmsg);
                    sqlite3_free(errmsg);
                }

                ////插入完成后清除上下文中需要插入的数据
                fini_insert(&ctx);
            }
        }
    }

    // Iterate through indices, create them if necessary.
    //如果标志位不是数据库不允许生成表单
    if (!(ctx.flags & SQLITERK_OUTPUT_NO_CREATE_TABLES)) {
        //遍历地图，对地图中多有type_index类型操作
        for (it = ctx.tables.begin(); it != ctx.tables.end(); ++it) {
            if (it->second.type != sqliterk_type_index)
                continue;

            const char *sql = it->second.sql.c_str();
            sqliterkOSDebug(SQLITERK_OK, ">>> %s", sql);
            char *errmsg = NULL;
            //执行sql语句(COMMIT时才会被真正执行)
            rc = sqlite3_exec(ctx.db, sql, NULL, NULL, &errmsg);
            if (errmsg) {
                sqliterkOSWarning(rc, "EXEC FAILED: %s [SQL: %s]", errmsg, sql);
                ctx.fail_count++;
                sqlite3_free(errmsg);
            } else
                ctx.success_count++;
        }
    }

    // Return OK only if we had successfully output at least one row.
    //只要有一条成功，就返回成功
    rc = SQLITERK_OK;
    if (ctx.success_count == 0) {
        rc = SQLITERK_DAMAGED;
        if (ctx.tables.empty())
            sqliterkOSError(rc, "No vaild sqlite_master info available, "
                                "sqlite_master is corrupted.");
        else
            sqliterkOSError(rc,
                            "No rows can be successfully output. [failed: %u]",
                            ctx.fail_count);
    } else
        sqliterkOSInfo(rc,
                       "Recovery output finished. [succeeded: %u, failed: %u]",
                       ctx.success_count, ctx.fail_count);
    return rc;
}

//生成新的sqliterk_master_info并初始化sqliterk_master_map
int sqliterk_make_master(const char **tables,
                         int num_tables,
                         sqliterk_master_info **out_master)
{
    if (!tables || !num_tables) {
        *out_master = NULL;
        return SQLITERK_OK;
    }

    //申请内存
    sqliterk_master_map *master = new sqliterk_master_map();
    //初始化
    for (int i = 0; i < num_tables; i++)
        (*master)[tables[i]] =
            sqliterk_master_entity(sqliterk_type_unknown, "", 0);

    *out_master = static_cast<sqliterk_master_info *>(master);
    return SQLITERK_OK;
}

#define SQLITERK_SM_TYPE_TABLE 0x01
#define SQLITERK_SM_TYPE_INDEX 0x02

#define SQLITERK_SM_MAGIC "\0dBmSt"
#define SQLITERK_SM_VERSION 1

//设置内存中的对齐方式
//是指把原来对齐方式设置压栈，并设新的对齐方式设置为一个字节对齐
#pragma pack(push, 1)
//管理者文件头信息
struct master_file_header {
    char magic[6];
    uint16_t version;
    uint32_t entities;
    unsigned char kdf_salt[16];
};

//管理者文件实体信息
struct master_file_entity {
    uint32_t root_page;
    uint8_t type;
    uint8_t name_len;
    uint8_t tbl_name_len;
    uint8_t reserved;
    uint16_t sql_len;

    unsigned char data[0];
};
#pragma pack(pop)

//数据库保存管理者
int sqliterk_save_master(sqlite3 *db,
                         const char *path,
                         const void *key,
                         int key_len)
{
    FILE *fp = NULL;
    int rc = SQLITERK_OK;
    //使用deflate进行压缩时，先要定义z_stream结构体，
    sqlite3_stmt *stmt = NULL;
    z_stream zstrm = {0};
    //加密上下文
    CipherContext cipherContext(CipherContext::Encrypt);
    unsigned char in_buf[512 + 8];
    unsigned char out_buf[2048];
    uint32_t entities = 0;
    master_file_header header;

    // Prepare deflate stream.
    //初始化zlib状态
    //deflateInit()有两个参数，一个是要初始化的结构的指针（strm），一个是压缩的等级（level）。level的值在-1~9之间。压缩等级越低，执行速度越快，压缩率越低。常量Z_DEFAULT_COMPRESSION（等于-1）表示在压缩率和速度方面寻求平衡，实际上和等级6一样。
    rc = deflateInit(&zstrm, Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK)
        goto bail_zlib;
    zstrm.data_type = Z_TEXT;

    // Open output file.
    //打开文件
    fp = fopen(path, "wb");
    if (!fp)
        goto bail_errno;

    // Prepare cipher key.
    //设置加密的key
    if (key && key_len) {
        cipherContext.setKey(key, key_len);
    } else {
        key = NULL;
    }

    // Prepare SQL statement.
    //准备sql语句
    rc =
        sqlite3_prepare_v2(db, "SELECT * FROM sqlite_master;", -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        goto bail_sqlite;

    // Write dummy header.
    //初始化header
    memset(&header, 0, sizeof(header));
    //将文件写入header
    if (fwrite(&header, sizeof(header), 1, fp) != 1)
        goto bail_errno;

    // Read all rows.
    //执行sql语句
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        //读取数据
        const char *typestr = (const char *) sqlite3_column_text(stmt, 0);
        int name_len = sqlite3_column_bytes(stmt, 1);
        const char *name = (const char *) sqlite3_column_text(stmt, 1);
        int tbl_name_len = sqlite3_column_bytes(stmt, 2);
        const char *tbl_name = (const char *) sqlite3_column_text(stmt, 2);
        int root_page = sqlite3_column_int(stmt, 3);
        int sql_len = sqlite3_column_bytes(stmt, 4);
        const char *sql = (const char *) sqlite3_column_text(stmt, 4);

        // Skip system tables and indices.
        //跳过系统表和数据
        if (strncmp(name, "sqlite_", 7) == 0)
            continue;

        unsigned char type;
        //根据typestr内容，设置type
        if (strcmp(typestr, "table") == 0)
            type = SQLITERK_SM_TYPE_TABLE;
        else if (strcmp(typestr, "index") == 0)
            type = SQLITERK_SM_TYPE_INDEX;
        else
            continue;

        //判断数据库名长度，表格名长度和sql语句长度，超长返回错误
        if (name_len > 255 || tbl_name_len > 255 || sql_len > 65535) {
            sqliterkOSError(SQLITERK_IOERR,
                            "Table/index has name longer than 255: %s, %s",
                            name, tbl_name);
            goto bail;
        }

        // Write to zip-stream buffer.
        //设置管理者文件实体
        master_file_entity *entity = (master_file_entity *) in_buf;
        entity->root_page = root_page;
        entity->type = type;
        entity->name_len = (unsigned char) name_len;
        entity->tbl_name_len = (unsigned char) tbl_name_len;
        entity->reserved = 0;
        entity->sql_len = (unsigned short) sql_len;
        unsigned char *p_data = entity->data;
        memcpy(p_data, name, name_len + 1);
        p_data += name_len + 1;
        memcpy(p_data, tbl_name, tbl_name_len + 1);
        p_data += tbl_name_len + 1;

        //下次输入的字节
        zstrm.next_in = in_buf;
        //可再next_in输入的字节数
        zstrm.avail_in = (uInt)(p_data - in_buf);

        do {
            //下一个输出字符
            zstrm.next_out = out_buf;
            //剩余的输出字符空间
            zstrm.avail_out = sizeof(out_buf);
            //deflate压缩算法，
            rc = deflate(&zstrm, Z_NO_FLUSH);
            if (rc == Z_STREAM_ERROR)
                goto bail_zlib;

            unsigned have = sizeof(out_buf) - zstrm.avail_out;
            if (key) {
                //加密输出内容
                cipherContext.cipher(out_buf, have);
            }
            if (fwrite(out_buf, 1, have, fp) != have) {
                sqliterkOSError(SQLITERK_IOERR,
                                "Cannot write to backup file: %s",
                                strerror(errno));
                goto bail;
            }
        } while (zstrm.avail_out == 0);

        zstrm.next_in = (unsigned char *) sql;
        zstrm.avail_in = sql_len + 1;

        do {
            zstrm.next_out = out_buf;
            zstrm.avail_out = sizeof(out_buf);
            rc = deflate(&zstrm, Z_NO_FLUSH);
            if (rc == Z_STREAM_ERROR)
                goto bail_zlib;

            unsigned have = sizeof(out_buf) - zstrm.avail_out;
            if (key) {
                cipherContext.cipher(out_buf, have);
            }
            //将out_buf写入fp文件
            if (fwrite(out_buf, 1, have, fp) != have) {
                sqliterkOSError(SQLITERK_IOERR,
                                "Cannot write to backup file: %s",
                                strerror(errno));
                goto bail;
            }
            //如果还有待输出项，执行循环
        } while (zstrm.avail_out == 0);

        entities++;
    } // sqlite3_step
    //释放sql语句
    rc = sqlite3_finalize(stmt);
    stmt = NULL;
    if (rc != SQLITE_OK)
        goto bail_sqlite;

    // Flush Z-stream.
    zstrm.avail_in = 0;
    do {
        zstrm.next_out = out_buf;
        zstrm.avail_out = sizeof(out_buf);
        //结束压缩
        rc = deflate(&zstrm, Z_FINISH);
        if (rc == Z_STREAM_ERROR)
            goto bail_zlib;

        unsigned have = sizeof(out_buf) - zstrm.avail_out;
        if (key) {
            cipherContext.cipher(out_buf, have);
        }
        //将out_buf写入fp文件
        if (fwrite(out_buf, 1, have, fp) != have) {
            sqliterkOSError(SQLITERK_IOERR, "Cannot write to backup file: %s",
                            strerror(errno));
            goto bail;
        }
        //如果还有待输出项，执行循环
    } while (zstrm.avail_out == 0);
    //关闭压缩
    deflateEnd(&zstrm);

    // Read KDF salt from file header.
    sqlite3_file *db_file;
    rc = sqlite3_file_control(db, "main", SQLITE_FCNTL_FILE_POINTER, &db_file);
    if (rc != SQLITE_OK)
        goto bail_sqlite;

    //读取kdf_salt
    rc = db_file->pMethods->xRead(db_file, header.kdf_salt,
                                  sizeof(header.kdf_salt), 0);
    if (rc != SQLITE_OK)
        goto bail_sqlite;

    // Write real header.
    //初始化header
    memcpy(header.magic, SQLITERK_SM_MAGIC, sizeof(header.magic));
    header.version = SQLITERK_SM_VERSION;
    header.entities = entities;
    fseek(fp, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, fp);
    fclose(fp);

    sqliterkOSInfo(SQLITERK_OK, "Saved master info with %u entries.", entities);
    return SQLITERK_OK;

bail_zlib:
    sqliterkOSError(SQLITERK_CANTOPEN, "Failed to backup master table: %s",
                    zstrm.msg);
    goto bail;
bail_errno:
    sqliterkOSError(rc, "Failed to backup master table: %s", strerror(errno));
    goto bail;
bail_sqlite:
    sqliterkOSError(rc, "Failed to backup master table: %s",
                    sqlite3_errmsg(db));

bail:
    if (fp)
        fclose(fp);
    if (stmt)
        sqlite3_finalize(stmt);
    deflateEnd(&zstrm);
    return rc;
}

static const size_t IN_BUF_SIZE = 4096;
static int inflate_read(FILE *fp,
                        z_streamp strm,
                        void *buf,
                        unsigned size,
                        CipherContext *cipherContext)
{
    int ret;
    if (size == 0)
        return SQLITERK_OK;

    //下一个输出的字节
    strm->next_out = (unsigned char *) buf;
    //剩余要输出的字节数
    strm->avail_out = size;

    do {
        //feof检测流上的文件结束符，如果文件结束，则返回非0值，否则返回0
        if (strm->avail_in == 0 && !feof(fp)) {
            unsigned char *in_buf = strm->next_in - strm->total_in;
            strm->total_in = 0;

            //读取文件到in_buf
            ret = (int) fread(in_buf, 1, IN_BUF_SIZE, fp);
            if (ret == 0 && ferror(fp))
                return SQLITERK_IOERR;
            if (ret > 0) {
                if (cipherContext) {
                    //加密
                    cipherContext->cipher(in_buf, ret);
                }
                strm->next_in = in_buf;
                strm->avail_in = ret;
            }
        }

        //解压缩
        ret = inflate(strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END)
            return SQLITERK_DAMAGED;

    } while (strm->avail_out > 0 && ret != Z_STREAM_END);

    //剩余输出大于0则返回SQLITERK_DAMAGED，否则返回SQLITERK_OK
    return strm->avail_out ? SQLITERK_DAMAGED : SQLITERK_OK;
}

//指针字符串比对
static int pstrcmp(const void *p1, const void *p2)
{
    return strcmp(*(char *const *) p1, *(char *const *) p2);
}

//数据库读取管理者
int sqliterk_load_master(const char *path,
                         const void *key,
                         int key_len,
                         const char **tables,
                         int num_tables,
                         sqliterk_master_info **out_master,
                         unsigned char *out_kdf_salt)
{
    FILE *fp = NULL;
    //加密流对象
    z_stream zstrm = {0};
    //加密上下文
    CipherContext cipherContext(CipherContext::Decrypt);
    int rc;
    unsigned char in_buf[IN_BUF_SIZE];
    const char **filter = NULL;
    sqliterk_master_map *master = NULL;
    uint32_t entities;

    // Allocate output buffer.
    char *str_buf = (char *) malloc(65536 + 512);
    if (!str_buf)
        goto bail_errno;

    // Sort table filter for later binary searching.
    if (tables && num_tables) {
        //创建Master,并初始化tables的sqliterk_master_map
        sqliterk_make_master(tables, num_tables,
                             (sqliterk_master_info **) &master);

        size_t filter_size = num_tables * sizeof(const char *);
        filter = (const char **) malloc(filter_size);
        if (!filter)
            goto bail_errno;

        //将tables拷贝到filter中
        memcpy(filter, tables, filter_size);
        //各参数：1 待排序数组首地址 2 数组中待排序元素数量 3 各元素的占用空间大小 4 指向函数的指针
        //根据pstrcmp函数返回对filter数组进行排序
        qsort(filter, num_tables, sizeof(const char *), pstrcmp);
    }
    if (!master)
        master = new sqliterk_master_map();

    //打开文件
    fp = fopen(path, "rb");
    if (!fp)
        goto bail_errno;

    // Read header.
    //从文件中读取头信息
    master_file_header header;
    if (fread(&header, sizeof(header), 1, fp) != 1)
        goto bail_errno;
    //初始化header
    if (memcmp(header.magic, SQLITERK_SM_MAGIC, sizeof(header.magic)) != 0 ||
        header.version != SQLITERK_SM_VERSION) {
        sqliterkOSError(SQLITERK_DAMAGED, "Invaild format: %s", path);
        goto bail;
    }

    // Initialize zlib and RC4.
    //解压初始化
    rc = inflateInit(&zstrm);
    if (rc != Z_OK)
        goto bail_zlib;

    if (key && key_len) {
        //解密
        cipherContext.setKey(key, key_len);
    } else {
        key = NULL;
    }

    // Read all entities.
    entities = header.entities;
    zstrm.next_in = in_buf;
    zstrm.avail_in = 0;
    while (entities--) {
        // Read entity header.
        //读取实例的头信息
        master_file_entity entity;
        rc = inflate_read(fp, &zstrm, &entity, sizeof(entity),
                          key ? &cipherContext : NULL);
        if (rc == SQLITERK_IOERR)
            goto bail_errno;
        else if (rc == SQLITERK_DAMAGED)
            goto bail_zlib;

        // Read names and SQL.
        //读取名字和sql
        rc = inflate_read(fp, &zstrm, str_buf,
                          entity.name_len + entity.tbl_name_len +
                              entity.sql_len + 3,
                          key ? &cipherContext : NULL);
        if (rc == SQLITERK_IOERR)
            goto bail_errno;
        else if (rc == SQLITERK_DAMAGED)
            goto bail_zlib;

        const char *name = str_buf;
        const char *tbl_name = name + entity.name_len + 1;
        const char *sql = tbl_name + entity.tbl_name_len + 1;
        if (name[entity.name_len] != '\0' ||
            tbl_name[entity.tbl_name_len] != '\0' ||
            sql[entity.sql_len] != '\0') {
            sqliterkOSError(SQLITERK_DAMAGED,
                            "Invalid string. File corrupted.");
            goto bail;
        }

        // Filter tables.
        //bsearch参数：第一个：要查找的关键字的指针。第二个：要查找的数组。第三个：被查找数组中的元素数量。第四个：每个元素的长度（以字节为单位）。第五个：指向比较函数的指针。
        //使用pstrcmp函数，查找过滤器中名字为tbl_name的表
        if (!filter || bsearch(&tbl_name, filter, num_tables,
                               sizeof(const char *), pstrcmp)) {
            //如果找到了tbl_name的表
            sqliterk_master_entity e(sqliterk_type_unknown, sql,
                                     entity.root_page);
            if (entity.type == SQLITERK_SM_TYPE_TABLE)
                e.type = sqliterk_type_table;
            else if (entity.type == SQLITERK_SM_TYPE_INDEX)
                e.type = sqliterk_type_index;

            //设置master map中key为name的项value为e
            (*master)[name] = e;
        }
    }

    //结束解压
    inflateEnd(&zstrm);
    free(str_buf);
    free(filter);
    fclose(fp);

    //将header中的kdf_salt拷贝到out_kdf_salt中
    if (out_kdf_salt)
        memcpy(out_kdf_salt, header.kdf_salt, sizeof(header.kdf_salt));
    *out_master = static_cast<sqliterk_master_info *>(master);
    sqliterkOSInfo(SQLITERK_OK, "Loaded master info with %u valid entries.",
                   master->size());
    return SQLITERK_OK;

bail_errno:
    sqliterkOSError(SQLITERK_IOERR, "Cannot load master table: %s",
                    strerror(errno));
    goto bail;
bail_zlib:
    sqliterkOSError(SQLITERK_DAMAGED, "Cannot load master table: %s",
                    zstrm.msg);
bail:
    if (master)
        delete master;
    free(str_buf);
    free(filter);
    inflateEnd(&zstrm);
    if (fp)
        fclose(fp);
    return SQLITERK_DAMAGED;
}

//释放master
void sqliterk_free_master(sqliterk_master_info *master)
{
    if (master)
        delete static_cast<sqliterk_master_map *>(master);
}
