#pragma once
// 互容 Dif 的线统计（原 Signal/Self.c，原厂 src/Alg/SelfData/SelfData.c）：每条扫描线一组和、极值与强信号统计，
// 下标 [0, dim1Len) 是各列（沿 Dim2 求和），[dim1Len, dim1Len + dim2Len) 是各行（沿 Dim1 求和）。
// 基线复位判定读的是 TSA_MSProcess 每帧拷下的副本（SelfForBlReset）。

#include <cstddef>
#include <cstdint>

#include "Common/Bss.h"

namespace Solvers::Tsa::Core {

inline constexpr int kMaxLineNum = 160;  // 列数 + 行数的上限

/// g_tsaBufSelf / g_tsaBufSelfForBlRst。强信号指 Dif 不小于 0x4b 或小于 -0x4a；inner 不含线的两端格。
struct SelfData {
    int32_t sum[kMaxLineNum];                // 全部格的 Dif 之和
    int32_t negSum[kMaxLineNum];             // 负 Dif 之和
    int32_t posSum[kMaxLineNum];             // 非负 Dif 之和
    int16_t reserved780[kMaxLineNum];
    int16_t maxAbs[kMaxLineNum];             // |Dif| 的最大值
    int32_t strongSum[kMaxLineNum];          // 强信号之和（正负相抵）
    int32_t innerStrongSum[kMaxLineNum];
    int32_t strongAbsSum[kMaxLineNum];       // 强信号的绝对值之和
    int32_t innerStrongAbsSum[kMaxLineNum];
    int32_t field1400[kMaxLineNum];          // TSAFold 的输入（TSAFold 被裁掉）；TZ 借来记每线遍历到的信号和
    int16_t commonNoise[2 * kMaxLineNum];    // CMF：游戏场景下各行的公共噪声（下标同上，只写行）
    int32_t field1900[kMaxLineNum];          // TZ 借来记每线未达门限的格数
};
static_assert(sizeof(SelfData) == 0x1b80);
static_assert(offsetof(SelfData, reserved780) == 0x780);
static_assert(offsetof(SelfData, maxAbs) == 0x8c0);
static_assert(offsetof(SelfData, strongSum) == 0xa00);
static_assert(offsetof(SelfData, innerStrongAbsSum) == 0x1180);
static_assert(offsetof(SelfData, commonNoise) == 0x1680);
static_assert(offsetof(SelfData, field1900) == 0x1900);

[[nodiscard]] inline SelfData& SelfBuf() noexcept { return Slot<SelfData>(Bss().bufSelf); }
[[nodiscard]] inline SelfData& SelfForBlReset() noexcept { return Slot<SelfData>(Bss().bufSelfForBlRst); }

namespace Self {

void Process();
void Reset();

}  // namespace Self

}  // namespace Solvers::Tsa::Core
