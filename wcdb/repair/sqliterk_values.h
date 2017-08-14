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

#ifndef sqliterk_values_h
#define sqliterk_values_h

#include "SQLiteRepairKit.h"
#include <stdint.h>

// sqliterk_values is a kind of [Any] array.
//包含sqliterk_value数组的结构体
typedef struct sqliterk_values sqliterk_values;
//标记value类型的结构体
typedef struct sqliterk_value sqliterk_value;

//申请内存
int sqliterkValuesAlloc(sqliterk_values **values);
//释放内存
int sqliterkValuesFree(sqliterk_values *values);
//清空values中所有value状态
int sqliterkValuesClear(sqliterk_values *values);

//为values数组添加int类型value数据
int sqliterkValuesAddInteger(sqliterk_values *values, int i);
//为values数组添加int64类型value数据
int sqliterkValuesAddInteger64(sqliterk_values *values, int64_t i);
//为values数组添加number类型value数据
int sqliterkValuesAddNumber(sqliterk_values *values, double d);
//为values数组添加text类型value数据
int sqliterkValuesAddText(sqliterk_values *values, const char *t);
//为values数组添加text类型value数据
int sqliterkValuesAddNoTerminatorText(sqliterk_values *values,
                                      const char *t,
                                      const int s);
//为values数组添加二进制块类型value数据
int sqliterkValuesAddBinary(sqliterk_values *values,
                            const void *b,
                            const int s);
//为values数组添加空数据
int sqliterkValuesAddNull(sqliterk_values *values);

//获取values数组大小
int sqliterkValuesGetCount(sqliterk_values *values);
//获取指定index的value的类型
sqliterk_value_type sqliterkValuesGetType(sqliterk_values *values, int index);
//获取指定index的value的int数据
int sqliterkValuesGetInteger(sqliterk_values *values, int index);
//获取指定index的value的int64数据
int64_t sqliterkValuesGetInteger64(sqliterk_values *values, int index);
//获取指定index的value的double数据
double sqliterkValuesGetNumber(sqliterk_values *values, int index);
//获取指定index的value的text数据
const char *sqliterkValuesGetText(sqliterk_values *values, int index);
//获取指定index的value的二进制块数据
const void *sqliterkValuesGetBinary(sqliterk_values *values, int index);
//获取指定index的value的大小
int sqliterkValuesGetBytes(sqliterk_values *values, int index);

//清空value状态，释放value->any相关内存
int sqliterkValueClear(sqliterk_value *value);

#endif /* sqliterk_values_h */
