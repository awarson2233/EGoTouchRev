#pragma once
// 射频噪声检测（原厂 src/WorkAround/RFNoise）。逐条扫描线比较相邻帧的原始数据差（PreHwRaw 与 Raw 之差），
// 连同前两帧的差一起看：整条线同向偏移、且画面两半的偏移量大小相近时判为射频噪声。
// 扫描方向随 FlashPrmt::dim1Axis：非 0 时逐行（沿 Dim1）扫，否则逐列（沿 Dim2）扫。HardwareAnalyzer 调用。

#include <cstdint>

namespace Solvers::Tsa::Core::RfNoise {

[[nodiscard]] bool Malloc();
void Free();
void Init();

/// 本帧检测：更新噪声值与噪声线数，返回是否判为射频噪声。
[[nodiscard]] bool Detect();
/// 噪声值超过峰检测门限。
[[nodiscard]] bool IsDetected();
/// 最近一次检测的噪声值（三种帧差里同向偏移最小的那个，取各线最大值）。
[[nodiscard]] int16_t NoiseValue();
/// 最近一次检测中偏移超过 finger 门限的线数。
[[nodiscard]] uint8_t NoisyLineCount();

/// featureFlags2 bit4 置位时，把本帧的 Raw2RawDif 存为上一帧的。
void SavePrevRaw2RawDif();

}  // namespace Solvers::Tsa::Core::RfNoise
