#pragma once
// TSACore 各模块共用的基础类型与容量常量。

#include <cstdint>

namespace Solvers::Tsa::Core {

/// 传感器坐标，Q8.8：高 8 位是列 / 行号，低 8 位是格内的小数部分。
struct Point {
    uint16_t dim1;
    uint16_t dim2;
};

/// 坐标差。
struct SPoint {
    int16_t dim1;
    int16_t dim2;
};

inline constexpr uint8_t kMaxTouchNum = 20;  // TSA_MAX_TOUCH_NUM
inline constexpr uint8_t kMaxPeakNum = 20;   // TSA_MAX_PEAK_NUM

}  // namespace Solvers::Tsa::Core
