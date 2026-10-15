// 每帧流程（TZ_StateMachine.c）与区域生长门限（TZ_SigThold）。

#include <algorithm>
#include <array>
#include <cstring>

#include "Alg/CTD/Ctd.h"
#include "Alg/Exception/Exception.h"
#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Sensor/Sensor.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TpSensor/TpSensor.h"
#include "Alg/TouchZone/SubTouchZone.h"
#include "Alg/TouchZone/TouchZone.h"
#include "Alg/TouchZone/TouchZoneInternal.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Memory.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TouchZone {

namespace {

constexpr size_t kMaxSensorNum = 0x1324;       // tz / subTz / 膨胀腐蚀结果缓冲的容量
constexpr int8_t kMorphologyThold = 0x14;      // 区域图编码不小于它的格子算在区域里
constexpr uint8_t kZoneAgeSlots = 30;          // 区号 0..29 各记一个最大年龄
constexpr uint16_t kNewAge = 2;
constexpr uint32_t kExcFloating = 0x8;         // 全局异常：悬浮
constexpr uint32_t kFeatStylus = 0x1000;

/// g_tsaDistanceMapBuf：TZ_Transform 的距离图；g_tsaFlagAfterDilate / Erode：区域图膨胀 / 腐蚀一格后的结果。
int8_t* DistanceMap() { return &Image::At<int8_t>(0x6bc34ac0); }
int8_t* FlagAfterDilate() { return &Image::At<int8_t>(0x6bc2ae00); }
int8_t* FlagAfterErode() { return &Image::At<int8_t>(0x6bc2e7c0); }

const PeakEntry& PeakAt(uint8_t idx) { return Peaks().entry[idx]; }

// ── 区域生长门限 ─────────────────────────────────────────────────────────

/// 峰值，不超过 peakFilterReferenceThreshold。
int RefSig(uint8_t peak) { return std::min(PrmtFlash().peakFilterReferenceThreshold, PeakAt(peak).sig); }

int16_t Scale128(uint8_t mult, int sig) { return static_cast<int16_t>(mult * sig >> 7); }

// TZ_IsSD @ 0x6ba8a67e
// 设备悬浮（SD）：全局峰值不高、上一帧全局异常为悬浮，或对应的历史触点是长大手指、已报点且信号刻度低。
bool IsSd(const TouchZoneUnit& z) {
    if (PrmtFlash().peakFilterReferenceThreshold * 2 < Prpt().dif.maxSig * 3) return false;
    const PrevTouchTable& prev = PrevTouches();
    if (prev.exceptionFlags == kExcFloating) return true;
    if (z.prevIdx == kNoTouch) return false;
    const PrevTouchUnit& u = prev.unit[z.prevIdx];
    return u.maxMajorMm >= 0x14 && u.zoneShape == static_cast<int32_t>(kShapeFatFinger) && (u.volatileFlags2 & 0x1000) != 0 &&
           PrmtDynamic().touchThold * 2 > u.signalScale * 3;
}

// TZ_GetSigTholdCommon @ 0x6ba8a7e4
// 外边缘的峰（设备不悬浮时）与 stylus 模式按 sigMultNormal，否则按 sigMult。
int16_t SigTholdCommon(const TouchZoneUnit& z, uint8_t peak) {
    const FlashPrmt& fl = PrmtFlash();
    const int ref = RefSig(peak);
    if ((Static().featureFlags & kFeatStylus) != 0) return Scale128(fl.sigMultNormal, ref);
    if (Peak::IsOnOuterEdge(peak) && !IsSd(z)) return Scale128(fl.sigMultNormal, ref);
    return Scale128(fl.sigMult, ref);
}

// TZ_GetSigTholdForCharger @ 0x6ba8a5d3
int16_t SigTholdForCharger(uint8_t peak) {
    const int sig = PeakAt(peak).sig;
    if (PrmtFlash().sensorArchitecture == 3) return Scale128(PrmtFlash().sigMultNormal, sig);
    return static_cast<int16_t>(sig / 2);
}

// TZ_GetSigThold @ 0x6ba8a90b（及 ForSideTouch @ 0x6ba8a480、ForAFT @ 0x6ba8a51f）
// 按模式取峰值的一个比例，再夹在 [噪声门限, 峰值] 之间。zone 是上一个区域留下的单元（只看它的 prevIdx）。
int16_t GetSigThold(const TouchZoneUnit& z, uint8_t peak) {
    const FlashPrmt& fl = PrmtFlash();
    int16_t t;
    if ((fl.gripFlags & kGripFixedSigThold) != 0) {
        t = fl.gripModeSigThold;
    } else if ((Static().processingState & kProcIdleScan) != 0) {
        t = static_cast<int16_t>(RefSig(peak) >> 1);
    } else if (static_cast<uint8_t>(Sys::IsAftModeEnabled()) != 0) {
        t = static_cast<int16_t>(RefSig(peak) >> (Peak::NeedUdfpLargeFingerWorkaround() ? 1 : 2));
    } else if (PrevTouches().exceptionFlags == Exception::kNoiseLevel2 || Static().noiseLevel == 2 ||
               Sys::IsChargerPlugged() != 0) {
        t = SigTholdForCharger(peak);
    } else {
        t = SigTholdCommon(z, peak);
    }
    t = std::max(t, Prpt().noiseThold);
    return std::min(t, PeakAt(peak).sig);
}

// ── 一个区域 ────────────────────────────────────────────────────────────

// TZ_PeakInfoRetrieval @ 0x6ba8acb6
// 生长覆盖到的、尚未归入别的区域的峰登记到本区域。
void PeakInfoRetrieval(TouchZoneUnit& z) {
    const int8_t* map = TzAccess().map;
    const PeakTable& pk = Peaks();
    for (uint8_t i = 0; i < pk.count; ++i) {
        const uint32_t bit = 1u << i;
        if ((PeakAccessed() & bit) != 0 || map[GridIndex(pk.entry[i].dim1, pk.entry[i].dim2)] == 0) continue;
        PeakAccessed() |= bit;
        PeakRegister(z, i);
    }
    GetMaxMinPkIdx(z);
}

// TZ_BeforeSDCompensationPeakInfoRetrieval @ 0x6ba8adec
// 同样地统计切换 Dif 前的峰表：覆盖到几个、其中峰值最大的一个。
void SdcPeakInfoRetrieval(TouchZoneUnit& z) {
    const int8_t* map = TzAccess().map;
    const PeakTable& pk = PeaksBeforeSwitchDif();
    int16_t best = INT16_MIN;
    z.sdcPeakCount = 0;
    for (uint8_t i = 0; i < pk.count; ++i) {
        const uint32_t bit = 1u << i;
        if ((SdcPeakAccessed() & bit) != 0 || map[GridIndex(pk.entry[i].dim1, pk.entry[i].dim2)] == 0) continue;
        SdcPeakAccessed() |= bit;
        ++z.sdcPeakCount;
        if (best < pk.entry[i].sig) {
            best = pk.entry[i].sig;
            z.sdcMaxPeakIdx = i;
        }
    }
}

// TZ_IsValid @ 0x6ba8aaca
// 有峰；互容主流程中还要覆盖到切换 Dif 前的峰。
bool IsValid(const TouchZoneUnit& z) {
    if (z.peakCount == 0) return false;
    if ((Static().processingState & kProcMsActive) == 0) return true;
    return z.sdcPeakCount != 0;
}

// TZ_GetPrevTouch @ 0x6ba8ab23
// 对应的历史触点：最强峰有 ID 时按它，否则按第一个有 ID 的峰；都没有时按覆盖到的上一帧触点 ID。
void GetPrevTouch(TouchZoneUnit& z) {
    const PeakTable& pk = Peaks();
    z.prevIdx = kNoTouch;
    if (pk.entry[z.maxSigPeakIdx].id != 0) {
        z.prevIdx = PrevTouch::GetIdxBasedOnId(pk.entry[z.maxSigPeakIdx].id);
    } else {
        for (uint8_t k = 0; k < z.peakCount; ++k) {
            const uint8_t id = pk.entry[z.peakIdx[k]].id;
            if (id != 0) {
                z.prevIdx = PrevTouch::GetIdxBasedOnId(id);
                break;
            }
        }
    }
    if (z.prevIdx == kNoTouch && z.mergedId != 0) z.prevIdx = PrevTouch::GetIdxBasedOnId(z.mergedId);
}

// TZ_GetProperty @ 0x6ba8b126
void GetProperty(TouchZoneUnit& z) {
    if (TpSensor::IsBrokenSensorInZone(z.mapTag)) z.flags |= kZoneBrokenSensor;
    CentroidCore(z);
    GetEdgeTouchedFlag(z);
    GetHtiValidFlag(z);
    GetAxis(z);
    GetShapeRatio(z);
    GetGripRatio(z);
    GetContour(z);
    if ((Static().processingState & kProcMsActive) != 0) {
        if ((PrmtFlash().gripFlags & kGripNoZoneException) == 0) Exception::ProcessTouchZone(z);
        if ((Static().featureFlags & 0x20) != 0) CheckToSeparateZone(z);
        SubTouchZone::Process(z);
        GetShapeFlag(z);
        GetSurroundingStatusFlag(z);
        GetCmfFlag(z);
        if ((z.flags & kZoneGripSeparated) == 0) {
            VpProcess(z);
            if ((z.flags & kZoneVirtualPeak) != 0) VirtualPeakInfoRetrieval();
        }
    }
    GetType(z);
}

// TZ_UnitProcess @ 0x6ba8b236
void UnitProcess(TouchZoneUnit& z) {
    PeakInfoRetrieval(z);
    SdcPeakInfoRetrieval(z);
    if (!IsValid(z)) return;
    GetPrevTouch(z);
    GetProperty(z);
    UnitToTouch(z);
}

// TZ_NormalTraversal @ 0x6ba8baec
// 峰离屏幕边不到 2 格时不做质心权重的偏移。
void NormalTraversal(TouchZoneUnit& z, uint8_t peak) {
    const int16_t thold = GetSigThold(z, peak);
    UnitInit(z);
    const PeakEntry& p = PeakAt(peak);
    const FlashPrmt& fl = PrmtFlash();
    z.peakId = p.id;
    z.fatFingerSD = 0;  // TZ_CheckFatFingerSD 需要 SS（自容），本机型裁掉了，恒为 0
    z.seedPeakIdx = peak;
    const bool nearBorder = p.dim1 < 2 || fl.dim1Len <= p.dim1 + 2 || p.dim2 < 2 || fl.dim2Len <= p.dim2 + 2;
    PeakBasedTraversal(z, thold, p.dim1, p.dim2, (fl.gripFlags & kGripCentroidSigOffset) != 0 && !nearBorder);
    z.mapTag = TzAccess().tag;
    UnitProcess(z);
    TzAccess().tag += 8;  // 下一个区域用下一档编码
}

// TZ_SimpleTraversal @ 0x6ba8b88f
// 只求质心、椭圆轴、握持比与贴边，不判形状。
void SimpleTraversal(TouchZoneUnit& z, uint8_t peak) {
    const int16_t thold = GetSigThold(z, peak);
    UnitInit(z);
    const PeakEntry& p = PeakAt(peak);
    z.peakId = p.id;
    PeakBasedTraversalSimple(z, thold, p.dim1, p.dim2);
    z.mapTag = TzAccess().tag;
    CentroidCore(z);
    GetAxis(z);
    GetGripRatio(z);
    GetEdgeTouchedFlag(z);
    UnitToTouch(z);
    TzAccess().tag += 8;  // 下一个区域用下一档编码
}

// TZ_CentrodToTouch @ 0x6ba8b9bf
// 不生长：峰周围 3×3 的质心，信号和取峰值 + Z8。
void CentroidToTouch(TouchZoneUnit& z, uint8_t peak) {
    UnitInit(z);
    const PeakEntry& p = PeakAt(peak);
    Ctd::General(p.dim1, p.dim2, 3, 3, &z.centroidDim1, &z.centroidDim2);
    z.sigSum = p.z8 + p.sig;
    z.gripRatioDim1 = 0x10;
    z.gripRatioDim2 = 0x10;
    UnitToTouch(z);
}

// ── 区域图的后处理 ───────────────────────────────────────────────────────

// TZ_Transform @ 0x6ba8ca70
// 两遍扫描求每格到最近的区域格（erode 时为非区域格）的曼哈顿距离，再按距离是否超过 radius 重写 img：
// 膨胀时距离不超过 radius 的格子记 1，腐蚀时距离超过 radius 的格子记 1。
void Transform(int8_t* img, int8_t thold, bool erode, int8_t radius) {
    const FlashPrmt& fl = PrmtFlash();
    const uint32_t cols = fl.dim1Len;
    const uint32_t rows = fl.dim2Len;
    int8_t* dist = DistanceMap();
    auto at = [&](uint32_t c, uint32_t r) { return static_cast<uint16_t>(cols * r + c); };
    auto relax = [&](uint16_t idx, uint16_t from) {
        if (dist[from] + 1 < dist[idx]) dist[idx] = static_cast<int8_t>(dist[from] + 1);
    };
    for (int c = static_cast<int>(cols) - 1; c >= 0; --c) {
        for (int r = static_cast<int>(rows) - 1; r >= 0; --r) {
            const uint16_t idx = at(c, r);
            const bool inside = erode ? img[idx] < thold : thold <= img[idx];
            if (inside) {
                dist[idx] = 0;
                continue;
            }
            dist[idx] = static_cast<int8_t>(rows + cols);
            if (c + 1 < static_cast<int>(cols)) relax(idx, at(c + 1, r));
            if (r + 1 < static_cast<int>(rows)) relax(idx, at(c, r + 1));
        }
    }
    for (int c = 0; c < static_cast<int>(cols); ++c) {
        for (int r = 0; r < static_cast<int>(rows); ++r) {
            const uint16_t idx = at(c, r);
            if (c > 0) relax(idx, static_cast<uint16_t>(idx - 1));
            if (r > 0) relax(idx, at(c, r - 1));
            img[idx] = (radius < dist[idx]) == erode ? 1 : 0;
        }
    }
}

// TZ_DilateAndErode @ 0x6ba8bcf4
// 区域图（编码不小于 0x14 的格子）分别膨胀、腐蚀一格，结果供后续模块使用。
void DilateAndErode() {
    const uint16_t n = PrmtFlash().sensorNum;
    for (int8_t* dst : {FlagAfterDilate(), FlagAfterErode()}) {
        TsaAssert(n <= kMaxSensorNum);
        if (n <= kMaxSensorNum) std::memcpy(dst, Buffers().tz, n);
        Transform(dst, kMorphologyThold, dst == FlagAfterErode(), 1);
    }
}

// TZ_UpdateGripFlagDim @ 0x6ba8b70a
// 握持年龄模式下补上首 / 末列：未被区域覆盖、|Dif| 超过 finger 门限的格子记为类别 7；
// 从强格走到弱格时 tag 前进一档，让下一段强格成为另一个区域。
void UpdateGripFlagDim(int8_t* map, uint8_t dim1) {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    int16_t prev = INT16_MIN;
    for (uint8_t r = 0; r < fl.dim2Len; ++r) {
        const uint16_t cell = GridIndex(dim1, r);
        const int16_t a = Abs16(dif[cell]);
        if (map[cell] == 0) {
            if (fl.fingerThold < a) {
                map[cell] = MapCode(7, dif[cell]);
            } else if (fl.fingerThold < prev) {
                TzAccess().tag += 8;
            }
        }
        prev = a;
    }
}

// TZ_UpdateGripFlag @ 0x6ba8b852
void UpdateGripFlag(int8_t* map) {
    UpdateGripFlagDim(map, 0);
    UpdateGripFlagDim(map, static_cast<uint8_t>(PrmtFlash().dim1Len - 1));
}

// TZ_PeakBasedProcess @ 0x6ba8bffd
// 清空区域图与 SubTZ 的遍历标记，复位访问句柄与峰的归属，然后以每个尚未被覆盖的峰为起点生长区域。
// 各区域共用同一个单元：下一个区域的生长门限仍看上一个区域留下的 prevIdx。
void PeakBasedProcess() {
    const FlashPrmt& fl = PrmtFlash();
    TsaBufferTable& buf = Buffers();
    const bool ms = (Static().processingState & kProcMsActive) != 0;
    TouchZoneUnit zone;
    ZeroFill(zone);
    ClearChecked(buf.tz, kMaxSensorNum, fl.sensorNum);
    if (ms) {
        ClearChecked(buf.subTz, kMaxSensorNum, fl.sensorNum);
        ClearChecked(buf.tzZone, GridBufSize(), fl.sensorNum);
    }
    TzAccess().tag = 0x10;
    TzAccess().map = buf.tz;
    CurrentAccess() = &TzAccess();
    SubTzAccess().map = buf.subTz;
    SubTzAccess().tag = 0x10;
    PeakAccessed() = 0;
    SubTouchZone::PeakAccessed() = 0;
    SdcPeakAccessed() = 0;
    for (uint8_t i = 0; i < Peaks().count; ++i) {
        const PeakEntry& p = PeakAt(i);
        const uint16_t cell = GridIndex(p.dim1, p.dim2);
        if (buf.tz[cell] != 0 || (ms && buf.subTz[cell] != 0)) continue;
        if ((fl.gripFlags & kGripSimpleTraversal) != 0) {
            SimpleTraversal(zone, i);
        } else if ((fl.gripFlags & kGripCentroidOnly) != 0) {
            CentroidToTouch(zone, i);
        } else {
            NormalTraversal(zone, i);
        }
    }
    if ((fl.gripFlags & kGripAge) != 0) UpdateGripFlag(buf.tz);
    DilateAndErode();
}

// ── 握持区域年龄 ─────────────────────────────────────────────────────────

// TZ_UpdatePeakTzAgeUnit @ 0x6ba8c665
// 新出现的峰（没有对应的历史触点）把所在格的年龄置 1；峰在首 / 末两列时只在内侧相邻格信号够强时置边缘格。
// 有历史触点的峰若所在格还不在区域核心，就沿用历史触点所在格的年龄。
void UpdatePeakAgeUnit(uint8_t peak) {
    const FlashPrmt& fl = PrmtFlash();
    TsaBufferTable& buf = Buffers();
    const PeakEntry& p = PeakAt(peak);
    const uint8_t d1 = p.dim1;
    const uint8_t d2 = p.dim2;
    const uint8_t prevIdx = PrevTouch::GetIdxBasedOnId(p.id);
    const auto thold = static_cast<int16_t>(p.sig * 3 / 4);
    if (prevIdx == kNoTouch) {
        if (d1 >= 2 && d1 <= fl.dim1Len - 3) {
            buf.tzAge[GridIndex(d1, d2)] = 1;
        } else if (d1 == 1) {
            if (thold < buf.dif[GridIndex(2, d2)]) buf.tzAge[GridIndex(1, d2)] = 1;
        } else if (d1 == fl.dim1Len - 2) {
            if (thold < buf.dif[static_cast<uint16_t>(GridIndex(d1, d2) - 1)]) buf.tzAge[GridIndex(d1, d2)] = 1;
        }
    } else if (Sensor::PosType(d1, d2) < 3) {
        const PrevTouchUnit& u = PrevTouches().unit[prevIdx];
        const auto pd1 = static_cast<uint8_t>(u.rawPos.dim1 >> 8);
        if (pd1 != 0 && pd1 < fl.dim1Len - 1) {
            buf.tzAge[GridIndex(d1, d2)] = buf.tzAge[GridIndex(pd1, static_cast<uint8_t>(u.rawPos.dim2 >> 8))];
        }
    }
}

// TZ_UpdatePeakTzAge @ 0x6ba8c9ad
// 原代码按上一帧的触点数（而不是峰数）遍历峰表。
void UpdatePeakAge() {
    for (uint8_t i = 0; i < Touches().count && i != kMaxTouchNum; ++i) UpdatePeakAgeUnit(i);
}

// TZ_AgeProcess @ 0x6ba8c428
// 区域核心格的年龄逐帧加上经过的时间（新格从 2 起），同一区域的格子统一取区域内的最大年龄。
void AgeProcess() {
    uint16_t* age = Buffers().tzAge;
    TsaPrpt& prpt = Prpt();
    const uint64_t now = Static().curTimeMs;
    std::array<uint16_t, kZoneAgeSlots> zoneAge{};
    if (prpt.tzAgeLastTime == UINT64_MAX) prpt.tzAgeLastTime = now;
    const int dt = static_cast<int>(now) - static_cast<int>(prpt.tzAgeLastTime);
    const int n = PrmtFlash().sensorNum;
    for (int i = n - 1; i >= 0; --i) {
        const auto cell = static_cast<uint16_t>(i);
        if (Sensor::PosType(cell) < 3) {
            age[i] = 0;
            continue;
        }
        const int a = age[i] < 2 ? kNewAge : std::min(dt + age[i], 0xffff);
        const int zone = static_cast<int8_t>(Sensor::ZoneIndex(cell));
        if (zone >= 0 && zone < kZoneAgeSlots && zoneAge[zone] < a) zoneAge[zone] = static_cast<uint16_t>(a);
        age[i] = static_cast<uint16_t>(a);
    }
    for (int i = n - 1; i >= 0; --i) {
        const int zone = static_cast<int8_t>(Sensor::ZoneIndex(static_cast<uint16_t>(i)));
        if (zone >= 0 && zone < kZoneAgeSlots && age[i] != 0) age[i] = zoneAge[zone];
    }
    prpt.tzAgeLastTime = now;
}

}  // namespace

