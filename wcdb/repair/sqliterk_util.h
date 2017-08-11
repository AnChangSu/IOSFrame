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

#ifndef sqliterk_util_h
#define sqliterk_util_h

#include <stdio.h>

//读取data中offset偏移，length长度的int
int sqliterkParseInt(const unsigned char *data,
                     int offset,
                     int length,
                     int *value);
//读取data中offset偏移，length长度的int64
int sqliterkParseInt64(const unsigned char *data,
                       int offset,
                       int length,
                       int64_t *value);
//读取data中offset偏移，length长度的数据压缩int
int sqliterkParseVarint(const unsigned char *data,
                        int offset,
                        int *length,
                        int *value);
//读取data中offset偏移，length长度的数据压缩int64
int sqliterkParseVarint64(const unsigned char *data,
                          int offset,
                          int *length,
                          int64_t *value);
//读取data中offset偏移number
int sqliterkParseNumber(const unsigned char *data, int offset, double *value);
//获取最大的有符号整形的长度
int sqliterkGetMaxVarintLength();
//根据状态吗，返回状态描述字符串
const char *sqliterkGetResultCodeDescription(int result);

#endif /* sqliterk_util_h */
