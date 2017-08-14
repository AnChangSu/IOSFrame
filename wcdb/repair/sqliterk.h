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

#ifndef sqliterk_h
#define sqliterk_h

//数据库结构体
typedef struct sqliterk sqliterk;
//数据库密码结构体
typedef struct sqliterk_cipher_conf sqliterk_cipher_conf;
//数据库通知结构体 
typedef struct sqliterk_notify sqliterk_notify;

//打开数据库
int sqliterkOpen(const char *path,
                 const sqliterk_cipher_conf *cipher,
                 sqliterk **rk);
//解析数据库
int sqliterkParse(sqliterk *rk);
//解析数据库页
int sqliterkParsePage(sqliterk *rk, int pageno);

//数据库中第一个页(page 1)永远是Btree页。而每个表或索引的第1个页称为根页，所有表或索引的根页编号都存储在系统表sqlite_master中，表sqlite_master的根页为page 1
//解析数据库master页
int sqliterkParseMaster(sqliterk *rk);
//关闭数据库
int sqliterkClose(sqliterk *rk);
//数据库设置通知
int sqliterkSetNotify(sqliterk *rk, sqliterk_notify notify);
//数据库设置用户信息userinfo
int sqliterkSetUserInfo(sqliterk *rk, void *userInfo);
//数据库获取用户信息userinfo
void *sqliterkGetUserInfo(sqliterk *rk);

//获取数据库已经解析的页数
int sqliterkGetParsedPageCount(sqliterk *rk);
//获取数据库验证过的页数
int sqliterkGetValidPageCount(sqliterk *rk);
//获取数据库总页数
int sqliterkGetPageCount(sqliterk *rk);
//获取数据库完整性
unsigned int sqliterkGetIntegrity(sqliterk *rk);

#endif /* sqliterk_h */
