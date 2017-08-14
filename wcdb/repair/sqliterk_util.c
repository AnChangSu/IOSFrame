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

#include "sqliterk_util.h"
#include "SQLiteRepairKit.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

//读取data中offset偏移，length长度的int
int sqliterkParseInt(const unsigned char *data,
                     int offset,
                     int length,
                     int *value)
{
    //如果value为空，或length大于一个Int长度
    if (!value || length > sizeof(int)) {
        return SQLITERK_MISUSE;
    }
    int64_t out;
    //读取int64数据
    int rc = sqliterkParseInt64(data, offset, length, &out);
    //如果读取失败，则返回失败状态码
    if (rc != SQLITERK_OK) {
        return rc;
    }
    //转换int64到int并给value赋值
    *value = (int) out;
    return SQLITERK_OK;
}

//读取data中offset偏移，length长度的int64
int sqliterkParseInt64(const unsigned char *data,
                       int offset,
                       int length,
                       int64_t *value)
{
    //如果数据指针或返回指针为空，则返回错误
    if (!data || !value)
        return SQLITERK_MISUSE;

    const unsigned char *p = data + offset;
    int64_t out;

    //实际256是2的8次方，和做移8位基本相同，只是乘法的话呢 可能会产生益处，影响OF的值的，左移就不会影响OF。而且如果是有符号乘法的话，最高位那个表示数字正负的符号不会改变，而位移可能改变第一位的
    //所以最高位使用乘法，用来确保数据的溢出和数据有符号时的正确性
    switch (length) {
        case 1:
            out = (int8_t) p[0];
            break;
        case 2:
            //2的8次方
            out = (((int8_t) p[0]) * 256) | p[1];
            break;
        case 3:
            //2的16次方
            out = (((int8_t) p[0]) * 65536) | (p[1] << 8) | p[2];
            break;
        case 4:
            //2的24次方，4字节齐全
            out = (((int8_t) p[0]) * 16777216) | (p[1] << 16) | (p[2] << 8) |
                  p[3];
            break;
        case 6:
            //拼接高位
            out = (((int8_t) p[0]) * 256) | p[1];
            out *= ((int64_t) 1) << 32;
            //拼接低位
            out +=
                (((uint32_t) p[2]) << 24) | (p[3] << 16) | (p[4] << 8) | p[5];
            break;
        case 8:
            //拼接高位
            out = (((int8_t) p[0]) * 16777216) | (p[1] << 16) | (p[2] << 8) |
                  p[3];
            out *= ((int64_t) 1) << 32;
            //拼接低位
            out +=
                (((uint32_t) p[4]) << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
            break;
        default:
            return SQLITERK_MISUSE;
    }

    *value = out;
    return SQLITERK_OK;
}

// Varint is a kind of huffman encoding value. For further informantion,
// see https://www.sqlite.org/fileformat2.html#varint
//读取data中offset偏移，length长度的数据压缩int
int sqliterkParseVarint(const unsigned char *data,
                        int offset,
                        int *length,
                        int *value)
{
    if (!value) {
        return SQLITERK_MISUSE;
    }
    int64_t out;
    int rc = sqliterkParseVarint64(data, offset, length, &out);
    if (rc != SQLITERK_OK) {
        return rc;
    }
    *value = (int) out;
    return SQLITERK_OK;
}

//读取data中offset偏移，length长度的数据压缩int64
int sqliterkParseVarint64(const unsigned char *data,
                          int offset,
                          int *length,
                          int64_t *value)
{
    if (!data || !length || !value) {
        return SQLITERK_MISUSE;
    }
    int64_t out = 0;
    const unsigned char *begin = data + offset;
    int i = 0;
    //begin[i]高4位有数据，且i小于最大长度
    while ((begin[i] & 0x80) && i < sqliterkGetMaxVarintLength()) {
        //begin[i] & 01111111,摒弃掉begin[i]的最高位后，与out按位或
        //每次读取7位
        out |= (begin[i] & 0x7f);
        out = (out << 7);
        i++;
        if (i >= sqliterkGetMaxVarintLength()) {
            return SQLITERK_DAMAGED;
        }
    }
    out |= begin[i];
    *length = i + 1;
    *value = out;
    return SQLITERK_OK;
}

//获取最大的有符号整形的长度
int sqliterkGetMaxVarintLength()
{
    return 9;
}

//读取data中offset偏移number
int sqliterkParseNumber(const unsigned char *data, int offset, double *value)
{
    if (!data || !value) {
        return SQLITERK_MISUSE;
    }
    unsigned char out[8];
    const unsigned char *begin = data + offset;
    int i;
    for (i = 0; i < 8; i++) {
        // All float values in SQLite is big-endian with 8 lengths.
        // For further informantion, see [Record Format] chapter at
        // https://www.sqlite.org/fileformat2.html
        out[i] = begin[8 - 1 - i];
        //out[0]=begin[7],out7=begin[0];out[0]存储最高位
    }
    //将out中的8位拷贝到value中
    memcpy(value, out, 8);
    return SQLITERK_OK;
}

//根据状态吗，返回状态描述字符串
const char *sqliterkGetResultCodeDescription(int result)
{
    switch (result) {
        case SQLITERK_OK:
            return "SQLITERK_OK";
        case SQLITERK_CANTOPEN:
            return "SQLITERK_CANTOPEN";
        case SQLITERK_MISUSE:
            return "SQLITERK_MISUSE";
        case SQLITERK_IOERR:
            return "SQLITERK_IOERR";
        case SQLITERK_NOMEM:
            return "SQLITERK_NOMEM";
        case SQLITERK_SHORT_READ:
            return "SQLITERK_SHORT_READ";
        case SQLITERK_DAMAGED:
            return "SQLITERK_DAMAGED";
        case SQLITERK_DISCARD:
            return "SQLITERK_DISCARD";
        default:
            break;
    }
    return "SQLITERK_UNKNOWN";
}
