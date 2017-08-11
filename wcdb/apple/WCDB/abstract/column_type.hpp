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

#ifndef column_type_hpp
#define column_type_hpp

#include <cstdint>
#include <string>
#include <type_traits>

namespace WCDB {
    
//列类型
enum class ColumnType : int {
    //空
    Null,
    //浮点数
    Float,
    //32位整形
    Integer32,
    //64位整形
    Integer64,
    //字符串
    Text,
    //二进制大对象(binary large object)
    BLOB,
};

//模板 列数据是否浮点型
template <typename T, typename Enable = void>
//函数匹配不能满足时 那么其的返回值类型就是std::false_type
struct ColumnIsFloatType : public std::false_type {
};
//模板 列数据是否浮点型
template <typename T>
struct ColumnIsFloatType<
    T,
    typename std::enable_if<std::is_floating_point<T>::value>::type>
    : public std::true_type {
};

//模板 列数据是否32位整型
template <typename T, typename Enable = void>
//函数匹配不能满足时 那么其的返回值类型就是std::false_type
struct ColumnIsInteger32Type : public std::false_type {
};
//模板 列数据是否32位整型
//但是当 condition 不满足的时候，enable_if<>::type 就是未定义的，当用到模板相关的场景时，只会 instantiate fail，并不会编译错误，这时很重要的一点。
template <typename T>
struct ColumnIsInteger32Type<
    T,
    typename std::enable_if<(std::is_integral<T>::value ||
                             std::is_enum<T>::value) &&
                            (sizeof(T) <= 4)>::type> : public std::true_type {
};

//模板 列数据是否64位整型
template <typename T, typename Enable = void>
//函数匹配不能满足时 那么其的返回值类型就是std::false_type
struct ColumnIsInteger64Type : public std::false_type {
};
//模板 列数据是否64位整型
template <typename T>
struct ColumnIsInteger64Type<
    T,
    typename std::enable_if<(std::is_integral<T>::value ||
                             std::is_enum<T>::value) &&
                            (sizeof(T) > 4)>::type> : public std::true_type {
};

//模板 列数据是否字符型
template <typename T, typename Enable = void>
//函数匹配不能满足时 那么其的返回值类型就是std::false_type
struct ColumnIsTextType : public std::false_type {
};
//模板 列数据是否字符型
template <typename T>
struct ColumnIsTextType<
    T,
    typename std::enable_if<std::is_same<std::string, T>::value ||
                            std::is_same<const char *, T>::value>::type>
    : public std::true_type {
};

//模板 列数据是否大二进制型
template <typename T, typename Enable = void>
//函数匹配不能满足时 那么其的返回值类型就是std::false_type
struct ColumnIsBLOBType : public std::false_type {
};
//模板 列数据是否大二进制型
template <typename T>
struct ColumnIsBLOBType<
    T,
    typename std::enable_if<std::is_same<void *, T>::value ||
                            std::is_same<const void *, T>::value>::type>
    : public std::true_type {
};

//Null
//空数据
template <ColumnType T = ColumnType::Null>
struct ColumnTypeInfo {
    static constexpr const bool isNull = true;
    static constexpr const bool isFloat = false;
    static constexpr const bool isInteger32 = false;
    static constexpr const bool isInteger64 = false;
    static constexpr const bool isText = false;
    static constexpr const bool isBLOB = false;
    static constexpr const bool isBaseType = false;
    using CType = void;
    static constexpr const ColumnType type = ColumnType::Null;
    static constexpr const char *name = "";
};
template <typename T, typename Enable = void>
struct ColumnInfo : public ColumnTypeInfo<ColumnType::Null> {
};
//Float
//浮点数
template <>
struct ColumnTypeInfo<ColumnType::Float> {
    static constexpr const bool isNull = false;
    static constexpr const bool isFloat = true;
    static constexpr const bool isInteger32 = false;
    static constexpr const bool isInteger64 = false;
    static constexpr const bool isText = false;
    static constexpr const bool isBLOB = false;
    static constexpr const bool isBaseType = true;
    using CType = double;
    static constexpr const ColumnType type = ColumnType::Float;
    static constexpr const char *name = "REAL";
};
template <typename T>
struct ColumnInfo<T, typename std::enable_if<ColumnIsFloatType<T>::value>::type>
    : public ColumnTypeInfo<ColumnType::Float> {
};
//Integer32
//32位整型
template <>
struct ColumnTypeInfo<ColumnType::Integer32> {
    static constexpr const bool isNull = false;
    static constexpr const bool isFloat = false;
    static constexpr const bool isInteger32 = true;
    static constexpr const bool isInteger64 = false;
    static constexpr const bool isText = false;
    static constexpr const bool isBLOB = false;
    static constexpr const bool isBaseType = true;
    using CType = int32_t;
    static constexpr const ColumnType type = ColumnType::Integer32;
    static constexpr const char *name = "INTEGER";
};
template <typename T>
struct ColumnInfo<
    T,
    typename std::enable_if<ColumnIsInteger32Type<T>::value>::type>
    : public ColumnTypeInfo<ColumnType::Integer32> {
};
//Integer64
//64位整型
template <>
struct ColumnTypeInfo<ColumnType::Integer64> {
    static constexpr const bool isNull = false;
    static constexpr const bool isFloat = false;
    static constexpr const bool isInteger32 = false;
    static constexpr const bool isInteger64 = true;
    static constexpr const bool isText = false;
    static constexpr const bool isBLOB = false;
    static constexpr const bool isBaseType = true;
    using CType = int64_t;
    static constexpr const ColumnType type = ColumnType::Integer64;
    static constexpr const char *name = "INTEGER";
};
template <typename T>
struct ColumnInfo<
    T,
    typename std::enable_if<ColumnIsInteger64Type<T>::value>::type>
    : public ColumnTypeInfo<ColumnType::Integer64> {
};
//Text
//字符型
template <>
struct ColumnTypeInfo<ColumnType::Text> {
    static constexpr const bool isNull = false;
    static constexpr const bool isFloat = false;
    static constexpr const bool isInteger32 = false;
    static constexpr const bool isInteger64 = false;
    static constexpr const bool isText = true;
    static constexpr const bool isBLOB = false;
    static constexpr const bool isBaseType = true;
    using CType = const char *;
    static constexpr const ColumnType type = ColumnType::Text;
    static constexpr const char *name = "TEXT";
};
template <typename T>
struct ColumnInfo<T, typename std::enable_if<ColumnIsTextType<T>::value>::type>
    : public ColumnTypeInfo<ColumnType::Text> {
};
//BLOB
//大二进制型
template <>
struct ColumnTypeInfo<ColumnType::BLOB> {
    static constexpr const bool isNull = false;
    static constexpr const bool isFloat = false;
    static constexpr const bool isInteger32 = false;
    static constexpr const bool isInteger64 = false;
    static constexpr const bool isText = false;
    static constexpr const bool isBLOB = true;
    static constexpr const bool isBaseType = false;
    using CType = const void *;
    using SizeType = int;
    static constexpr const ColumnType type = ColumnType::BLOB;
    static constexpr const char *name = "BLOB";
};
template <typename T>
struct ColumnInfo<T, typename std::enable_if<ColumnIsBLOBType<T>::value>::type>
    : public ColumnTypeInfo<ColumnType::BLOB> {
};

const char *ColumnTypeName(ColumnType type);

} //namespace WCDB

#endif /* column_type_hpp */
