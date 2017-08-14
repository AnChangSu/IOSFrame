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

#include <WCDB/describable.hpp>

namespace WCDB {
    
//构造函数
Describable::Describable(const std::string &description)
    : m_description(description)
{
}

//构造函数
Describable::Describable(const char *description) : m_description(description)
{
}
    
//获取描述字符串
const std::string &Describable::getDescription() const
{
    return m_description;
}
    
//描述字符串是否为空
bool Describable::isEmpty() const
{
    return m_description.empty();
}

//强制类型转换
Describable::operator const std::string &() const
{
    return m_description;
}

//覆盖字符串
std::string Describable::stringByReplacingOccurrencesOfString(
    const std::string &origin,
    const std::string &target,
    const std::string &replacement)
{
    bool replace = false;
    size_t last = 0;
    size_t found = 0;
    std::string output;
    //find函数的返回值是整数，假如字符串存在包含关系，其返回值必定不等于npos，但如果字符串不存在包含关系，那么返回值就一定是npos。
    while ((found = origin.find(target, last)) != std::string::npos) {
        if (!replace) {
            replace = true;
            output.clear();
        }
        std::string sub = origin.substr(last, found - last);
        output += sub;
        output += replacement;
        last = found + target.length();
    }
    if (replace) {
        output += origin.substr(last, -1);
    }
    return replace ? output : origin;
}

} //namespace WCDB
