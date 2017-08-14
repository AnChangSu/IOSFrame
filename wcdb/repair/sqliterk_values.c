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

#include "sqliterk_values.h"
#include "SQLiteRepairKit.h"
#include "sqliterk_os.h"
#include "sqliterk_util.h"
#include <stdlib.h>
#include <string.h>

//declaration
//根据当前的count数量，重新计算values的容量，并重新分配内存
static int sqliterkValuesAutoGrow(sqliterk_values *values);

//定义整型，数据型，文字型数据
typedef int64_t sqliterk_integer;
typedef double sqliterk_number;
typedef struct sqliterk_text sqliterk_text;
struct sqliterk_text {
    char *t;
    int s;          //size
};
//定义二进制块数据
typedef struct sqliterk_binary sqliterk_binary;
struct sqliterk_binary {
    void *b;
    int s;         //size
};

//定义数据库数据类型结构体
typedef union sqliterk_any sqliterk_any;
union sqliterk_any {
    sqliterk_integer *integer;
    sqliterk_number *number;
    sqliterk_text *text;
    sqliterk_binary *binary;
    void *memory;
};

//定义value结构体，包含value和数据
struct sqliterk_value {
    sqliterk_value_type type;
    sqliterk_any any;
};

//定义values结构体，包含数组大小，容量和value数组
//数据队列
struct sqliterk_values {
    int count;
    int capacity;
    sqliterk_value *values;
};

//给指定的values申请内存
int sqliterkValuesAlloc(sqliterk_values **values)
{
    if (!values) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;
    //根据values结构体大小，申请内存
    sqliterk_values *theValues = sqliterkOSMalloc(sizeof(sqliterk_values));
    if (!theValues) {
        rc = SQLITERK_NOMEM;
        goto sqliterkValuesAlloc_Failed;
    }
    //根据当前的count数量，重新计算values的容量，并重新分配数组内存
    rc = sqliterkValuesAutoGrow(theValues);
    if (rc != SQLITERK_OK) {
        goto sqliterkValuesAlloc_Failed;
    }
    *values = theValues;
    return SQLITERK_OK;
sqliterkValuesAlloc_Failed:
    if (theValues) {
        sqliterkValuesFree(theValues);
    }
    return rc;
}

//释放values内存
int sqliterkValuesFree(sqliterk_values *values)
{
    if (!values) {
        return SQLITERK_MISUSE;
    }

    int i;
    //获取values中所有的value
    for (i = 0; i < values->count; i++) {
        sqliterk_value *value = &values->values[i];
        //清空vaule状态，并释放value->any相关的内存
        sqliterkValueClear(value);
    }
    values->count = 0;
    //释放values中的value数组内存
    if (values->values) {
        sqliterkOSFree(values->values);
        values->values = NULL;
    }
    values->capacity = 0;
    //释放values内存
    sqliterkOSFree(values);
    return SQLITERK_OK;
}

////清空values中所有value状态，并释放value->any相关的内存
int sqliterkValuesClear(sqliterk_values *values)
{
    if (!values) {
        return SQLITERK_MISUSE;
    }

    int i;
    for (i = 0; i < values->count; i++) {
        sqliterk_value *value = &values->values[i];
        sqliterkValueClear(value);
    }
    values->count = 0;
    return SQLITERK_OK;
}

//根据当前的count数量，重新计算values的容量，并重新分配内存
static int sqliterkValuesAutoGrow(sqliterk_values *values)
{
    if (!values) {
        return SQLITERK_MISUSE;
    }
    //当count大于当前容量的时候，重新计算容量
    if (values->count >= values->capacity) {
        int oldCapacity = values->capacity;
        if (oldCapacity <= 0) {
            //init for 4
            values->capacity = 4;
        } else {
            values->capacity = oldCapacity * 2;
        }
        //分配新容量的内存
        sqliterk_value *newValues =
            sqliterkOSMalloc(sizeof(sqliterk_value) * (values->capacity + 1));
        if (!newValues) {
            return SQLITERK_NOMEM;
        }
        //拷贝原数据到申请的内存中，并释放values数组内存
        if (values->values) {
            memcpy(newValues, values->values,
                   sizeof(sqliterk_value) * oldCapacity);
            sqliterkOSFree(values->values);
        }
        //使values指向新的数组
        values->values = newValues;
    }
    return SQLITERK_OK;
}

//获取vlues结构体的count参数
int sqliterkValuesGetCount(sqliterk_values *values)
{
    if (!values) {
        return 0;
    }
    return values->count;
}

//获取values结构体重指定index的value的类型
sqliterk_value_type sqliterkValuesGetType(sqliterk_values *values, int index)
{
    if (values && index < sqliterkValuesGetCount(values)) {
        return values->values[index].type;
    }
    return sqliterk_value_type_null;
}

