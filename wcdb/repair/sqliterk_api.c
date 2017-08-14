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

//基础方法
#include "SQLiteRepairKit.h"
//数据库相关方法
#include "sqliterk.h"
//数据库二叉树相关方法
#include "sqliterk_btree.h"
//数据库列相关方法
#include "sqliterk_column.h"
//数据库基础功能
#include "sqliterk_os.h"
//数据库页信息及相关方法
#include "sqliterk_pager.h"
//常用工具方法
#include "sqliterk_util.h"
//数据库存储数值
#include "sqliterk_values.h"

//数据库注册通知
int sqliterk_register_notify(sqliterk *rk, sqliterk_notify notify)
{
    return sqliterkSetNotify(rk, notify);
}

//数据库打开
int sqliterk_open(const char *path,
                  const sqliterk_cipher_conf *cipher,
                  sqliterk **rk)
{
    return sqliterkOpen(path, cipher, rk);
}

//数据库获取用户信息
void *sqliterk_user_info(sqliterk *rk)
{
    return sqliterkGetUserInfo(rk);
}

//数据库解析
int sqliterk_parse(sqliterk *rk)
{
    return sqliterkParse(rk);
}

//数据库解析页
int sqliterk_parse_page(sqliterk *rk, int pageno)
{
    return sqliterkParsePage(rk, pageno);
}

//数据库解析master页
int sqliterk_parse_master(sqliterk *rk)
{
    return sqliterkParseMaster(rk);
}

//关闭数据库
int sqliterk_close(sqliterk *rk)
{
    return sqliterkClose(rk);
}

//数据库获取用户信息userinfo
void *sqliterk_get_user_info(sqliterk *rk)
{
    return sqliterkGetUserInfo(rk);
}

//数据库设置用户信息userinfo
void sqliterk_set_user_info(sqliterk *rk, void *userInfo)
{
    sqliterkSetUserInfo(rk, userInfo);
}

//获取数据库列数
int sqliterk_column_count(sqliterk_column *column)
{
    return sqliterkValuesGetCount(sqliterkColumnGetValues(column));
}

//获取数据库列类型
sqliterk_value_type sqliterk_column_type(sqliterk_column *column, int index)
{
    return sqliterkValuesGetType(sqliterkColumnGetValues(column), index);
}

//获取指定index的value的int数据
int sqliterk_column_integer(sqliterk_column *column, int index)
{
    return sqliterkValuesGetInteger(sqliterkColumnGetValues(column), index);
}

//获取sqliterk_column结构体重指定index的value的数据
int64_t sqliterk_column_integer64(sqliterk_column *column, int index)
{
    return sqliterkValuesGetInteger64(sqliterkColumnGetValues(column), index);
}

//获取指定index的sqliterk_column的double数据
double sqliterk_column_number(sqliterk_column *column, int index)
{
    return sqliterkValuesGetNumber(sqliterkColumnGetValues(column), index);
}

//获取指定index的sqliterk_column的char数据
const char *sqliterk_column_text(sqliterk_column *column, int index)
{
    return sqliterkValuesGetText(sqliterkColumnGetValues(column), index);
}

//获取指定index的sqliterk_column的binary数据
const void *sqliterk_column_binary(sqliterk_column *column, int index)
{
    return sqliterkValuesGetBinary(sqliterkColumnGetValues(column), index);
}

//获取指定index的sqliterk_column的数据大小
int sqliterk_column_bytes(sqliterk_column *column, int index)
{
    return sqliterkValuesGetBytes(sqliterkColumnGetValues(column), index);
}

//获取列对象的id
int sqliterk_column_rowid(sqliterk_column *column)
{
    return sqliterkColumnGetRowId(column);
}

//获取table所属btree名字
const char *sqliterk_table_name(sqliterk_table *table)
{
    return sqliterkBtreeGetName((sqliterk_btree *) table);
}

//设置table的userinfo
void sqliterk_table_set_user_info(sqliterk_table *table, void *userInfo)
{
    sqliterkBtreeSetUserInfo((sqliterk_btree *) table, userInfo);
}

//获取table的userinfo
void *sqliterk_table_get_user_info(sqliterk_table *table)
{
    return sqliterkBtreeGetUserInfo((sqliterk_btree *) table);
}

//获取table类型
sqliterk_type sqliterk_table_type(sqliterk_table *table)
{
    return (sqliterk_type) sqliterkBtreeGetType((sqliterk_btree *) table);
}

//数据库注册os
int sqliterk_register(sqliterk_os os)
{
    return sqliterkOSRegister(os);
}

//返回table的根页页码
int sqliterk_table_root(sqliterk_table *table)
{
    sqliterk_page *page = sqliterkBtreeGetRootPage((sqliterk_btree *) table);
    return sqliterkPageGetPageno(page);
}

//根据状态吗，返回状态描述字符串
const char *sqliterk_description(int result)
{
    return sqliterkGetResultCodeDescription(result);
}

//数据库获取解析的页面数
int sqliterk_parsed_page_count(sqliterk *rk)
{
    return sqliterkGetParsedPageCount(rk);
}

//验证的页的数量
int sqliterk_valid_page_count(sqliterk *rk)
{
    return sqliterkGetValidPageCount(rk);
}

//获取数据库总页数
int sqliterk_page_count(sqliterk *rk)
{
    return sqliterkGetPageCount(rk);
}

//获取数据库完整性
unsigned int sqliterk_integrity(sqliterk *rk)
{
    return sqliterkGetIntegrity(rk);
}
