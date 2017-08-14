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

#ifndef path_hpp
#define path_hpp

#include <string>

//wcdb命名空间
namespace WCDB {

//路径
namespace Path {
//添加扩展
std::string addExtention(const std::string &base, const std::string &extention);
//添加路径
std::string addComponent(const std::string &base, const std::string &component);
//获取文件名
std::string getFileName(const std::string &base);
//获取基础名
std::string getBaseName(const std::string &base);
} //namespace Path

} //namespace WCDB

#endif /* path_hpp */