#pragma once
// 参数表。原代码用 TSAPrmt(模块, 字段) / TSAConstPrmt(模块, 字段) 等宏访问，宏按字段分派到下面几块存储；
// 字段名能从断言文本得到的用原名，其余按用途命名。只列已改写的模块用到的字段。

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Common/Image.h"

namespace Solvers::Tsa::Core {

/// g_tsaPrmtFlash：机型参数表（W273AS2700 由 TSAPrmt_FlashPrmtLoadProject 装入），运行中只读。
struct FlashPrmt {
    uint8_t reserved000[0x18];
    uint8_t displayXAxis;  // 与 dim1Axis 相等时 Dim1 是显示的 X 方向
    uint8_t dim1Axis;
    uint8_t reserved01A[0x3];
    uint8_t dim1Len;  // 列数
    uint8_t dim2Len;  // 行数
    uint8_t reserved01F[0xD];
    uint16_t sensorNum;  // dim1Len × dim2Len
    uint8_t reserved02E[0xA];
    uint8_t reportRateHz;  // 0 时按 8 ms 一帧
    uint8_t reserved039[0x56D];
    uint16_t dim1Res;  // Dim1 方向的显示分辨率
    uint16_t dim2Res;
    uint8_t reserved5AA[0xA];
    uint16_t peakFilterReferenceThreshold;
    uint16_t peakMode3ThresholdMax;
    uint8_t reserved5B8[0x12D8];
};
static_assert(sizeof(FlashPrmt) == 0x1890);
static_assert(offsetof(FlashPrmt, displayXAxis) == 0x018);
static_assert(offsetof(FlashPrmt, dim1Axis) == 0x019);
static_assert(offsetof(FlashPrmt, dim1Len) == 0x01D);
static_assert(offsetof(FlashPrmt, dim2Len) == 0x01E);
static_assert(offsetof(FlashPrmt, sensorNum) == 0x02C);
static_assert(offsetof(FlashPrmt, reportRateHz) == 0x038);
static_assert(offsetof(FlashPrmt, dim1Res) == 0x5A6);
static_assert(offsetof(FlashPrmt, dim2Res) == 0x5A8);
static_assert(offsetof(FlashPrmt, peakFilterReferenceThreshold) == 0x5B4);
static_assert(offsetof(FlashPrmt, peakMode3ThresholdMax) == 0x5B6);

/// g_tsaPrmtFlashConst：由机型参数推导出的常量（TSAConstPrmt_Init）。
struct FlashConstPrmt {
    uint8_t reserved000[0x2A];
    uint16_t dim1NearEdge;  // Dim1 两端的逻辑边缘（Q8.8）
    uint16_t dim1FarEdge;
    uint16_t dim2NearEdge;
    uint16_t dim2FarEdge;
    uint8_t reserved032[0xE];
};
static_assert(sizeof(FlashConstPrmt) == 0x040);
static_assert(offsetof(FlashConstPrmt, dim1NearEdge) == 0x02A);
static_assert(offsetof(FlashConstPrmt, dim1FarEdge) == 0x02C);
static_assert(offsetof(FlashConstPrmt, dim2NearEdge) == 0x02E);
static_assert(offsetof(FlashConstPrmt, dim2FarEdge) == 0x030);

/// g_tsaPrmtDynamic：随场景、噪声、触点状态每帧更新的门限（TSADynPrmt）。
struct DynPrmt {
    int16_t touchThold;
    uint8_t reserved002[0x6];
    int16_t sigThold;
    int16_t edgePeakThold;
    int16_t peakThold;
    int16_t edgePeakTholdLow;
    int32_t znTouchDownThold;
    int32_t znFingerThold;
    int32_t znTouchDownThold2;
    uint8_t reserved01C[0x1];
    uint8_t lowTholdOnDim1FarEdge;  // 峰在 Dim1 远端最后一列时用低门限
    uint8_t lowTholdOnDim2FarEdge;
    uint8_t reserved01F[0x59];
    uint8_t clickSuppressRange;  // 外部像素
    uint8_t reserved079[0x1];
    uint8_t maxClickSuppressRange;
    uint8_t reserved07B[0x1];
    uint8_t flickDistance;
    uint8_t reserved07D[0x3];
    uint8_t zTholdFlags;  // bit1 抬起门限按区域分支；bit2 手套、bit3 指关节模式用按下门限判稳定
    uint8_t reserved081[0x7];
    uint16_t largeMoveDistMm;
    uint8_t reserved08A[0xC];
    uint8_t skipZ8CheckOnInnerEdge;
    uint8_t reserved097[0x9];
};
static_assert(sizeof(DynPrmt) == 0x0A0);
static_assert(offsetof(DynPrmt, touchThold) == 0x000);
static_assert(offsetof(DynPrmt, sigThold) == 0x008);
static_assert(offsetof(DynPrmt, edgePeakThold) == 0x00A);
static_assert(offsetof(DynPrmt, peakThold) == 0x00C);
static_assert(offsetof(DynPrmt, edgePeakTholdLow) == 0x00E);
static_assert(offsetof(DynPrmt, znTouchDownThold) == 0x010);
static_assert(offsetof(DynPrmt, znFingerThold) == 0x014);
static_assert(offsetof(DynPrmt, znTouchDownThold2) == 0x018);
static_assert(offsetof(DynPrmt, lowTholdOnDim1FarEdge) == 0x01D);
static_assert(offsetof(DynPrmt, lowTholdOnDim2FarEdge) == 0x01E);
static_assert(offsetof(DynPrmt, clickSuppressRange) == 0x078);
static_assert(offsetof(DynPrmt, maxClickSuppressRange) == 0x07A);
static_assert(offsetof(DynPrmt, flickDistance) == 0x07C);
static_assert(offsetof(DynPrmt, zTholdFlags) == 0x080);
static_assert(offsetof(DynPrmt, largeMoveDistMm) == 0x088);
static_assert(offsetof(DynPrmt, skipZ8CheckOnInnerEdge) == 0x096);

/// 自适应 IIR 的系数表：位移量在 lightDelta 以下用 lightCoef，heavyDelta 以上用 heavyCoef，中间线性插值。
struct AiirPrmt {
    uint16_t lightDelta;
    uint16_t heavyDelta;
    uint8_t lightCoef;
    uint8_t heavyCoef;
};

/// g_tsaPrmtRam：运行中由各模块按场景改写的参数。
struct RamPrmt {
    uint8_t ctdEdgeTable[0x16];  // CTD 的边缘补偿分段表（CTD 尚未改写，按字节访问）
    AiirPrmt aiir;               // Filter 按触点状态从机型参数里选一组写入
};
static_assert(sizeof(RamPrmt) == 0x1C);
static_assert(offsetof(RamPrmt, aiir) == 0x16);

/// g_tsaPrmtRamConst
struct RamConstPrmt {
    uint8_t reserved000[0x2];
    uint16_t aiirScaledCoefToDeltaRatio;  // 系数插值斜率，Q10：每单位位移增加的系数
};
static_assert(offsetof(RamConstPrmt, aiirScaledCoefToDeltaRatio) == 0x2);

[[nodiscard]] inline FlashPrmt& PrmtFlash() noexcept { return Image::At<FlashPrmt>(0x6bca8a00); }
[[nodiscard]] inline FlashConstPrmt& PrmtFlashConst() noexcept { return Image::At<FlashConstPrmt>(0x6bcab600); }
[[nodiscard]] inline DynPrmt& PrmtDynamic() noexcept { return Image::At<DynPrmt>(0x6bcab680); }
[[nodiscard]] inline RamPrmt& PrmtRam() noexcept { return Image::At<RamPrmt>(0x6bcab720); }
[[nodiscard]] inline RamConstPrmt& PrmtRamConst() noexcept { return Image::At<RamConstPrmt>(0x6bcab664); }

/// 机型名（参数表开头的字符串，TSAPrmt_GetProjectName / TSA_GetPrmtName）。
[[nodiscard]] inline const char* ProjectName() noexcept { return reinterpret_cast<const char*>(&PrmtFlash()); }

/// 机型名是否以给定的 4 个字符开头。
[[nodiscard]] inline bool IsProject(const char* prefix) noexcept { return std::strncmp(ProjectName(), prefix, 4) == 0; }

/// 与 TSA_GetXRes 等相同的判定：dim1Axis 与 displayXAxis 相等时 Dim1 是显示的 X 方向。
[[nodiscard]] inline bool IsDim1DisplayX() noexcept {
    return PrmtFlash().dim1Axis == PrmtFlash().displayXAxis;
}

}  // namespace Solvers::Tsa::Core
