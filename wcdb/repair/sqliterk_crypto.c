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

#include "sqliterk_crypto.h"
#include "SQLiteRepairKit.h"
#include "sqliterk_os.h"
#include "sqliterk_pager.h"
#include <sqlite3.h>
#include <string.h>

// Declarations by SQLCipher.
//解密
#define CIPHER_DECRYPT 0
//加密
#define CIPHER_ENCRYPT 1

//读操作上下文
#define CIPHER_READ_CTX 0
//写操作上下文
#define CIPHER_WRITE_CTX 1
//读写操作上下文
#define CIPHER_READWRITE_CTX 2

/* Extensions defined in crypto_impl.c */
//编码上下文？
typedef struct codec_ctx codec_ctx;

/* Activation and initialization */
//加密对象被激活
void sqlcipher_activate();
//加密对象不再活跃
void sqlcipher_deactivate();
//初始化编码上下文
int sqlcipher_codec_ctx_init(
    codec_ctx **, void *, void *, void *, const void *, int);
//释放编码上下文
void sqlcipher_codec_ctx_free(codec_ctx **);
//得到编码key
int sqlcipher_codec_key_derive(codec_ctx *);
//拷贝编码key
int sqlcipher_codec_key_copy(codec_ctx *, int);

/* Page cipher implementation */
//页密码初始化
int sqlcipher_page_cipher(
    codec_ctx *, int, int, int, int, unsigned char *, unsigned char *);

/* Context setters & getters */
//void sqlcipher_codec_ctx_set_error(codec_ctx *, int);

//加密编码设置通过
int sqlcipher_codec_ctx_set_pass(codec_ctx *, const void *, int, int);
//加密编码获得钥匙
void sqlcipher_codec_get_keyspec(codec_ctx *, void **zKey, int *nKey);

//加密编码上下文设置页大小
int sqlcipher_codec_ctx_set_pagesize(codec_ctx *, int);
//加密编码上下文得到页大小
int sqlcipher_codec_ctx_get_pagesize(codec_ctx *);
//加密编码上下文得到预留位大小
int sqlcipher_codec_ctx_get_reservesize(codec_ctx *);

//加密编码设置默认页大小
void sqlcipher_set_default_pagesize(int page_size);
//加密编码得到默认页大小
int sqlcipher_get_default_pagesize();

//kdf : 基于SHA-256的单步密钥导出函数
//设置默认的秘钥导出迭代器
void sqlcipher_set_default_kdf_iter(int iter);
//获得默认的秘钥到处迭代器
int sqlcipher_get_default_kdf_iter();

//加密编码上下文设置秘钥导出迭代器
int sqlcipher_codec_ctx_set_kdf_iter(codec_ctx *, int, int);
//加密编码上下文得到秘钥导出迭代器
int sqlcipher_codec_ctx_get_kdf_iter(codec_ctx *ctx, int);

//加密编码上下文得到kdf salt变量
void *sqlcipher_codec_ctx_get_kdf_salt(codec_ctx *ctx);

//加密编码上下文设置快速秘钥导出迭代器
int sqlcipher_codec_ctx_set_fast_kdf_iter(codec_ctx *, int, int);
//加密编码上下文获得快速秘钥导出迭代器
int sqlcipher_codec_ctx_get_fast_kdf_iter(codec_ctx *, int);

//加密编码上下文设置加密密码
int sqlcipher_codec_ctx_set_cipher(codec_ctx *, const char *, int);
//加密编码上下文获得密码
const char *sqlcipher_codec_ctx_get_cipher(codec_ctx *ctx, int for_ctx);

//加密编码上下文获得数据
void *sqlcipher_codec_ctx_get_data(codec_ctx *);

//void sqlcipher_exportFunc(sqlite3_context *, int, sqlite3_value **);

//hmac:HMAC是密钥相关的哈希运算消息认证码，HMAC运算利用哈希算法，以一个密钥和一个消息为输入，生成一个消息摘要作为输出。
//设置hmac
void sqlcipher_set_default_use_hmac(int use);
//获得hmac
int sqlcipher_get_default_use_hmac();

