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

#ifndef error_hpp
#define error_hpp

#include <WCDB/utility.hpp>
#include <climits>
#include <map>
#include <string>

namespace WCDB {

typedef int Tag;
const Tag InvalidTag = 0;

class ErrorValue {
public:
    enum class Type : int {
        Int,
        String,
    };
    //构造函数，初始化ErrorValue对象
    ErrorValue(int value);
    ErrorValue(const char *value);
    ErrorValue(const std::string &value);

    //获取Int value
    int getIntValue() const;
    //获取string value
    std::string getStringValue() const;
    //获取type信息
    ErrorValue::Type getType() const;

protected:
    //type
    ErrorValue::Type m_type;
    //int 数据
    int m_intValue;
    //string 数据
    std::string m_stringValue;
};

class Error {
public:
    //key
    //类型key
    enum class Key : int {
        Tag = 1,
        Operation = 2,
        ExtendedCode = 3,
        Message = 4,
        SQL = 5,
        Path = 6,
    };
    //type
    //错误类型
    enum class Type : int {
        NotSet = 0,
        SQLite = 1,
        SystemCall = 2,
        Core = 3,
        Interface = 4,
        Abort = 5,
        Warning = 6,
        SQLiteGlobal = 7,
        Repair = 8,
    };
    //operation
    //句柄操作
    enum class HandleOperation : int {
        Open = 1,
        Close = 2,
        Prepare = 3,
        Exec = 4,
        Step = 5,
        Finalize = 6,
        SetCipherKey = 7,
        IsTableExists = 8,
    };
    //接口操作
    enum class InterfaceOperation : int {
        StatementHandle = 1,
        Insert = 2,
        Update = 3,
        Select = 4,
        Table = 5,
        ChainCall = 6,
    };
    //核心操作
    enum class CoreOperation : int {
        Prepare = 1,
        Exec = 2,
        Begin = 3,
        Commit = 4,
        Rollback = 5,
        GetThreadedHandle = 6,
    };
    //系统调用操作
    enum class SystemCallOperation : int {
        Lstat = 1,
        Access = 2,
        Remove = 3,
        Link = 4,
        Unlink = 5,
        Mkdir = 6,
    };
    //修复操作
    enum class RepairOperation : int {
        SaveMaster,
        LoadMaster,
        Repair,
    };
    //code
    //核心编码
    enum class CoreCode : int {
        Misuse = 1,   //误用
    };
    //接口编码
    enum class InterfaceCode : int {
        ORM = 1,
        Inconsistent = 2,
        NilObject = 3,
        Misuse = 4,
    };
    //全局编码
    enum class GlobalCode : int {
        Warning = 1,
        Abort = 2,
    };

    //错误信息地图
    typedef std::map<Key, ErrorValue> Infos;
    //汇报错误的方法
    typedef std::function<void(const Error &)> ReportMethod;

    //构造函数，初始化
    Error();
    Error(Error::Type type, int code, const Error::Infos &infos);

    //获取错误类型
    Error::Type getType() const;
    //获取错误编码
    int getCode() const;
    //获取错误信息
    const Error::Infos &getInfos() const;
    //获取是否有错误
    bool isOK() const; //getCode()==0
    //重置错误信息
    void reset();
    //错误描述
    std::string description() const;

    //调用ReportMethod 打印错误
    void report() const;

    //获取key的描述字符串
    static const char *GetKeyName(Key key);
    //获取type的描述字符串
    static const char *GetTypeName(Type type);

    //设置reportMethod
    static void SetReportMethod(const ReportMethod &reportMethod);

    //发送错误
    static void Report(Error::Type type,
                       int code,
                       const std::map<Key, ErrorValue> &infos,
                       Error *outError);

    //Convenience
    //易用
    //发送sql错误
    static void ReportSQLite(Tag tag,
                             const std::string &path,
                             HandleOperation operation,
                             int code,
                             const char *message,
                             Error *outError);
    //发送sql错误
    static void ReportSQLite(Tag tag,
                             const std::string &path,
                             HandleOperation operation,
                             int code,
                             int extendedError,
                             const char *message,
                             Error *outError);
    //发送sql错误
    static void ReportSQLite(Tag tag,
                             const std::string &path,
                             HandleOperation operation,
                             int code,
                             int extendedError,
                             const char *message,
                             const std::string &sql,
                             Error *outError);
    //发送核心方法错误
    static void ReportCore(Tag tag,
                           const std::string &path,
                           CoreOperation operation,
                           CoreCode code,
                           const char *message,
                           Error *outError);
    //发送接口调用错误
    static void ReportInterface(Tag tag,
                                const std::string &path,
                                InterfaceOperation operation,
                                InterfaceCode code,
                                const char *message,
                                Error *outError);
    ////发送系统调用错误
    static void ReportSystemCall(SystemCallOperation operation,
                                 const std::string &path,
                                 int code,
                                 const char *message,
                                 Error *outError);
    //发送sql全局错误错误
    static void
    ReportSQLiteGlobal(int rc, const char *message, Error *outError);
    //发送修复错误
    static void ReportRepair(const std::string &path,
                             RepairOperation operation,
                             int code,
                             Error *outError);
    //中止  错误
    static void Abort(const char *message, Error *outError = nullptr);
    //警告  错误
    static void Warning(const char *message, Error *outError = nullptr);

protected:
    //错误编码
    int m_code;
    //错误类型
    Error::Type m_type;
    //错误信息
    Error::Infos m_infos;

    static std::shared_ptr<Error::ReportMethod> s_reportMethod;
};

} //namespace WCDB

#endif /* error_hpp */
