#include "App/Buffer.h"

#include <algorithm>
#include <array>
#include <cstdint>

#include "Alg/Cmf/Cmf.h"
#include "Alg/Exception/Exception.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchZone/TouchZone.h"
#include "Alg/TpSensor/TpSensor.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Extension/HardwareAnalyzer/HardwareAnalyzer.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"
#include "WorkAround/BadChannel/BadChannel.h"
#include "WorkAround/RfNoise/RfNoise.h"
#include "WorkAround/ToeSynaBl/ToeSynaBl.h"

namespace Solvers::Tsa::Core::Buffer {

namespace {

constexpr int16_t kTimeNone = 0x7fff;
constexpr uint32_t kGripFlagNew = 0x200;   // TouchUnit::gripFlags
constexpr uint16_t kNearEdgeQ8 = 0x80;     // 半格
constexpr uint16_t kAtEdgeQ8 = 0x20;

uint16_t SensorNum() { return PrmtFlash().sensorNum; }

/// 计时加一个帧间隔，饱和到 int16。
int16_t GetNextTime(int16_t t) {
    return static_cast<int16_t>(std::clamp(t + Static().frameIntervalMs, -0x8000, 0x7fff));
}

template <class T>
bool Alloc(T*& slot, size_t count) {
    slot = static_cast<T*>(TSA_MallocAndInit(static_cast<uint32_t>(count * sizeof(T))));
    return slot != nullptr;
}

template <class T>
void Release(T*& slot) { TSA_Free(reinterpret_cast<int64_t*>(&slot)); }

// TSABuffer_TzIsMergedHandle @ 0x6ba8ec68：格在区域里
void TzIsMergedHandle(uint32_t i) {
    TsaBufferTable& b = Buffers();
    if (b.tzZonePrev[i] == 0) {
        b.timeDown[i] = 0;
        b.maxSigDown[i] = b.dif[i];
        if (PrevTouches().count < Touches().count) ++b.silentTouchNum[i];
    } else {
        b.timeDown[i] = GetNextTime(b.timeDown[i]);
        if (b.maxSigDown[i] < b.dif[i]) b.maxSigDown[i] = b.dif[i];
    }
}

// TSABuffer_TzIsNotMergedHandle @ 0x6ba8ed8c：格不在区域里
void TzIsNotMergedHandle(uint32_t i) {
    TsaBufferTable& b = Buffers();
    if (b.tzZonePrev[i] == 0) {
        if (b.timeOff[i] != kTimeNone) b.timeOff[i] = GetNextTime(b.timeOff[i]);
        if (b.dif[i] < b.minSigOff[i]) b.minSigOff[i] = b.dif[i];
    } else {
        b.timeOff[i] = Static().frameIntervalMs;
        b.minSigOff[i] = b.dif[i];
    }
}

// TSABuffer_TZIDProcess @ 0x6ba8e7ac
// 每个未抬起触点所在格的 SubTZ 编码对应到触点 ID，再按编码给每格填上 ID。
void TzIdProcess() {
    std::array<int8_t, 0x100> idOfCode{};
    const TouchTable& tt = Touches();
    TsaBufferTable& b = Buffers();
    for (uint8_t k = 0; k < tt.count; ++k) {
        if (Touch::IsOff(k)) continue;
        const TouchUnit& u = tt.unit[k];
        const auto code = static_cast<uint8_t>(b.subTz[GridIndex(static_cast<uint8_t>(u.pos.dim1 >> 8), static_cast<uint8_t>(u.pos.dim2 >> 8))]);
        idOfCode[code] = static_cast<int8_t>(u.id);
    }
    idOfCode[0] = 0;
    for (uint16_t i = SensorNum(); i-- != 0;) b.tzId[i] = idOfCode[static_cast<uint8_t>(b.subTz[i])];
}

// TSABuffer_TimeRptProcess @ 0x6ba8e9b0
// 上一帧未抬起且已报过点的触点，其峰所在区域里的格报点计时清零，其余格推进（饱和）。
void TimeRptProcess() {
    std::array<uint8_t, 0x100> reported{};
    const PrevTouchTable& prev = PrevTouches();
    const PrevPeakTable& peaks = PrevPeaks();
    TsaBufferTable& b = Buffers();
    for (uint8_t k = 0; k < prev.count; ++k) {
        const PrevTouchUnit& p = prev.unit[k];
        if (Touch::IsOffEvent(p.event)) continue;
        const PrevPeakEntry& pk = peaks.entry[p.peakIdx];
        const uint8_t zone = b.tzZone[GridIndex(pk.dim1, pk.dim2)];
        if (zone != 0) reported[zone] = (p.persistFlags & 0x20000000) != 0 ? 1 : 0;
    }
    const int16_t dt = Static().frameIntervalMs;
    for (uint16_t i = 0; i < SensorNum(); ++i) {
        if (reported[b.tzZone[i]] == 0) {
            b.timeRpt[i] = static_cast<int16_t>(std::clamp(dt + b.timeRpt[i], -0x8000, 0x7fff));
        } else {
            b.timeRpt[i] = 0;
        }
    }
}

// TSABuffer_GripTimeProcess @ 0x6ba8eea1：Dif 超过握持跟踪门限的格累计时间
void GripTimeProcess() {
    const auto thold = static_cast<int16_t>(GripFilter_GetTrackingThold());
    TsaBufferTable& b = Buffers();
    for (uint16_t i = 0; i < SensorNum(); ++i) {
        b.gripTime[i] = thold < b.dif[i] ? std::max<int16_t>(0, GetNextTime(b.gripTime[i])) : int16_t{0};
    }
}

}  // namespace

// TSABuffer_Malloc @ 0x6ba8d7f6（侧边区域图原在 SideTouch_Malloc 里分配）
bool Malloc() {
    TsaBufferTable& b = Buffers();
    const size_t n = GridBufSize();
    const size_t map = GridMapBufSize();
    if (!Alloc(b.gridIirDif, n) || !Alloc(b.blNormal, n) || !Alloc(b.blGrid, n)) return false;
    b.bl = b.blGrid;
    if (!Alloc(b.tzGrid, n)) return false;
    b.tz = b.tzGrid;
    if (!Alloc(b.sd, n) || !Alloc(b.ssChargerNFilterDif, n) || !Alloc(b.rawStoredSinceSsNoSignal, n) ||
        !Alloc(b.rawStoredSinceMutualSsNotMatch, n) || !Alloc(b.tzPrev, n) || !Alloc(b.tzZonePrev, n) || !Alloc(b.tzZone, n) ||
        !Alloc(b.tzId, n) || !Alloc(b.subTz, n) || !Alloc(b.timeOff, n) || !Alloc(b.timeDown, n) || !Alloc(b.timeRpt, n) ||
        !Alloc(b.minSigOff, n) || !Alloc(b.gripStage, n) || !Alloc(b.gripTime, n) || !Alloc(b.gripTimeOff, n) ||
        !Alloc(b.gripRegion, n) || !Alloc(b.maxSigDown, n) || !Alloc(b.silentTouchNum, n) || !Alloc(PreCmfDif(), n) ||
        !Alloc(b.preDif, n) || !Alloc(b.preHwRaw, n) || !Alloc(b.raw2RawDif, n) || !Alloc(b.raw2RawDifAftCmf, n) ||
        !Alloc(b.commonModeNoise, map) || !Alloc(b.rawCapture, n) || !Alloc(b.rawCaptureNextSide, n) || !Alloc(b.tzAge, n) ||
        !Alloc(b.cleanBaseline, n) || !Alloc(b.preUnnormalizedRaw, n) || !Alloc(b.rawGrid, n)) {
        return false;
    }
    b.raw = b.rawGrid;
    if (!Alloc(b.preCmfRawGrid, n)) return false;
    b.preCmfRaw = b.preCmfRawGrid;
    if (!Alloc(b.difGrid, map)) return false;
    b.dif = b.difGrid;
    return true;
}

// TSABuffer_Free @ 0x6ba8df9f
void Free() {
    TsaBufferTable& b = Buffers();
    Release(b.gridIirDif);
    Release(b.blNormal);
    Release(b.blGrid);
    b.bl = nullptr;
    Release(b.tzGrid);
    b.tz = nullptr;
    Release(b.sd);
    Release(b.ssChargerNFilterDif);
    Release(b.rawStoredSinceSsNoSignal);
    Release(b.rawStoredSinceMutualSsNotMatch);
    Release(b.tzPrev);
    Release(b.tzZonePrev);
    Release(b.tzZone);
    Release(b.tzId);
    Release(b.subTz);
    Release(b.timeOff);
    Release(b.timeDown);
    Release(b.timeRpt);
    Release(b.minSigOff);
    Release(b.gripStage);
    Release(b.gripTime);
    Release(b.gripTimeOff);
    Release(b.gripRegion);
    Release(b.maxSigDown);
    Release(b.silentTouchNum);
    Release(PreCmfDif());
    Release(b.preDif);
    Release(b.preHwRaw);
    Release(b.raw2RawDif);
    Release(b.raw2RawDifAftCmf);
    Release(b.commonModeNoise);
    Release(b.rawCapture);
    Release(b.rawCaptureNextSide);
    Release(b.tzAge);
    Release(b.cleanBaseline);
    Release(b.preUnnormalizedRaw);
    Release(b.preCmfRawGrid);
    Release(b.rawGrid);
    Release(b.difGrid);
}

// TSABuffer_Init @ 0x6ba8e183
void Init() {
    TsaBufferTable& b = Buffers();
    const size_t cap = GridBufSize();
    const size_t n = SensorNum();
    ClearChecked(b.tzPrev, cap, n);
    ClearChecked(SideTz(), cap, cap);
    ClearChecked(b.tzZonePrev, cap, n);
    ClearChecked(b.tzId, cap, n);
    ClearChecked(b.tzZone, cap, n);
    ClearChecked(b.subTz, cap, n);
    std::fill_n(b.timeOff, n, kTimeNone);
    std::fill_n(b.timeRpt, n, kTimeNone);
    ClearChecked(b.timeDown, cap * 2, n * 2);
    ClearChecked(b.minSigOff, cap * 2, n * 2);
    ClearChecked(b.maxSigDown, cap * 2, n * 2);
    ClearChecked(b.silentTouchNum, cap, n);
    ClearChecked(b.gripStage, cap, n);
    ClearChecked(b.gripRegion, cap, n);
    ClearChecked(b.gripTime, cap * 2, n * 2);
    std::fill_n(b.gripTimeOff, n, kTimeNone);
}

// TSABuffer_PreProcess @ 0x6ba8f37f（SafeBaseline 被裁掉，信号恒不稳定）
void PreProcess() {
    TsaBufferTable& b = Buffers();
    for (uint32_t i = SensorNum(); i-- != 0;) {
        if (b.tzZone[i] == 0) {
            TzIsNotMergedHandle(i);
        } else {
            TzIsMergedHandle(i);
        }
    }
    CopyChecked(b.tzZonePrev, GridBufSize(), b.tzZone, SensorNum());
    ClearChecked(b.silentTouchNum, GridBufSize(), SensorNum());
    TzIdProcess();
}

// TSABuffer_PostProcess @ 0x6ba8f4c1
void PostProcess() { CopyChecked(Buffers().tzPrev, GridBufSize(), Buffers().tz, SensorNum()); }

// TSABuffer_PrevFrameProcess @ 0x6ba8f540
void PrevFrameProcess() { TimeRptProcess(); }

// TSABuffer_GripStageProcess @ 0x6ba8ef57
// 每个区域：有握持、刚握持且刚按下、离边缘不到半格、或贴着边缘的触点时判为握持；区域里有 persistFlags4
// bit0 的触点时不更新。握持计时超过 gripTimeInterval 的格才取区域的判定；不在区域里的格清零并推进抬起计时。
void GripStageProcess() {
    GripTimeProcess();
    std::array<uint8_t, 0x100> zoneGrip{};
    std::array<uint8_t, 0x100> zoneLocked{};
    const TouchTable& tt = Touches();
    for (uint8_t k = 0; k < tt.count; ++k) {
        if (Touch::IsOff(k)) continue;
        const TouchUnit& u = tt.unit[k];
        const uint8_t z = u.zoneIdx;
        if (u.gripStage == 1) zoneGrip[z] = 1;
        if ((u.gripFlags & kGripFlagNew) != 0 && u.age == 1) zoneGrip[z] = 1;
        if (u.maxEdgeDistDim1 < kNearEdgeQ8 || u.maxEdgeDistDim2 < kNearEdgeQ8) zoneGrip[z] = 1;
        if (u.edgeDistDim1 < kAtEdgeQ8 || u.edgeDistDim2 < kAtEdgeQ8) zoneGrip[z] = 1;
        zoneLocked[z] = (u.persistFlags4 & 1) != 0 ? 1 : 0;
    }
    TsaBufferTable& b = Buffers();
    const uint16_t interval = PrmtFlash().gripTimeInterval;
    for (uint16_t i = 0; i < SensorNum(); ++i) {
        const uint8_t z = b.tzZone[i];
        if (z == 0) {
            if (b.gripStage[i] != 0) b.gripTimeOff[i] = 0;
            b.gripStage[i] = 0;
            b.gripTimeOff[i] = GetNextTime(b.gripTimeOff[i]);
        } else if (zoneLocked[z] == 0 && static_cast<int>(interval) < b.gripTime[i]) {
            b.gripStage[i] = zoneGrip[z];
        }
        if (b.timeRpt[i] == 0) b.gripTimeOff[i] = kTimeNone;
    }
}

// TSAMem_Free @ 0x6baa0608（SafeBaseline、BaliSensorBroken、侧边触摸的释放被裁掉）
void FreeAll() {
    RfNoise::Free();
    HandGesture_Free();
    ToeSynaBl::Free();
    HardwareAnalyzer::Free();
    BadChannel::Free();
    TSAPrpt_Free();
    Free();
    TpSensor::Free();
    Cmf::Free();
    Exception::Free();
    TouchZone::Free();
}

// TSAMem_InitProject @ 0x6baa065d / TSAMem_Malloc @ 0x6baa04f0
bool InitProject() {
    FreeAll();
    const FlashPrmt& fl = PrmtFlash();
    GridBufSize() = fl.sensorNum;
    GridMapBufSize() = static_cast<uint16_t>((GridBufSize() * 2u + GridBufSize()) / 2);
    LineBufSize() = static_cast<uint16_t>(fl.dim1Len + fl.dim2Len);
    MaxDimSize() = std::max(fl.dim1Len, fl.dim2Len);
    return RfNoise::Malloc() && HandGesture_Malloc() != 0 && ToeSynaBl::Malloc() && HardwareAnalyzer::Malloc() &&
           BadChannel::Malloc() && TSAPrpt_Malloc() && Alloc(SideTz(), GridBufSize()) && Malloc() && TpSensor::Malloc() &&
           Cmf::Malloc() && Exception::Malloc() && TouchZone::Malloc() && AFE_Malloc() != 0;
}

}  // namespace Solvers::Tsa::Core::Buffer

// raw 的调用方
extern "C" {
namespace B = Solvers::Tsa::Core::Buffer;
bool TSAMem_InitProject(void) { return B::InitProject(); }
void TSAMem_Free(void) { B::FreeAll(); }
void TSABuffer_Init(void) { B::Init(); }
void TSABuffer_PreProcess(void) { B::PreProcess(); }
void TSABuffer_PostProcess(void) { B::PostProcess(); }
void TSABuffer_PrevFrameProcess(void) { B::PrevFrameProcess(); }
void TSABuffer_GripStageProcess(void) { B::GripStageProcess(); }
}  // extern "C"
