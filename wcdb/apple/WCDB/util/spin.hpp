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

#ifndef spin_hpp
#define spin_hpp

#include <atomic>

namespace WCDB {

class Spin {
public:
    void lock();
    void unlock();

protected:
    //使用ATOMIC_FLAG_INIT初始化，保证对象创建时处于clear状态
    //如果在初始化时没有明确使用 ATOMIC_FLAG_INIT初始化，那么新创建的 std::atomic_flag 对象的状态是未指定的（unspecified）（既没有被 set 也没有被 clear。）另外，atomic_flag不能被拷贝，也不能 move 赋值。
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
};

template <typename Spin>
class SpinLockGuard {
public:
    SpinLockGuard(Spin &spin) : m_spin(spin) { m_spin.lock(); }
    ~SpinLockGuard() { m_spin.unlock(); }

protected:
    Spin &m_spin;
};

} //namespace WCDB

#endif /* spin_hpp */
