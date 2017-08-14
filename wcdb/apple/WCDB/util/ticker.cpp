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

#include <WCDB/ticker.hpp>
#include <mach/mach_time.h>
#include <mutex>

namespace WCDB {

Ticker::Ticker() : m_base(0)
{
}

//计时
void Ticker::tick()
{
    //获取cpu的tickcount计数值
    uint64_t now = mach_absolute_time();
    //将上次tick到本次tick时间差压入队列
    if (m_base != 0) {
        m_elapses.push_back(now - m_base);
    }
    //更新当前tickcount
    m_base = now;
}

//暂停
void Ticker::pause()
{
    tick();
    m_base = 0;
}

//获取使用的时间
std::vector<double> Ticker::getElapseTimes() const
{
    std::vector<double> result;
    for (const auto &elapse : m_elapses) {
        //转化为秒后压入队列
        result.push_back(secondsFromElapse(elapse));
    }
    return result;
}

//获取使用的时间总和
double Ticker::getElapseTime() const
{
    double result = 0;
    for (const auto &elapse : m_elapses) {
        //转化为秒后压入队列
        result += secondsFromElapse(elapse);
    }
    return result;
}

std::string Ticker::log() const
{
    std::string result;
    int i = 0;
    for (const auto &cost : getElapseTimes()) {
        //打印第i次操作消耗的时间
        result.append(std::to_string(i) + ": " + std::to_string(cost) + "\n");
        ++i;
    }
    return result;
}

//将cup的tick数转换为秒
inline double Ticker::secondsFromElapse(const uint64_t &elapse)
{
    static double s_numer = 0;
    static double s_denom = 0;
    static std::once_flag s_once;
    std::call_once(s_once, []() {
        //时间基准值结构体
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        s_numer = info.numer;
        s_denom = info.denom;
    });
    /*
     这里最重要的是调用mach_timebase_info。我们传递一个结构体以返回时间基准值。最后，一旦我们获取到系统的心跳，我们便能生成一个转换因子。通常，转换是通过分子(info.numer)除以分母(info.denom)。这里我乘了一个1e-9来获取秒数。最后，我们获取两个时间的差值，并乘以转换因子，便得到真实的时间差。
     http://www.cocoachina.com/industry/20130608/6362.html
     */

    //cup的tick数 转换为秒后返回
    return (double) elapse * s_numer / s_denom / 1000 / 1000 / 1000;
}

//计时
ScopedTicker::ScopedTicker(std::shared_ptr<Ticker> &ticker) : m_ticker(ticker)
{
    if (m_ticker) {
        m_ticker->tick();
    }
}

//暂停
ScopedTicker::~ScopedTicker()
{
    if (m_ticker) {
        m_ticker->pause();
    }
}

} //namespace WCDB
