#include "App/Tsa.h"

#include <array>
#include <cstring>

#include "Alg/Baseline/Baseline.h"
#include "Alg/CTD/Ctd.h"
#include "Alg/Cmf/Cmf.h"
#include "Alg/EdgeRecovery/EdgeRecovery.h"
#include "Alg/Exception/Exception.h"
#include "Alg/Filter/RxLineFilter.h"
#include "Alg/Filter/SigSumFilter.h"
#include "Alg/GridIir/GridIir.h"
#include "Alg/IDTracking/IdTracking.h"
#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Peak/PrevPeak.h"
#include "Alg/Rawdata/Rawdata.h"
#include "Alg/SelfData/SelfData.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchAction/TouchAction.h"
#include "Alg/TouchEvent/TouchEvent.h"
#include "Alg/TouchMode/TouchMode.h"
#include "Alg/TouchSize/TouchSize.h"
#include "Alg/TouchThold/TouchThold.h"
#include "Alg/TouchZone/TouchZone.h"
#include "Alg/TouchZone/TouchZoneInternal.h"
#include "Alg/TpSensor/TpSensor.h"
#include "App/Buffer.h"
#include "App/DataSwitch.h"
#include "App/Property.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaOut.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Common/Image.h"
#include "Common/Memory.h"
#include "Extension/HardwareAnalyzer/HardwareAnalyzer.h"
#include "Extension/Thenar/Thenar.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"
#include "Prmt/TsaPrmt.h"
#include "Rejection/AntiTouch/AntiTouch.h"
#include "Rejection/Asa/Asa.h"
#include "Rejection/StylusAft/StylusAft.h"
#include "Rejection/TouchReport/TouchReport.h"
#include "WorkAround/BadChannel/BadChannel.h"
#include "WorkAround/ToeSynaBl/ToeSynaBl.h"

