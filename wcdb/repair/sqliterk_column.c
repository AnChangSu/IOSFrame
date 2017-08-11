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

#include "sqliterk_column.h"
#include "sqliterk_os.h"

//数据库列结构体
struct sqliterk_column {
    int rowid;
    //列数据信息
    sqliterk_values *values;
    //页信息
    sqliterk_values *overflowPages;
};

//为列结构体对象申请内存
int sqliterkColumnAlloc(sqliterk_column **column)
{
    if (!column) {
        return SQLITERK_MISUSE;
    }
    int rc = SQLITERK_OK;
    //列结构体对象申请内存
    sqliterk_column *theColumn = sqliterkOSMalloc(sizeof(sqliterk_column));
    if (!theColumn) {
        rc = SQLITERK_NOMEM;
        goto sqliterkColumnAlloc_Failed;
    }
    //列数据信息申请内存
    rc = sqliterkValuesAlloc(&theColumn->values);
    if (rc != SQLITERK_OK) {
        goto sqliterkColumnAlloc_Failed;
    }
    //页信息申请内存
    rc = sqliterkValuesAlloc(&theColumn->overflowPages);
    if (rc != SQLITERK_OK) {
        goto sqliterkColumnAlloc_Failed;
    }
    *column = theColumn;
    return SQLITERK_OK;

    //内存申请失败，释放内存
sqliterkColumnAlloc_Failed:
    if (theColumn) {
        sqliterkColumnFree(theColumn);
    }
    *column = NULL;
    return rc;
}

//释放列对象内存
int sqliterkColumnFree(sqliterk_column *column)
{
    if (!column) {
        return SQLITERK_MISUSE;
    }
    //释放页信息内存
    if (column->overflowPages) {
        sqliterkValuesFree(column->overflowPages);
    }
    //释放列数据内存
    if (column->values) {
        sqliterkValuesFree(column->values);
    }
    sqliterkOSFree(column);
    return SQLITERK_OK;
}

//获取列对象的数据数组sqliterk_values
sqliterk_values *sqliterkColumnGetValues(sqliterk_column *column)
{
    if (!column) {
        return NULL;
    }
    return column->values;
}

//设置列对象的id
void sqliterkColumnSetRowId(sqliterk_column *column, int rowid)
{
    if (column) {
        column->rowid = rowid;
    }
}

//获取列对象的id
int sqliterkColumnGetRowId(sqliterk_column *column)
{
    if (!column) {
        return 0;
    }
    return column->rowid;
}

//返回列对象的页信息
sqliterk_values *sqliterkColumnGetOverflowPages(sqliterk_column *column)
{
    if (!column) {
        return NULL;
    }
    return column->overflowPages;
}

//清空列数据
int sqliterkColumnClear(sqliterk_column *column)
{
    if (!column) {
        return SQLITERK_MISUSE;
    }
    column->rowid = 0;
    sqliterkValuesClear(column->overflowPages);
    sqliterkValuesClear(column->values);
    return SQLITERK_OK;
}
