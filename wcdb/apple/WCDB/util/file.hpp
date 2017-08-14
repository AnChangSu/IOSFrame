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

#ifndef file_hpp
#define file_hpp

#include <WCDB/error.hpp>
#include <list>
#include <string>

//wcdb命名空间
namespace WCDB {

//文件命名空间
namespace File {
//Basic
//指定路径文件是否存在
bool isExists(const std::string &path, Error &error);
//获取文件大小
size_t getFileSize(const std::string &path, Error &error);
//建立硬链接
bool createHardLink(const std::string &from,
                    const std::string &to,
                    Error &error);
//移除硬连接
bool removeHardLink(const std::string &path, Error &error);
//移除文件
bool removeFile(const std::string &path, Error &error);
//创建文件夹
bool createDirectory(const std::string &path, Error &error);
//Combination
//获取文件大小
size_t getFilesSize(const std::list<std::string> &paths, Error &error);
//移除文件
bool removeFiles(const std::list<std::string> &paths, Error &error);
//移动文件
bool moveFiles(const std::list<std::string> &paths,
               const std::string &directory,
               Error &error);
//是否能生成指定路径文件
bool createDirectoryWithIntermediateDirectories(const std::string &path,
                                                Error &error);
} //namespace File

} //namespace WCDB

#endif /* file_hpp */
