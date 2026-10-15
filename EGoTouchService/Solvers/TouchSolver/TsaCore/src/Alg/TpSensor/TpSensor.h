#pragma once
// 传感器修正（原 Signal/TPSensor.c，原厂 src/Alg/TPSensor/TPSensor.c）：
// - 平移表 FlashPrmt::sensorTranslate：个别格的 raw 加平移量、Dif 按 difScale / 1024 缩放；
// - 偏移表 FlashPrmt::sensorOffset：个别格的传感器中心偏移，供 TZ 求质心；
// - 坏点表：BadChannel / HardwareAnalyzer 每帧报上来的坏传感器，用邻格线性外推补偿其 Dif。
// 挖孔（Notch）修正已裁掉，挖孔标志位恒为 0。

#include <cstdint>

namespace Solvers::Tsa::Core::TpSensor {

/// 每格的传感器标志 Flags()[cell]
inline constexpr uint8_t kFlagBrokenMask = 0x0f;  // 坏点类型（BadChannel 2、HardwareAnalyzer 4）
inline constexpr uint8_t kFlagNotch = 0x20;       // 挖孔区域内，不是有效传感器
inline constexpr uint8_t kFlagReusedNotch = 0x40;

[[nodiscard]] bool Malloc();
void Free();
void Init();
/// 清空本帧的坏点表（HardwareAnalyzer 每帧开头调用）。
void Reset();
/// raw 平移（在基线之前）。
void ProcessRaw();
/// 坏点补偿与 Dif 缩放（在 CMF 之后）。
void Process();

[[nodiscard]] uint8_t* Flags() noexcept;
/// 每格传感器中心相对格中心的偏移（1/128 格）。
[[nodiscard]] const int8_t* Dim1Offset() noexcept;
[[nodiscard]] const int8_t* Dim2Offset() noexcept;

/// 在面板内且不在挖孔区。
[[nodiscard]] bool IsSensorButNotNotch(int dim1, int dim2);
/// 在面板内、不在挖孔区且没有被报为坏点。
[[nodiscard]] bool IsSensorValid(int dim1, int dim2);
/// 加入本帧的坏点表；表满（100 个）时返回 false。
bool AddToBrokenSensorList(uint8_t dim1, uint8_t dim2, uint8_t type);
/// 坏点表里有落在区域图 mapTag 区域内的格。
[[nodiscard]] bool IsBrokenSensorInZone(uint8_t mapTag);
/// Dif 缩放的逆运算（Dif × 1024 / difScale）。
[[nodiscard]] int16_t RevertScalingDif(int16_t dif, uint8_t dim1, uint8_t dim2);
/// raw 平移的逆运算，只处理 n 以内的格。
void RevertTranslateRaw(int16_t* raw, uint32_t n);

}  // namespace Solvers::Tsa::Core::TpSensor