//salt:加密作料。使加密难以被破解
//设置hmac salt掩码
void sqlcipher_set_hmac_salt_mask(unsigned char mask);
//获得hmac salt 掩码
unsigned char sqlcipher_get_hmac_salt_mask();

//设置加密编码上下文 hmac
int sqlcipher_codec_ctx_set_use_hmac(codec_ctx *ctx, int use);
//获取加密编码上线文 hmac
int sqlcipher_codec_ctx_get_use_hmac(codec_ctx *ctx, int for_ctx);

//加密编码上下文设置标志位
int sqlcipher_codec_ctx_set_flag(codec_ctx *ctx, unsigned int flag);
//加密编码上下文移除标志位
int sqlcipher_codec_ctx_unset_flag(codec_ctx *ctx, unsigned int flag);
//加密编码上下文获得标志位
int sqlcipher_codec_ctx_get_flag(codec_ctx *ctx,
                                 unsigned int flag,
                                 int for_ctx);

//加密编码或得密码提供者
const char *sqlcipher_codec_get_cipher_provider(codec_ctx *ctx);
//int sqlcipher_codec_ctx_migrate(codec_ctx *ctx);
//加密编码增加随机值
int sqlcipher_codec_add_random(codec_ctx *ctx, const char *data, int random_sz);
//加密编码配置
int sqlcipher_cipher_profile(sqlite3 *db, const char *destination);
//static void sqlcipher_profile_callback(void *file, const char *sql, sqlite3_uint64 run_time);
//static int sqlcipher_codec_get_store_pass(codec_ctx *ctx);
//static void sqlcipher_codec_get_pass(codec_ctx *ctx, void **zKey, int *nKey);
//static void sqlcipher_codec_set_store_pass(codec_ctx *ctx, int value);
//加密编码状态
int sqlcipher_codec_fips_status(codec_ctx *ctx);
//加密编码版本号
const char *sqlcipher_codec_get_provider_version(codec_ctx *ctx);

// sqlite3_file redirector
typedef struct {
    //数据库io方法
    const struct sqlite3_io_methods *pMethods;
    //数据库文件结构体
    sqliterk_file *fd;
    //秘钥导出佐料（加密时候加入的字符串）
    const unsigned char *kdf_salt;
} sqlite3_file_rkredir;    //数据库文件操作结构体

//读取数据库文件
int sqliterkRead(sqlite3_file *fd, void *data, int iAmt, sqlite3_int64 iOfst)
{
    sqlite3_file_rkredir *rkos = (sqlite3_file_rkredir *) fd;
    //如果有加密添加字符串，则拷贝到data中，并返回
    if (rkos->kdf_salt) {
        memcpy(data, rkos->kdf_salt, (iAmt > 16) ? 16 : iAmt);
        return SQLITE_OK;
    } else {
        //没有加密添加字符串，则读取数据库文件信息
        sqliterk_file *f = rkos->fd;
        size_t size = iAmt;
        return sqliterkOSRead(f, (off_t) iOfst, data, &size);
    }
}

