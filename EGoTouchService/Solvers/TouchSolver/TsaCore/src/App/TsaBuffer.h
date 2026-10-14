#pragma once
// 网格缓冲（TSA_Buffer）：每个传感器一个元素的数组，指针集中放在 0x6bb81280 起的一张表里，
// 由 TSABuffer 模块分配，并在 grid / hand / side 之间切换。

#include <cstddef>
#include <cstdint>

#include "Common/Image.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core {

/// 0x6bb81280 起的网格缓冲指针（TSABuffer 模块分配与切换）。元素按传感器 dim2 × dim1Len + dim1 排列。
/// 元素类型尚未确定的先写成 void*，改写到对应模块时再定。
struct TsaBufferTable {
    int16_t* timeOff;  // 每格自抬起以来的时间
    int16_t* timeDown;  // 每格自按下以来的时间
    int16_t* timeRpt;  // 每格自开始报点以来的时间
    int16_t* minSigOff;  // 每格抬起后的最小信号
    void* maxSigDown;
    void* silentTouchNum;
    void* unnormalizedRaw;
    void* preUnnormalizedRaw;
    void* rawGrid;
    void* difGrid;
    void* preCmfRawGrid;
    void* cleanBaseline;
    uint16_t* tzAge;  // 每格所在握持区域的年龄（gripFlags 0x200）
    void* rawCapture;
    void* rawCaptureNextSide;
    int16_t* dif;  // 当前帧 Dif
    int16_t* raw;
    void* preCmfRaw;
    int16_t* bl;
    int8_t* tz;  // 区域图：每格的区域编码，见 Alg/Sensor/Sensor.h
    void* commonModeNoise;
    int16_t* preHwRaw;
    int16_t* raw2RawDif;
    void* raw2RawDifAftCmf;
    void* preDif;
    void* gridIirDif;
    void* blGrid;
    int16_t* blNormal;
    int8_t* tzGrid;
    void* sd;
    void* ssChargerNFilterDif;
    void* aftSsCmfDif;
    void* rawStoredSinceSsNoSignal;
    void* rawStoredSinceMutualSsNotMatch;
    int8_t* tzPrev;  // 上一帧的区域图
    uint8_t* tzZonePrev;  // 上一帧的 tzZone
    uint8_t* tzZone;  // 每格所属区域的编码基数（TouchZoneUnit::mapTag），只记强信号格
    int8_t* tzId;  // 每格上一帧所属触点的 ID
    int8_t* subTz;  // SubTZ 的遍历标记：(编码 + 1) × Dif 符号，0 为未访问
    uint8_t* gripStage;  // 每格的握持阶段，1 为握持
    int16_t* gripTime;  // 每格进入握持以来的时间
    int16_t* gripTimeOff;
    uint8_t* gripRegion;
};
static_assert(sizeof(TsaBufferTable) == 0x158);
static_assert(offsetof(TsaBufferTable, timeOff) == 0x000);
static_assert(offsetof(TsaBufferTable, timeDown) == 0x008);
static_assert(offsetof(TsaBufferTable, timeRpt) == 0x010);
static_assert(offsetof(TsaBufferTable, minSigOff) == 0x018);
static_assert(offsetof(TsaBufferTable, maxSigDown) == 0x020);
static_assert(offsetof(TsaBufferTable, silentTouchNum) == 0x028);
static_assert(offsetof(TsaBufferTable, unnormalizedRaw) == 0x030);
static_assert(offsetof(TsaBufferTable, preUnnormalizedRaw) == 0x038);
static_assert(offsetof(TsaBufferTable, rawGrid) == 0x040);
static_assert(offsetof(TsaBufferTable, difGrid) == 0x048);
static_assert(offsetof(TsaBufferTable, preCmfRawGrid) == 0x050);
static_assert(offsetof(TsaBufferTable, cleanBaseline) == 0x058);
static_assert(offsetof(TsaBufferTable, tzAge) == 0x060);
static_assert(offsetof(TsaBufferTable, rawCapture) == 0x068);
static_assert(offsetof(TsaBufferTable, rawCaptureNextSide) == 0x070);
static_assert(offsetof(TsaBufferTable, dif) == 0x078);
static_assert(offsetof(TsaBufferTable, raw) == 0x080);
static_assert(offsetof(TsaBufferTable, preCmfRaw) == 0x088);
static_assert(offsetof(TsaBufferTable, bl) == 0x090);
static_assert(offsetof(TsaBufferTable, tz) == 0x098);
static_assert(offsetof(TsaBufferTable, commonModeNoise) == 0x0A0);
static_assert(offsetof(TsaBufferTable, preHwRaw) == 0x0A8);
static_assert(offsetof(TsaBufferTable, raw2RawDif) == 0x0B0);
static_assert(offsetof(TsaBufferTable, raw2RawDifAftCmf) == 0x0B8);
static_assert(offsetof(TsaBufferTable, preDif) == 0x0C0);
static_assert(offsetof(TsaBufferTable, gridIirDif) == 0x0C8);
static_assert(offsetof(TsaBufferTable, blGrid) == 0x0D0);
static_assert(offsetof(TsaBufferTable, blNormal) == 0x0D8);
static_assert(offsetof(TsaBufferTable, tzGrid) == 0x0E0);
static_assert(offsetof(TsaBufferTable, sd) == 0x0E8);
static_assert(offsetof(TsaBufferTable, ssChargerNFilterDif) == 0x0F0);
static_assert(offsetof(TsaBufferTable, aftSsCmfDif) == 0x0F8);
static_assert(offsetof(TsaBufferTable, rawStoredSinceSsNoSignal) == 0x100);
static_assert(offsetof(TsaBufferTable, rawStoredSinceMutualSsNotMatch) == 0x108);
static_assert(offsetof(TsaBufferTable, tzPrev) == 0x110);
static_assert(offsetof(TsaBufferTable, tzZonePrev) == 0x118);
static_assert(offsetof(TsaBufferTable, tzZone) == 0x120);
static_assert(offsetof(TsaBufferTable, tzId) == 0x128);
static_assert(offsetof(TsaBufferTable, subTz) == 0x130);
static_assert(offsetof(TsaBufferTable, gripStage) == 0x138);
static_assert(offsetof(TsaBufferTable, gripTime) == 0x140);
static_assert(offsetof(TsaBufferTable, gripTimeOff) == 0x148);
static_assert(offsetof(TsaBufferTable, gripRegion) == 0x150);

[[nodiscard]] inline TsaBufferTable& Buffers() noexcept { return Image::At<TsaBufferTable>(0x6bb81280); }

/// 网格缓冲的元素个数（g_gridBufSize）。
[[nodiscard]] inline uint16_t GridBufSize() noexcept { return Image::At<uint16_t>(0x6bc34a80); }

/// 传感器 (dim1, dim2) 在网格缓冲里的下标，按 16 位截断。
[[nodiscard]] inline uint16_t GridIndex(uint8_t dim1, uint8_t dim2) noexcept {
    return static_cast<uint16_t>(dim1 + dim2 * static_cast<uint32_t>(PrmtFlash().dim1Len));
}

}  // namespace Solvers::Tsa::Core
