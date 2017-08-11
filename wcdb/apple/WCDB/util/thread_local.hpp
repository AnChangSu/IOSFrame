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

#ifndef thread_local_hpp
#define thread_local_hpp

/*
 * Thread-local storage
 *
 * Since Apple clang support C++11 thread_local from arm64+iOS 8 and armv7+iOS 9, this class can be a replacement.
 *
 * See also: http://stackoverflow.com/questions/28094794/why-does-apple-clang-disallow-c11-thread-local-when-official-clang-supports
 */

#include <pthread.h>

//TSD一键多值技术 线程私有数据(tsd)采用了一种一键多值的技术，即一个键对应多个数值。访问数据时都是通过键值来访问，好像是对一个变量进行访问，其实是在访问不同的数据。
//http://blog.chinaunix.net/uid-27034868-id-3349185.html

//wcdb命名空间
namespace WCDB {

//c++模板 http://www.cnblogs.com/gw811/archive/2012/10/25/2738929.html
template <typename T>
class ThreadLocal {
public:
    ThreadLocal()
    {
        //创建一个键，参数2为一个销毁函数，线程退出时调用
        pthread_key_create(&m_key, [](void *value) { delete (T *) value; });
    }

    T *get()
    {
        //从一个键读取线程私有数据
        T *value = (T *) pthread_getspecific(m_key);
        if (value) {
            return value;
        }
        value = new T;
        //为一个键设置线程私有数据
        pthread_setspecific(m_key, value);
        return value;
    }

    //析构函数   删除一个键
    ~ThreadLocal() { pthread_key_delete(m_key); }

protected:
    pthread_key_t m_key;
};

} //namespace WCDB

#endif /* thread_local_hpp */
