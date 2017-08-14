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

#ifndef in_case_lock_guard_hpp
#define in_case_lock_guard_hpp

#include <memory>
#include <mutex>

namespace WCDB {

class InCaseLockGuard {
public:
    /*
     shared_ptr使用引用计数，每一个shared_ptr的拷贝都指向相同的内存。每使用他一次，内部的引用计数加1，每析构一次，内部的引用计数减1，减为0时，删除所指向的堆内存。shared_ptr内部的引用计数是安全的，但是对象的读取需要加锁。
     */
    InCaseLockGuard(std::shared_ptr<std::mutex> &mutex);
    ~InCaseLockGuard();

protected:
    //对象互斥锁
    std::shared_ptr<std::mutex> m_mutex;
};

} //namespace WCDB

#endif /* in_case_lock_guard_hpp */
