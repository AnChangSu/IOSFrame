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

#ifndef recyclable_hpp
#define recyclable_hpp

#include <functional>

//wcdb命名空间
namespace WCDB {

//类模板
template <typename T>
class Recyclable {
public:
    //std::function可以绑定到全局函数/类静态成员函数(类静态成员函数与全局函数没有区别),如果要绑定到类的非静态成员函数，则需要使用std::bind
    typedef std::function<void(T &)> OnRecycled;
    static const Recyclable inValid;

    //构造函数 使用value和函数指针构造
    Recyclable(const T &value, const Recyclable::OnRecycled &onRecycled)
        : m_value(value)
        , m_onRecycled(onRecycled)
        , m_reference(new std::atomic<int>(0))
    {
        retain();
    }

    //构造函数 使用其他对象构造
    Recyclable(const Recyclable &other)
        : m_value(other.m_value)
        , m_onRecycled(other.m_onRecycled)
        , m_reference(other.m_reference)
    {
        retain();
    }

    //重载=运算符，用其他对象赋值本对象
    Recyclable &operator=(const Recyclable &other)
    {
        other.retain();
        release();
        m_value = other.m_value;
        m_onRecycled = other.m_onRecycled;
        m_reference = other.m_reference;
        return *this;
    }

    //enable_if 的主要作用就是当某个 condition 成立时，enable_if可以提供某种类型。
    //std::is_convertible 检测是否有继承关系，是返回true 否则返回false
    //std::nullptr_t可以显式或隐式地转换为任何指针（包括类的成员函数指针），但不能显式或隐式地转换为任何其他类型。
    typename std::enable_if<std::is_convertible<std::nullptr_t, T>::value,
                            Recyclable &>::type
    operator=(const std::nullptr_t &)
    {
        release();
        m_value = nullptr;
        m_reference = nullptr;
        m_onRecycled = nullptr;
        return *this;
    }

    //析构函数
    ~Recyclable() { release(); }

    //重载->()运算法
    constexpr T operator->() const { return m_value; }

    //重载!=运算符
    typename std::enable_if<std::is_convertible<std::nullptr_t, T>::value,
                            bool>::type
    operator!=(const std::nullptr_t &) const
    {
        return m_value != nullptr;
    }

    //重载==运算符
    typename std::enable_if<std::is_convertible<std::nullptr_t, T>::value,
                            bool>::type
    operator==(const std::nullptr_t &) const
    {
        return m_value == nullptr;
    }

protected:
    Recyclable() //invalid
        : m_reference(new std::atomic<int>(0))
    {
    }

    //引用计数+1
    void retain() const
    {
        if (m_reference) {
            ++(*m_reference);
        }
    }
    //引用计数-1  为0时释放
    void release()
    {
        if (m_reference) {
            if (--(*m_reference) == 0) {
                delete m_reference;
                m_reference = nullptr;
                //如果有回收函数，回收value
                if (m_onRecycled) {
                    m_onRecycled(m_value);
                    m_onRecycled = nullptr;
                }
            }
        }
    }

    T m_value;
    //std::atomic对int, char, bool等数据结构进行原子性封装，在多线程环境中，对std::atomic对象的访问不会造成竞争-冒险。利用std::atomic可实现数据结构的无锁设计。
    //引用计数
    mutable std::atomic<int> *m_reference;
    //回收函数
    Recyclable::OnRecycled m_onRecycled;
};

} //namespace WCDB

#endif /* recyclable_hpp */
