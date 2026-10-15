#include "App/Afe.h"
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
#include "App/Property.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Common/Memory.h"
#include "Extension/HandGesture/HandGesture.h"
#include "Extension/HardwareAnalyzer/HardwareAnalyzer.h"
#include "Prmt/Prmt.h"
#include "WorkAround/BadChannel/BadChannel.h"
#include "WorkAround/RfNoise/RfNoise.h"
#include "WorkAround/ToeSynaBl/ToeSynaBl.h"
#include "Rejection/GripFilter/GripFilter.h"

namespace Solvers::Tsa::Core::Buffer {

namespace {

constexpr int16_t kTimeNone = 0x7fff;
constexpr uint32_t kGripFlagNew = 0x200;   // TouchUnit::gripFlags
constexpr uint16_t kNearEdgeQ8 = 0x80;     // 半格
constexpr uint16_t kAtEdgeQ8 = 0x20;

uint16_t SensorNum() { return PrmtFlash().sensorNum; }

/// 计时加 dt，饱和到 int16。
int16_t AddTime(int16_t t, int dt) { return static_cast<int16_t>(std::clamp(t + dt, -0x8000, 0x7fff)); }

/// 计时加一个帧间隔，饱和到 int16。
int16_t GetNextTime(int16_t t) { return AddTime(t, Static().frameIntervalMs); }

template <class T>
bool Alloc(T*& slot, size_t count) {
    slot = static_cast<T*>(Mem::Allocate(static_cast<uint32_t>(count * sizeof(T))));
    return slot != nullptr;
}

template <class T>
void Release(T*& slot) { Mem::Release(slot); }


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
    const int n = SensorNum();
    const int8_t* subTz = b.subTz;
    int8_t* tzId = b.tzId;
    if (std::all_of(idOfCode.begin(), idOfCode.end(), [](int8_t id) { return id == 0; })) {
        std::fill_n(tzId, n, int8_t{0});
        return;
    }
    for (int i = 0; i < n; ++i) tzId[i] = idOfCode[static_cast<uint8_t>(subTz[i])];
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
    const int dt = Static().frameIntervalMs;
    const int n = SensorNum();
    const uint8_t* zone = b.tzZone;
    int16_t* timeRpt = b.timeRpt;
    for (int i = 0; i < n; ++i) {
        const int16_t next = AddTime(timeRpt[i], dt);  // 先无条件算出，选择表达式里不放读取，循环才能向量化
        timeRpt[i] = reported[zone[i]] == 0 ? next : int16_t{0};
    }
}

// TSABuffer_GripTimeProcess @ 0x6ba8eea1：Dif 超过握持跟踪门限的格累计时间
void GripTimeProcess() {
    const int thold = GripFilter::GetTrackingThold();
    const int dt = Static().frameIntervalMs;
    const int n = SensorNum();
    const int16_t* dif = Buffers().dif;
    int16_t* gripTime = Buffers().gripTime;
    for (int i = 0; i < n; ++i) {
        const int16_t next = std::max<int16_t>(0, AddTime(gripTime[i], dt));
        gripTime[i] = thold < dif[i] ? next : int16_t{0};
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
        !Alloc(b.gripRegion, n) ||
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
    ClearChecked(b.gripStage, cap, n);
    ClearChecked(b.gripRegion, cap, n);
    ClearChecked(b.gripTime, cap * 2, n * 2);
    std::fill_n(b.gripTimeOff, n, kTimeNone);
}

// TSABuffer_PreProcess @ 0x6ba8f37f（SafeBaseline 被裁掉，信号恒不稳定）
// 逐格更新计时（TSABuffer_TzIsMergedHandle @ 0x6ba8ec68 / TSABuffer_TzIsNotMergedHandle @ 0x6ba8ed8c）：
// - 不在区域里：刚离开时抬起计时取一个帧间隔、抬起后最小信号取当前 Dif；否则计时推进（kTimeNone 不动）、
//   最小信号取较小者；
// - 在区域里：刚进入时按下计时清零，否则推进。
// 原代码还记按下期间的最大 Dif 与「触点数增加时仍未进入区域」的次数（后者处理完即清零），都没有模块读，不记。
// 各格互不相关，写成无分支的选择（区域的进出逐格随机）。
void PreProcess() {
    TsaBufferTable& b = Buffers();
    const int dt = Static().frameIntervalMs;
    const int n = SensorNum();
    const uint8_t* zone = b.tzZone;
    const uint8_t* prev = b.tzZonePrev;
    const int16_t* dif = b.dif;
    int16_t* timeOff = b.timeOff;
    int16_t* minSigOff = b.minSigOff;
    int16_t* timeDown = b.timeDown;
    for (int i = 0; i < n; ++i) {
        const bool in = zone[i] != 0;
        const bool was = prev[i] != 0;
        const int16_t off = timeOff[i];
        const int16_t ms = minSigOff[i];
        const int16_t td = timeDown[i];
        const int16_t d = dif[i];
        const int16_t outOff = was ? static_cast<int16_t>(dt) : off == kTimeNone ? off : AddTime(off, dt);
        const int16_t outMs = was ? d : std::min(ms, d);
        const int16_t inTd = was ? AddTime(td, dt) : int16_t{0};
        timeOff[i] = in ? off : outOff;
        minSigOff[i] = in ? ms : outMs;
        timeDown[i] = in ? inTd : td;
    }
    CopyChecked(b.tzZonePrev, GridBufSize(), b.tzZone, SensorNum());
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
    const TsaBufferTable& b = Buffers();
    const int interval = PrmtFlash().gripTimeInterval;
    const int dt = Static().frameIntervalMs;
    const int n = SensorNum();
    const uint8_t* zone = b.tzZone;
    const int16_t* gripTime = b.gripTime;
    const int16_t* timeRpt = b.timeRpt;
    uint8_t* gripStage = b.gripStage;
    int16_t* gripTimeOff = b.gripTimeOff;
    for (int i = 0; i < n; ++i) {
        const uint8_t z = zone[i];
        const bool in = z != 0;
        const int16_t offBase = gripStage[i] != 0 ? int16_t{0} : gripTimeOff[i];
        const int16_t off = in ? gripTimeOff[i] : AddTime(offBase, dt);
        const bool update = zoneLocked[z] == 0 && interval < gripTime[i];
        gripStage[i] = !in ? uint8_t{0} : update ? zoneGrip[z] : gripStage[i];
        gripTimeOff[i] = timeRpt[i] == 0 ? kTimeNone : off;
    }
}

// TSAMem_Free @ 0x6baa0608（SafeBaseline、BaliSensorBroken、侧边触摸的释放被裁掉）
void FreeAll() {
    RfNoise::Free();
    HandGesture::Free();
    ToeSynaBl::Free();
    HardwareAnalyzer::Free();
    BadChannel::Free();
    Property::Free();
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
    return RfNoise::Malloc() && HandGesture::Malloc() && ToeSynaBl::Malloc() && HardwareAnalyzer::Malloc() &&
           BadChannel::Malloc() && Property::Malloc() && Alloc(SideTz(), GridBufSize()) && Malloc() && TpSensor::Malloc() &&
           Cmf::Malloc() && Exception::Malloc() && TouchZone::Malloc() && Afe::Malloc();
}

}  // namespace Solvers::Tsa::Core::Buffer
