

// ChiliTimer.cpp
// =========================
// 这个文件实现 ChiliTimer 类的方法
// =========================

#include "ChiliTimer.h"

using namespace std::chrono;

// 构造函数：在创建 ChiliTimer 实例时，记录当前时间
ChiliTimer::ChiliTimer() noexcept
{
    last = steady_clock::now(); // 记录当前时间点
}

// Mark(): 计算自上次调用 Mark() 以来经过的时间，并更新时间戳
float ChiliTimer::Mark() noexcept
{
    const auto old = last; // 备份旧的时间点
    last = steady_clock::now(); // 记录新的时间点
    const duration<float> frameTime = last - old; // 计算时间差（单位：秒）
    return frameTime.count(); // 返回时间差（浮点数，单位：秒）
}

// Peek(): 计算当前时间与上次调用 Mark() 之间的时间（不重置时间戳）
float ChiliTimer::Peek() const noexcept
{
    return duration<float>(steady_clock::now() - last).count(); // 返回时间差
}