//数据库加密设置加密信息
int sqliterkCryptoSetCipher(sqliterk_pager *pager,
                            sqliterk_file *fd,
                            const sqliterk_cipher_conf *conf)
{
    codec_ctx *codec = NULL;
    int rc;

    if (conf) {
        // Check arguments.
        if (!conf->key || conf->key_len <= 0)
            return SQLITERK_MISUSE;

        // SQLite library must be initialized before calling sqlcipher_activate(),
        // or it will cause a deadlock.
        //初始化数据库
        sqlite3_initialize();
        //激活加密管理者
        sqlcipher_activate();

        // XXX: fake BTree structure passed to sqlcipher_codec_ctx_init.
        // Member of such structure is assigned but never used by repair kit.
        int fake_db[8];

        //数据库文件操作结构体，初始化
        sqlite3_file_rkredir file;
        struct sqlite3_io_methods methods = {0};
        methods.xRead = sqliterkRead;
        file.pMethods = &methods;
        file.fd = fd;
        file.kdf_salt = conf->kdf_salt;

        // Initialize codec context.
        //初始化编码上下文
        rc = sqlcipher_codec_ctx_init(&codec, fake_db, NULL, &file, conf->key,
                                      conf->key_len);
        if (rc != SQLITE_OK)
            goto bail_sqlite_errstr;

        // Set cipher.
        //编码上下文设置加密管理者
        if (conf->cipher_name) {
            rc = sqlcipher_codec_ctx_set_cipher(codec, conf->cipher_name,
                                                CIPHER_READWRITE_CTX);
            if (rc != SQLITE_OK)
                goto bail_sqlite_errstr;
        }

        // Set page size.
        //设置页大小
        if (conf->page_size > 0) {
            rc = sqlcipher_codec_ctx_set_pagesize(codec, conf->page_size);
            if (rc != SQLITE_OK)
                goto bail_sqlite_errstr;
        }

        // Set HMAC usage.
        //设置
        if (conf->use_hmac >= 0) {
            rc = sqlcipher_codec_ctx_set_use_hmac(codec, conf->use_hmac);
            if (rc != SQLITE_OK)
                goto bail_sqlite_errstr;
        }

        // Set KDF Iteration.
        //设置导出秘钥迭代器
        if (conf->kdf_iter > 0) {
            rc = sqlcipher_codec_ctx_set_kdf_iter(codec, conf->kdf_iter,
                                                  CIPHER_READWRITE_CTX);
            if (rc != SQLITE_OK)
                goto bail;
        }

        // Update pager page size.
        int page_sz = sqlcipher_codec_ctx_get_pagesize(codec);
        int reserve_sz = sqlcipher_codec_ctx_get_reservesize(codec);

        //更新加密上下文页面大小和预留大小
        pager->pagesize = page_sz;
        pager->reservedBytes = reserve_sz;
    }

    //释放资源
    if (pager->codec) {
        sqlcipher_codec_ctx_free(&pager->codec);
        sqlcipher_deactivate();
    }

    pager->codec = codec;
    return SQLITERK_OK;

bail_sqlite_errstr:
    sqliterkOSError(SQLITERK_CANTOPEN,
                    "Failed to initialize cipher context: %s",
                    sqlite3_errstr(rc));
    rc = SQLITERK_CANTOPEN;
bail:
    if (codec)
        sqlcipher_codec_ctx_free(&codec);
    //使加密管理者不再活跃
    sqlcipher_deactivate();
    return rc;
}

//释放页面管理者的加密上下文
void sqliterkCryptoFreeCodec(sqliterk_pager *pager)
{
    if (!pager->codec)
        return;
    //释放加密上下文
    sqlcipher_codec_ctx_free(&pager->codec);
    //使加密管理者不再活跃
    sqlcipher_deactivate();
}

//解密
int sqliterkCryptoDecode(sqliterk_codec *codec, int pgno, void *data)
{
    int rc;
    int offset = 0;
    unsigned char *pdata = (unsigned char *) data;

    //获取加密上下文大小
    int page_sz = sqlcipher_codec_ctx_get_pagesize(codec);
    //获取加密上下文数据
    unsigned char *buffer =
        (unsigned char *) sqlcipher_codec_ctx_get_data(codec);

    //提取加密上下文秘钥
    rc = sqlcipher_codec_key_derive(codec);
    if (rc != SQLITE_OK)
        return rc;

    if (pgno == 1) {
        offset = 16; // FILE_HEADER_SZ
        memcpy(buffer, "SQLite format 3", 16);
    }
    //解密pdata数据到buffer中
    rc = sqlcipher_page_cipher(codec, CIPHER_READ_CTX, pgno, CIPHER_DECRYPT,
                               page_sz - offset, pdata + offset,
                               buffer + offset);
    if (rc != SQLITE_OK)
        goto bail;
    memcpy(pdata, buffer, page_sz);

    return SQLITERK_OK;

bail:
    sqliterkOSError(SQLITERK_DAMAGED, "Failed to decode page %d: %s", pgno,
                    sqlite3_errstr(rc));
    return rc;
}