//获取values结构体重指定index的value的数据
int64_t sqliterkValuesGetInteger64(sqliterk_values *values, int index)
{
    int64_t out = 0;
    if (values && index < sqliterkValuesGetCount(values)) {
        sqliterk_value *value = &values->values[index];
        switch (sqliterkValuesGetType(values, index)) {
                //数据是Int
            case sqliterk_value_type_integer:
                out = (int64_t)(*value->any.integer);
                break;
                //数据是double
            case sqliterk_value_type_number:
                out = (int64_t)(*value->any.number);
                break;
                //数据是text
            case sqliterk_value_type_text:
                //atol功 能: 把字符串转换成长整型数
                out = atol(value->any.text->t);
                break;
            default:
                break;
        }
    }
    return out;
}

//获取指定index的value的int数据
int sqliterkValuesGetInteger(sqliterk_values *values, int index)
{
    return (int) sqliterkValuesGetInteger64(values, index);
}

//获取指定index的value的double数据
double sqliterkValuesGetNumber(sqliterk_values *values, int index)
{
    double out = 0;
    if (values && index < sqliterkValuesGetCount(values)) {
        sqliterk_value *value = &values->values[index];
        switch (sqliterkValuesGetType(values, index)) {
            case sqliterk_value_type_integer:
                out = (double) (*value->any.integer);
                break;
            case sqliterk_value_type_number:
                out = (double) (*value->any.number);
                break;
            case sqliterk_value_type_text:
                //atol功 能: 把字符串转换成长整型数
                out = atof(value->any.text->t);
                break;
            default:
                break;
        }
    }
    return out;
}

//获取指定index的value的text数据
const char *sqliterkValuesGetText(sqliterk_values *values, int index)
{
    char *out = NULL;
    if (values && index < sqliterkValuesGetCount(values)) {
        sqliterk_value *value = &values->values[index];
        switch (value->type) {
            case sqliterk_value_type_text:
                out = value->any.text->t;
                break;
            default:
                break;
        }
    }
    return out;
}

//获取指定index的value的二进制块数据
const void *sqliterkValuesGetBinary(sqliterk_values *values, int index)
{
    void *out = NULL;
    if (values && index < sqliterkValuesGetCount(values)) {
        sqliterk_value *value = &values->values[index];
        switch (value->type) {
            case sqliterk_value_type_binary:
                out = value->any.binary->b;
                break;
            default:
                break;
        }
    }
    return out;
}

//获取指定index的value的大小
int sqliterkValuesGetBytes(sqliterk_values *values, int index)
{
    int out = 0;
    if (values && index < sqliterkValuesGetCount(values)) {
        sqliterk_value *value = &values->values[index];
        switch (value->type) {
            case sqliterk_value_type_binary:
                out = value->any.binary->s;
                break;
            case sqliterk_value_type_text:
                out = value->any.text->s;
                break;
            default:
                break;
        }
    }
    return out;
}

//为values数组添加int64类型value数据
int sqliterkValuesAddInteger64(sqliterk_values *values, int64_t i)
{
    if (!values) {
        return SQLITERK_MISUSE;
    }
    //计算values数组容量
    int rc = sqliterkValuesAutoGrow(values);
    if (rc != SQLITERK_OK) {
        return rc;
    }
    //获取数组有数据的value后第一个空value
    sqliterk_value *value = &values->values[values->count];
    //设置value数据类型，申请内存
    value->type = sqliterk_value_type_integer;
    value->any.integer = sqliterkOSMalloc(sizeof(sqliterk_integer));
    if (!value->any.integer) {
        rc = SQLITERK_NOMEM;
        goto sqliterkValuesAddInteger64_Failed;
    }
    //设置any.integer的数据
    *value->any.integer = i;
    values->count++;
    return SQLITERK_OK;

sqliterkValuesAddInteger64_Failed:
    sqliterkValueClear(value);
    return rc;
}

//为values数组添加int类型value数据
int sqliterkValuesAddInteger(sqliterk_values *values, int i)
{
    return sqliterkValuesAddInteger64(values, i);
}

//为values数组添加number类型value数据
int sqliterkValuesAddNumber(sqliterk_values *values, double d)
{
    if (!values) {
        return SQLITERK_MISUSE;
    }
    //计算values数组容量
    int rc = sqliterkValuesAutoGrow(values);
    if (rc != SQLITERK_OK) {
        return rc;
    }
    //获取数组有数据的value后第一个空value
    sqliterk_value *value = &values->values[values->count];
    //设置value数据类型，申请内存
    value->type = sqliterk_value_type_number;
    value->any.number = sqliterkOSMalloc(sizeof(sqliterk_number));
    if (!value->any.number) {
        rc = SQLITERK_NOMEM;
        goto sqliterkValuesAddNumber_Failed;
    }
    //设置any.number的数据
    *value->any.number = d;
    values->count++;
    return SQLITERK_OK;

sqliterkValuesAddNumber_Failed:
    sqliterkValueClear(value);
    return rc;
}