namespace Solvers::Tsa::Core::Tsa {

namespace {

constexpr uint32_t kCtrlAfeAutoCheck = 0x100000;       // 宿主控制位：AFE 自检

constexpr uint32_t kSysSkipMs = 0x200;                 // TsaStatic::systemFlags
constexpr uint32_t kSysDifBufPath = 0x10000;
constexpr uint32_t kSysProcessingActive = 0x40000000;
constexpr uint32_t kSysSuspended = 0x80000000;         // 不处理本帧（只做收尾）

constexpr uint32_t kFeatExtMask = 0x4813;              // 宿主可开关的特性位
constexpr uint32_t kFeatStylusAft = 0x100;
constexpr uint32_t kFeatUnderWater = 0x800;
constexpr uint32_t kFeatAsa = 0x10000;

constexpr uint32_t kGripRegionPass = 0x400;            // FlashPrmt::gripFlags：GripFilter 的区域处理阶段

constexpr uint32_t kTouchExcHw = 0x400;                // TouchTable::exceptionFlags：只有硬件异常
constexpr uint32_t kTouchExcRecal = 0x1000;            // 检出需要重新校准

constexpr int64_t kNoTime = -1;
constexpr uint16_t kRptScale = 0x100;            // 报点坐标是 Q8.8 的格坐标
constexpr uint64_t kStylusIdleDelayMs = 2999;

/// TSA_ProcessingAFEFrame 交给主流程的 raw 副本（g_rawDataBuf）。
std::array<uint16_t, 0x2648 / 2> g_rawFrame{};

/// 笔离开后 3 s 内不进 IDLE（g_flagOKtoDelay3sToIdle / g_lastStylusTimeStamp）。
bool g_stylusSeen = false;
uint64_t g_lastStylusTime = 0;

bool AsaEnabled() { return (Static().featureFlags & kFeatAsa) != 0; }

void MsEnd() { Static().processingState &= ~kProcMsActive; }

// TSA_MSInitCommon @ 0x6ba90830（被裁掉的模块不再列出）
void MsInitCommon(bool full) {
    Prmt::Reset();
    PrevTouch::Init();
    Touch::Init();
    Buffer::Init();
    PrevPeak::Init();
    Sys::Init();
    Baseline::Init(full);
    Cmf::Init();
    TpSensor::Init();
    TouchZone::Init();
    Property::Init();
    EdgeRecovery::Init();
    TouchReport::Init();
    TouchMode::Init();
    Exception::Init();
    AntiTouch::Init();
    HardwareAnalyzer::Init();
    GripFilter_Init();
    BadChannel::Init();
    HandGesture_Init();
    if (full) Baseline::ClearRefreshRequest();
    Thenar::Init();
}

// TSA_MSInitProject @ 0x6ba90994
bool MsInitProject(const char* project) {
    if (!Prmt::LoadProject(project) || !Buffer::InitProject()) return false;
    MsInitCommon(true);
    Prpt().rawDirectionChecked = std::strcmp(ProjectName(), "DFLT000000") == 0 ? 0 : 1;
    return true;
}

void InitAsa(const Afe::Caps* caps) {
    if (!AsaEnabled()) return;
    Asa::InitAfeProject(caps, &Image::At<uint8_t>(0x6bca7bc0));  // g_tsaPrmtFlashAsa
}

// TSA_InitProject_UI @ 0x6ba99d4e / TSA_InitCommonUI @ 0x6ba99ce0
bool InitProject(const char* project) {
    if (!MsInitProject(project)) return false;
    InitAsa(Afe::GetCaps());
    Afe::Init();
    StylusAft::Init();
    return true;
}

// TSA_MSRawDirectionDectect @ 0x6ba90a3d
// 缺省机型第一次出现大信号时：负向时把 raw 的零点翻到另一侧，再重算常量参数。
void CheckRawDirection() {
    TsaPrpt& p = Prpt();
    if (p.rawDirectionChecked != 0 || std::strcmp(ProjectName(), "DFLT000000") != 0) return;
    if (-1000 <= p.dif.minSig && p.dif.maxSig <= 1000) return;
    if (p.dif.minSig < -1000) {
        FlashPrmt& fl = PrmtFlash();
        fl.field600 = fl.field5FC * 2 - fl.field600;
    }
    ConstPrmt::Init();
    p.rawDirectionChecked = 1;
}

// TSA_SetDMDType @ 0x6ba9d881：异常类型只记日志，检出时 HardwareAnalyzer 会置 kSpecialHwException
void ReportHwException() { (void)HardwareAnalyzer::GetDetectedException(); }

// SensorPitchSizeMap @ 0x6bb05fc0
// 传感器坐标（每格 scale）换成物理位置：在本格与下一格起点的位置间线性插值。表首项为 100.0 时不映射；
// 超出 0x4f 格时为 0。
int PitchSizeMap(int pos, const double* map, uint16_t scale) {
    if (map[0] == 100.0) return pos;
    if (scale == 0) return 0;
    const auto cell = static_cast<uint16_t>(pos / scale);
    if (0x4f <= cell) return 0;
    const auto frac = static_cast<uint32_t>(pos % scale) & 0xffffu;
    return static_cast<int>(map[cell + 1] * static_cast<double>(frac) +
                            map[cell] * static_cast<double>(static_cast<int>(scale - frac)));
}

// TSA_PitchSizeRemapProcess @ 0x6bb0611c：传感器间距不均匀时把报点坐标映射到物理位置
void PitchSizeRemap() {
    const FlashPrmt& fl = PrmtFlash();
    TouchTable& tt = Touches();
    for (uint8_t k = 0; k < tt.count; ++k) {
        Point& p = tt.unit[k].rptPos;
        p.dim1 = static_cast<uint16_t>(PitchSizeMap(p.dim1, fl.dim1PitchMap, kRptScale));
        p.dim2 = static_cast<uint16_t>(PitchSizeMap(p.dim2, fl.dim2PitchMap, kRptScale));
    }
}

// 互容主流程里，跳过当前帧时沿用上一帧的触点。
void KeepPrevTouches() {
    Touch::KeepPrevTouchWithExceptionExcluded();
    MsEnd();
}

// 峰检测到报点
void TrackAndReport() {
    FlashPrmt& fl = PrmtFlash();
    Prmt::Process();
    Peak::Process();
    TouchZone::PeaksBeforeSwitchDif() = Peaks();
    fl.gripFlags |= kGripRegionPass;
    GripFilter_RegionProcess();
    Buffer::PrevFrameProcess();
    TouchZone::Process();
    fl.gripFlags &= ~kGripRegionPass;
    RxLineFilter::Process();
    SigSumFilter::ReserveTouch(fl.sigSumReserveTouch);
    Ctd::EcProcess();
    IdTracking::Process(false);
    Touch::AssignPreIdxBasedOnId();
    PrevTouch::AssignCurIdxBasedOnId();
    TouchSize::Process();
    TouchEvent::Process();
    Buffer::PreProcess();
    EdgeRecovery::Process();
    TouchAction::Process();
    TouchSize::PostProcess();
    TouchAction::PostProcess();
    Exception::Process();
    GripFilter_Process();
    TouchMode::Process();
    AntiTouch::Process();
    Touch::ProcessBetaGrip();
    TouchReport::Process();
    Touch::ProcessExtInfo();
    HardwareAnalyzer::PostProcess();
    HandGesture_Process();
    PrevTouch::PostProcess();
    PrevPeak::Process();
    Buffer::PostProcess();
}

// TSA_MSProcess @ 0x6ba90e43
// raw → Dif（基线、CMF、GridIIR）→ 属性统计 → 峰 → 触点跟踪与报点。硬件分析判定跳帧时沿用上一帧的触点。
// 被裁掉的模块：自容（SS）、安全基线、信号差异、接近、边缘峰滤波；充电噪声检测恒为未检出。
void MsProcess(const int16_t* raw) {
    TsaStatic& s = Static();
    s.processingState |= kProcMsActive;
    DataSwitch::ToGrid();
    if (raw == nullptr || PrmtDynamic().field024 != 0) {  // 后者原本还要求自容 raw 无效，自容被裁掉后恒成立
        if (Sys::IsHwReset()) MsReset();
        MsEnd();
        return;
    }
    TsaBufferTable& b = Buffers();
    const size_t bytes = PrmtFlash().sensorNum * 2u;
    const size_t gridBytes = GridBufSize() * 2u;
    HardwareAnalyzer::Reset();
    if (Sys::IsHwReset()) MsReset();
    Touch::Clean();
    CopyChecked(b.preDif, gridBytes, b.dif, bytes);
    const bool difPath = (s.systemFlags & kSysDifBufPath) != 0;
    if (!difPath) {
        CopyChecked(b.raw, 0x2648, raw, bytes);
        TpSensor::ProcessRaw();
    } else {
        CopyChecked(b.dif, 0x2648, raw, bytes);
    }
    Rawdata::Process();
    HardwareAnalyzer::Process();
    if (HardwareAnalyzer::IsToBypassCurrentFrame()) {
        Touch::KeepPrevTouchWithExceptionExcluded();
        Baseline::CalcDiffCommon(b.dif, b.raw, b.bl);
        Property::GetDifPreCmf();
        CopyChecked(b.difGrid, gridBytes, b.preDif, bytes);
        MsEnd();
        return;
    }
    Prmt::PreProcess();
    CopyChecked(b.preCmfRaw, 0x2648, b.raw, bytes);
    if (!difPath) {
        const FlashPrmt& fl = PrmtFlash();
        Baseline::CalcDiffCommon(b.dif, b.raw, b.bl);
        Property::GetDifPreCmf();
        Baseline::UpdateForRawUnstable();
        SelfForBlReset() = SelfBuf();
        if (fl.cmfEnabled != 0) Rawdata::Cmf();
        if ((s.featureFlags & kFeatUnderWater) != 0) Exception::CheckUnderWater();
        Baseline::Process(Sys::IsHwReset(), false);
        if (!Baseline::IsReset()) {
            if (fl.cmfEnabled == 0) Cmf::Process();
            GridIir::Process();
        }
    }
    HardwareAnalyzer::ProcessDif();
    TpSensor::Process();
    Property::Get(reinterpret_cast<const uint16_t*>(b.raw), reinterpret_cast<const uint16_t*>(b.bl));
    TsaPrpt& p = Prpt();
    p.msDif = p.dif;
    p.msUnderWaterCells = p.underWaterCells;
    p.msDifCore = p.difCore;
    p.msDifInner = p.difInner;
    Self::Process();
    ToeSynaBl::GetSidePrpt();
    CheckRawDirection();
    if (HardwareAnalyzer::IsToBypassCurrentFrame()) {
        KeepPrevTouches();
        return;
    }
    TrackAndReport();
    MsEnd();
}

// TSA_Processing @ 0x6ba9a443
void Processing(const int16_t* raw, uint32_t ctrl, uint64_t timeMs) {
    TsaStatic& s = Static();
    Afe::PreProcess();
    s.curTimeMs = timeMs;
    Sys::UpdateCtrlFlags(ctrl);
    Sys::PreProcess();
    Baseline::PreProcess();
    if ((ctrl & kCtrlAfeAutoCheck) != 0) {
        Afe::AutoCheckProcess(reinterpret_cast<const uint16_t*>(raw), static_cast<uint16_t>(PrmtFlash().sensorNum * 2),
                              ctrl, timeMs);
        Sys::PostProcess();
        return;
    }
    if (Afe::IsInAutoCheck()) Afe::ClearAutoCheckFlag();
    if ((s.systemFlags & kSysSuspended) != 0) {
        Sys::PostProcess();
        return;
    }
    if (s.frameDone == 0) s.systemFlags |= kSysProcessingActive;
    if ((s.systemFlags & kSysSkipMs) == 0) MsProcess(raw);
    Thenar::ProcessInfoByDif();
    if (AsaEnabled()) {
        Asa::Process(ctrl);
        if ((s.featureFlags & kFeatStylusAft) != 0 && (s.aftGateFlags & 4) == 0) StylusAft::Process();
    }
    Out::PostProcess();
    ReportHwException();
    Sys::PostProcess();
    s.frameDone = 1;
    ++s.frameCounter;
    Afe::Process();
    PitchSizeRemap();
}

// TSAMS_IsReadyForIDLEMode @ 0x6ba9998b（HardwareAnalyzer 从不阻止进 IDLE）
bool IsMsReadyForIdle() {
    bool ready = true;
    if (GripFilter_IsOKToEnterIDLE() == 0) {
        if (Prpt().touchMode != 0) ready = false;
        if (!Baseline::IsOkToIdle()) ready = false;
    }
    const uint32_t ex = Touches().exceptionFlags;
    if (ex != 0 && ex != kTouchExcHw) ready = false;
    if (Baseline::ResetInDebounce()) ready = false;
    if (TouchThold::IsSwitching()) ready = false;
    return ready;
}

// TSAMS_IsOKToEnterIDLEMode @ 0x6ba99ad6：另外互容主流程初始化后 idleAfterInitMs 内不进 IDLE
bool IsMsOkToEnterIdle() {
    TsaStatic& s = Static();
    bool ok = IsMsReadyForIdle();
    if (s.curTimeMs < PrmtFlash().idleAfterInitMs + s.msInitTime) ok = false;
    if (s.curTimeMs < s.msInitTime) s.msInitTime = s.curTimeMs;
    return ok;
}

// TSA_DelayToIdleModeHPPx @ 0x6ba9e3c6：用过笔之后 3 s 内不进 IDLE
bool DelayIdleAfterStylus() {
    const uint64_t now = Static().curTimeMs;
    if (InStylusMode()) {
        g_stylusSeen = true;
        g_lastStylusTime = now;
    }
    return g_stylusSeen && now - g_lastStylusTime <= kStylusIdleDelayMs;
}

}  // namespace

// TSA_InitAFEProject_UI @ 0x6ba99dc7
int InitAfeProject(const char* project, const void* afeInfo, const Afe::Caps* caps) {
    if (project == nullptr || afeInfo == nullptr || caps == nullptr) return 1;
    Afe::SetCaps(caps);
    if (!InitProject(project)) return 1;
    Afe::SetCaps(caps);
    InitAsa(caps);
    Afe::SetInfo(afeInfo);
    return 0;
}

// TSA_ProcessingAFEFrame @ 0x6ba9a7a7
int ProcessAfeFrame(const Afe::Frame* frame, uint32_t ctrl, uint64_t timeMs) {
    if (frame == nullptr) return 1;
    Afe::SetFrame(frame);
    const Afe::Frame* f = Afe::GetFrame();
    const int16_t* raw = nullptr;
    if (f->rawData != nullptr) {
        const FlashPrmt& fl = PrmtFlash();
        CopyChecked(g_rawFrame.data(), sizeof(g_rawFrame), f->rawData, fl.afeDim1Len * fl.afeDim2Len * 2u);
        raw = reinterpret_cast<const int16_t*>(g_rawFrame.data());
    }
    Processing(raw, ctrl, timeMs);
    return 0;
}

// TSA_MSReset @ 0x6ba90b16
// 重新初始化各模块；外部特性开关、复位保持帧数、以及（视系统复位位）基线复位计数、各频点初始化次数、
// 重校准计数、互容初始化时间跨复位保留。
void MsReset() {
    TsaStatic& s = Static();
    TsaPrpt& p = Prpt();
    const uint32_t extFeatures = s.extFeatureFlagsInit;
    const uint8_t holdFrames = s.resetHoldFrames;
    uint8_t allFreqInitCount = 0;
    uint16_t maxRfNoiseFrames = 0;
    uint16_t blResetFrames = 0;
    uint8_t recalFrames = 0;
    uint8_t recalBoundaryHits = 0;
    uint64_t msInitTime = s.curTimeMs;
    if ((s.systemFlags & 8) != 0) {
        allFreqInitCount = p.allFreqInitCount;
        maxRfNoiseFrames = s.maxRfNoiseFrames;
    }
    if ((s.systemFlags & 4) == 0) blResetFrames = p.blResetFrames;
    if ((s.systemFlags & 1) == 0) {
        recalFrames = s.recalFrames;
        recalBoundaryHits = s.recalBoundaryHits;
    }
    if ((s.systemFlags & 5) == 0) msInitTime = s.msInitTime;
    MsInitCommon(false);
    s.featureFlags = (s.featureFlags & ~kFeatExtMask) | extFeatures;
    s.extFeatureFlagsInit = extFeatures;
    s.resetHoldFrames = holdFrames;
    if ((s.systemFlags & 8) != 0) {
        p.allFreqInitCount = allFreqInitCount;
        s.maxRfNoiseFrames = maxRfNoiseFrames;
    }
    if ((s.systemFlags & 4) == 0) p.blResetFrames = blResetFrames;
    if ((s.systemFlags & 1) == 0) {
        s.recalFrames = recalFrames;
        s.recalBoundaryHits = recalBoundaryHits;
    }
    s.msInitTime = msInitTime;
    s.blResetRefTime = 0;
}

// TSA_IsOKToEnterIDLEMode @ 0x6ba9e755
bool IsOkToEnterIdle() {
    if (AsaEnabled() && InStylusMode()) return false;
    if (Afe::IsInAutoCheck()) return Afe::AutoCheckToEnterIdle();
    return (Static().featureFlags & kFeatSideTouch) != 0 ? Sys::IsOkToEnterSideIdle() : Sys::IsOkToEnterIdle();
}

// TSA_UpdateIDLEModeInternally @ 0x6ba9e458
// 各条件都允许后再等 dbEnterIdle 才放行；任一条件不允许时重新计时。
bool UpdateIdleMode() {
    TsaStatic& s = Static();
    bool ok = IsMsOkToEnterIdle();
    if (Sys::DoesAfeRequireToExitIdle()) ok = false;
    if (Sys::IsAfeEnterIdleTimeout()) ok = false;
    if (AsaEnabled() && DelayIdleAfterStylus()) ok = false;
    if (!ok) {
        s.idleDebounceStart = kNoTime;
        return false;
    }
    if (s.idleDebounceStart == kNoTime) {
        s.idleDebounceStart = static_cast<int64_t>(s.curTimeMs);
        return false;
    }
    return static_cast<uint64_t>(s.idleDebounceStart) + PrmtFlash().dbEnterIdle <= s.curTimeMs;
}

// TSA_UpdateSideIDLEModeInternally @ 0x6ba9e663（侧边触摸被裁掉，恒不允许；两项 AFE 检查仍要更新计时）
bool UpdateSideIdleMode() {
    (void)Sys::DoesAfeRequireToExitSideIdle();
    (void)Sys::IsAfeEnterSideIdleTimeout();
    return false;
}

// TSA_InStylusMode @ 0x6ba9c771
bool InStylusMode() { return AsaEnabled() && (Asa::IsHpp2() || Asa::IsHpp3()); }

// TSA_GetAFEScanFreqIndex @ 0x6ba9f998
bool GetAfeScanFreqIndex(uint8_t* index) {
    if (index == nullptr || (AsaEnabled() && InStylusMode())) return true;
    return Afe::GetNextScanFreqIndex(index);
}

// TSA_GetAFEScanRateIndex @ 0x6ba9fa2d：可进 IDLE 时不切扫描率
bool GetAfeScanRateIndex(uint8_t* index) {
    if (index == nullptr || (AsaEnabled() && InStylusMode())) return true;
    if (IsOkToEnterIdle()) return true;
    return Afe::GetNextScanRateIndex(index);
}

// TSA_IsReCalibNeeded @ 0x6ba9e32b
// 第一次系统复位（AFE 不自己校准时）、检出需要校准、或各频点独立基线且基线复位完成时需要。
bool IsReCalibNeeded() {
    static bool firstReset = true;
    if (AsaEnabled() && Asa::IsHpp2()) return false;
    bool need = false;
    if (firstReset && (Static().systemFlags & 1) != 0 && !Afe::IsAutoCalibration()) {
        need = true;
        firstReset = false;
    }
    if ((Touches().exceptionFlags & kTouchExcRecal) != 0) need = true;
    if (Afe::IsIndividualBaselineForEachFreq() && Baseline::ResetStatus() == 1) need = true;
    return need;
}

}  // namespace Solvers::Tsa::Core::Tsa

