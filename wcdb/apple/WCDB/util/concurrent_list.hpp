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

#ifndef concurrent_list_hpp
#define concurrent_list_hpp

#include <WCDB/spin.hpp>
#include <list>

namespace WCDB {

//并行队列
template <typename T>
class ConcurrentList {
public:
    using ElementType = std::shared_ptr<T>;
    //构造函数初始化容量
    ConcurrentList(size_t capacityCap) : m_capacityCap(capacityCap) {}

    size_t getCapacityCap() const
    {
        //自旋锁
        SpinLockGuard<Spin> lockGuard(m_spin);
        //获得锁后返回容量
        return m_capacityCap;
    }

    bool pushBack(const ElementType &value)
    {
        //自旋锁
        SpinLockGuard<Spin> lockGuard(m_spin);
        //获得锁后将数据压入队列尾部
        if (m_list.size() < m_capacityCap) {
            m_list.push_back(value);
            return true;
        }
        return false;
    }

    bool pushFront(const ElementType &value)
    {
        //自旋锁
        SpinLockGuard<Spin> lockGuard(m_spin);
        //获得锁后，将数据压入队列头部
        if (m_list.size() < m_capacityCap) {
            m_list.push_front(value);
            return true;
        }
        return false;
    }

    ElementType popBack()
    {
        //自旋锁
        SpinLockGuard<Spin> lockGuard(m_spin);
        //获得锁后，进行一下操作
        //队列为空，返回空
        if (m_list.empty()) {
            return nullptr;
        }
        //弹出队列最后一个元素，并且返回该元素
        ElementType value = m_list.back();
        m_list.pop_back();
        return value;
    }

    ElementType popFront()
    {
        //自旋锁
        SpinLockGuard<Spin> lockGuard(m_spin);
        //获得锁后，进行一下操作
        //队列为空，返回空
        if (m_list.empty()) {
            return nullptr;
        }
        //弹出队列第一个元素，并返回该元素
        ElementType value = m_list.front();
        m_list.pop_front();
        return value;
    }

    bool isEmpty() const
    {
        //自旋锁
        SpinLockGuard<Spin> lockGuard(m_spin);
        //获得锁后，返回队列是否为空
        return m_list.empty();
    }

    size_t size() const
    {
        //自旋锁
        SpinLockGuard<Spin> lockGuard(m_spin);
        //获得锁后，返回队列大小
        return m_list.size();
    }

    size_t clear()
    {
        //自旋锁
        SpinLockGuard<Spin> lockGuard(m_spin);
        //获得锁后，清空队列
        size_t size = m_list.size();
        m_list.clear();
        return size;
    }

protected:
    //队列
    std::list<ElementType> m_list;
    //默认大小
    size_t m_capacityCap;
    //线程锁 mutable表示随时可变
    mutable Spin m_spin;
};

} //namespace WCDB

#endif /* concurrent_list_hpp */
