#pragma once
// Time.h — 芯片时序里用到的毫秒级延时。
//
// Windows 上直接调 Sleep，与原厂驱动的时序一致；其他平台用 sleep_for。

#include <cstdint>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <chrono>
#include <thread>
#endif

namespace Port {

inline void SleepMs(uint32_t ms) {
#if defined(_WIN32)
    ::Sleep(ms);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

} // namespace Port