// TZ_UnitInit @ 0x6ba7f620
void UnitInit(TouchZoneUnit& z) { ZeroFill(z); }

// TZ_VirtualPeakInfoRetrieval @ 0x6ba8af94
// 新插入的虚拟峰尚未归入区域时，把它所在格标进当前区域。
void VirtualPeakInfoRetrieval() {
    int8_t* map = TzAccess().map;
    const PeakTable& pk = Peaks();
    for (uint8_t i = 0; i < pk.count; ++i) {
        const uint32_t bit = 1u << i;
        if ((pk.entry[i].flags & kPeakVirtual) == 0 || (PeakAccessed() & bit) != 0) continue;
        const uint16_t cell = GridIndex(pk.entry[i].dim1, pk.entry[i].dim2);
        map[cell] = MapCode(1, Buffers().dif[cell]);
        PeakAccessed() |= bit;
    }
}

// TZ_Malloc @ 0x6ba856e0
bool Malloc() {
    ZoneSig() = static_cast<int16_t*>(Mem::Allocate(GridBufSize() * 2u));
    return ZoneSig() != nullptr;
}

// TZ_Free @ 0x6ba85723
void Free() { Mem::Release(ZoneSig()); }

// TZ_Init @ 0x6ba8aa70
void Init() {
    VpInit();
    ContourInit();
    Prpt().tzAgeLastTime = UINT64_MAX;
    TzAccess().tag = 0x10;
    TzAccess().map = Buffers().tz;
    CurrentAccess() = &TzAccess();
}

// TZ_Process @ 0x6ba8c9f0
// 原代码的 SS 低信号检测（TZ_SSLoProcess）依赖 SS，本机型裁掉了：其结果恒为 0。
void Process() {
    if ((PrmtFlash().gripFlags & kGripAge) != 0) UpdatePeakAge();
    Touches().count = 0;
    PeakBasedProcess();
    if ((PrmtFlash().gripFlags & kGripAge) != 0) AgeProcess();
}

}  // namespace Solvers::Tsa::Core::TouchZone

// raw 的调用方
extern "C" {
namespace TZ = Solvers::Tsa::Core::TouchZone;
void TZ_Process(void) { TZ::Process(); }
}  // extern "C"