//为values数组添加text类型value数据
int sqliterkValuesAddText(sqliterk_values *values, const char *t)
{
    return sqliterkValuesAddNoTerminatorText(values, t, (int) strlen(t));
}

//为values数组添加text类型value数据
int sqliterkValuesAddNoTerminatorText(sqliterk_values *values,
                                      const char *t,
                                      const int s)
{
    if (!values || !t) {
        return SQLITERK_MISUSE;
    }
    //计算values数组容量
    int rc = sqliterkValuesAutoGrow(values);
    if (rc != SQLITERK_OK) {
        return rc;
    }
    //获取数组有数据的value后第一个空value
    sqliterk_value *value = &values->values[values->count];
    //设置value数据类型，申请内存
    value->type = sqliterk_value_type_text;
    value->any.text = sqliterkOSMalloc(sizeof(sqliterk_text));
    if (!value->any.text) {
        rc = SQLITERK_NOMEM;
        goto sqliterkValuesAddNoTerminatorText_Failed;
    }
    //设置value数据类型，申请内存
    value->any.text->s = s;
    value->any.text->t = sqliterkOSMalloc(sizeof(char) * (s + 1));
    if (!value->any.text->t) {
        rc = SQLITERK_NOMEM;
        goto sqliterkValuesAddNoTerminatorText_Failed;
    }
    //将字符串的内容拷贝到text->t中
    memcpy(value->any.text->t, t, s);
    //为字符串添加结束符
    value->any.text->t[s] = '\0';
    values->count++;
    return SQLITERK_OK;

sqliterkValuesAddNoTerminatorText_Failed:
    sqliterkValueClear(value);
    return rc;
}

//为values数组添加二进制块类型value数据
int sqliterkValuesAddBinary(sqliterk_values *values, const void *b, const int s)
{
    if (!values || !b) {
        return SQLITERK_MISUSE;
    }
    //计算values数组容量
    int rc = sqliterkValuesAutoGrow(values);
    if (rc != SQLITERK_OK) {
        return rc;
    }
    //获取数组有数据的value后第一个空value
    sqliterk_value *value = &values->values[values->count];
    //设置value数据类型，申请内存
    value->type = sqliterk_value_type_binary;
    value->any.binary = sqliterkOSMalloc(sizeof(sqliterk_binary));
    if (!value->any.binary) {
        rc = SQLITERK_NOMEM;
        goto sqliterkValuesAddBinary_Failed;
    }
    //设置value数据类型，申请内存
    value->any.binary->s = s;
    value->any.binary->b = sqliterkOSMalloc(sizeof(void *) * s);
    if (!value->any.binary->b) {
        return SQLITERK_NOMEM;
    }
    //将数据拷贝到binary->b中
    memcpy(value->any.binary->b, b, s);
    values->count++;
    return SQLITERK_OK;

sqliterkValuesAddBinary_Failed:
    sqliterkValueClear(value);
    return rc;
}

//为values数组添加空数据
int sqliterkValuesAddNull(sqliterk_values *values)
{
    if (!values) {
        return SQLITERK_MISUSE;
    }
     //计算values数组容量
    int rc = sqliterkValuesAutoGrow(values);
    if (rc != SQLITERK_OK) {
        return rc;
    }
    //获取数组有数据的value后第一个空value
    sqliterk_value *value = &values->values[values->count];
    //设置数据状态和数据内容为空
    value->type = sqliterk_value_type_null;
    value->any.memory = NULL;
    values->count++;
    return SQLITERK_OK;
}

//清空value状态，释放value->any相关内存
int sqliterkValueClear(sqliterk_value *value)
{
    if (!value) {
        return SQLITERK_MISUSE;
    }
    if (value->any.memory) {
        switch (value->type) {
            case sqliterk_value_type_text:
                if (value->any.text->t) {
                    sqliterkOSFree(value->any.text->t);
                    value->any.text->t = NULL;
                }
                break;
            case sqliterk_value_type_binary:
                if (value->any.binary->b) {
                    sqliterkOSFree(value->any.binary->b);
                    value->any.binary->b = NULL;
                }
                break;
            default:
                break;
        }
        sqliterkOSFree(value->any.memory);
        value->any.memory = NULL;
    }
    value->type = sqliterk_value_type_null;
    return SQLITERK_OK;
}