// 宿主与 raw 的调用方
extern "C" {
namespace T = Solvers::Tsa::Core::Tsa;
namespace A = Solvers::Tsa::Core::Afe;
uint64_t TSA_InitAFEProject_UI(int64_t project, int64_t afeInfo, int64_t caps) {
    return static_cast<uint64_t>(T::InitAfeProject(reinterpret_cast<const char*>(project), reinterpret_cast<const void*>(afeInfo),
                                                   reinterpret_cast<const A::Caps*>(caps)));
}
int TSA_ProcessingAFEFrame(void* frame, uint32_t ctrl, uint64_t timeMs) {
    return T::ProcessAfeFrame(static_cast<const A::Frame*>(frame), ctrl, timeMs);
}
void TSA_MSReset(void) { T::MsReset(); }
uint8_t TSA_IsOKToEnterIDLEMode(void) { return T::IsOkToEnterIdle() ? 1 : 0; }
uint64_t TSA_InStylusMode(void) { return T::InStylusMode() ? 1 : 0; }
uint64_t TSA_GetAFEScanFreqIndex(int64_t index) { return T::GetAfeScanFreqIndex(reinterpret_cast<uint8_t*>(index)) ? 1 : 0; }
uint64_t TSA_GetAFEScanRateIndex(int64_t index) { return T::GetAfeScanRateIndex(reinterpret_cast<uint8_t*>(index)) ? 1 : 0; }
uint8_t TSA_IsReCalibNeeded(void) { return T::IsReCalibNeeded() ? 1 : 0; }
}  // extern "C"
