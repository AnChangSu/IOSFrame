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

#include <WCDB/spin.hpp>

namespace WCDB {
    
//结合 std::atomic_flag::test_and_set() 和 std::atomic_flag::clear()，std::atomic_flag 对象可以当作一个简单的自旋锁使用
/*
 如果在该线程解锁（即调用 lock.clear(std::memory_order_release)） 之前，另外一个线程也调用 lock.test_and_set(std::memory_order_acquire) 试图获得锁，则 test_and_set(std::memory_order_acquire) 返回 true，则 while 进入自旋状态。如果获得锁的线程解锁（即调用了 lock.clear(std::memory_order_release)）之后，某个线程试图调用 lock.test_and_set(std::memory_order_acquire) 并且返回 false，则 while 不会进入自旋，此时表明该线程成功地获得了锁。
 */

void Spin::lock()
{
    //获得锁
   // test_and_set() 函数检查 std::atomic_flag 标志，如果 std::atomic_flag 之前没有被设置过，则设置 std::atomic_flag 的标志，并返回先前该 std::atomic_flag 对象是否被设置过，如果之前 std::atomic_flag 对象已被设置，则返回 true，否则返回 false。
    while (locked.test_and_set(std::memory_order_acquire))
        ;
}

void Spin::unlock()
{
    //释放锁
    locked.clear(std::memory_order_release);
}

} //namespace WCDB
