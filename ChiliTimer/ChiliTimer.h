#pragma once
// ChiliTimer.h
// =========================
// 这个类用于测量时间间隔，适用于帧时间计算（如游戏循环）
// =========================

#include <chrono> // 引入 chrono 进行高精度计时

class ChiliTimer
{
public:
    // 构造函数：初始化时记录当前时间
    ChiliTimer() noexcept;

    // 记录时间戳，并返回自上次调用 Mark() 以来经过的时间（秒）
    float Mark() noexcept;

    // 返回当前时间与上次调用 Mark() 之间的时间（不重置时间戳）
    float Peek() const noexcept;

private:
    std::chrono::steady_clock::time_point last; // 记录上一次调用 Mark() 的时间
};