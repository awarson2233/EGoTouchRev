#include "App/Afe.h"

#include "Alg/Touch/TouchData.h"
#include "App/Sys.h"
#include "App/Tsa.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Common/Bss.h"
#include "Prmt/Prmt.h"
#include "Prmt/TsaPrmt.h"

namespace Solvers::Tsa::Core::Afe {

namespace {

constexpr uint32_t kStatusIdle = 0x1;
constexpr uint32_t kStatusRecal = 0x80;
constexpr uint32_t kStatusProximityScreenOff = 0x400;
constexpr uint32_t kStatusSideIdle = 0x8000;
constexpr uint32_t kStatusRawJump = 0x20000;
constexpr uint32_t kStatusRawUnstable = 0x40000;
constexpr uint32_t kStatusCoreTouch = 0x100000;

constexpr uint32_t kExceptionNoise = 0x100;             // TouchTable::exceptionFlags
constexpr uint32_t kAftIdleScanRate = 0x4;              // TsaStatic::aftGateFlags

constexpr uint8_t kMaxScanNum = 10;
constexpr uint8_t kNone = 0xff;
constexpr uint16_t kIdleScanRate = 60;
constexpr uint64_t kFreqSwitchTimeoutMs = 2000;
constexpr uint16_t kQuietFramesToRaise = 0xfa;

/// TSACore 自己的 AFE 结构（g_thpAFEStruct）。info 原是宿主的驱动信息，只有 ALPS 机型读其中的版本名，这里恒为空。
struct Slots {
    const void* info;
    Caps* caps;
    Frame* frame;
};
Slots& AfeSlots() { return Slot<Slots>(Bss().afeSlots); }

// 频点调度的状态位
constexpr uint32_t kScanSwitching = 0x1;  // 已决定切换，等 AFE 报到新频点
constexpr uint32_t kScanStay = 0x2;       // 已决定留在当前频点
constexpr uint32_t kScanShiftDone = 0x4;  // 本帧完成一次跳频决策

struct FreqEntry {
    uint64_t enableTime;     // 切到该频点的时间
    uint64_t noisyTime;      // 在该频点上第一次检出噪声的时间（0 为未检出）
    int16_t maxChargerNoise;
};
struct RateEntry {
    uint32_t flags;          // bit0：所有频点都噪，应降扫描率
    uint16_t quietFrames;    // 有触点且无噪声的连续帧数
};

/// g_thpAFEScanStruct：频点与扫描率的当前 / 目标索引。
struct Scan {
    uint8_t curFreq;
    uint8_t nextFreq;
    uint64_t switchTime;
    uint32_t flags;          // kScan*
    FreqEntry freq[kMaxScanNum];
    uint8_t curRate;
    uint8_t nextRate;
    uint16_t maxRate;
    RateEntry rate[kMaxScanNum];
};

struct State {
    Scan scan;
    int32_t hopFrames;
    int32_t hopCount;
    bool frequentlyHopping;
    Frame internalFrame;
    Caps internalCaps;
};
State g;

uint64_t Now() { return Static().curTimeMs; }
const Caps* CapsPtr() { return AfeSlots().caps; }
bool FrameFlag(uint32_t bit) { return AfeSlots().frame != nullptr && (AfeSlots().frame->status & bit) != 0; }
bool NoiseDetected() { return (Touches().exceptionFlags & kExceptionNoise) != 0; }

bool IsValid() { return CapsPtr() != nullptr && AfeSlots().frame != nullptr; }

// ── 扫描率 ──────────────────────────────────────────────────────────────

// AFE_GetTargetScanRateIndex @ 0x6bb2d733
// 不超过 target 的最高扫描率；没有时取最低的。返回 true 表示没有恰好等于 target 的。
bool TargetRateIndex(uint16_t target, uint8_t* index) {
    const Caps* caps = CapsPtr();
    if (caps == nullptr) {
        *index = 0;
        return true;
    }
    uint8_t best = 0, lowest = 0;
    uint16_t bestRate = 0, lowestRate = 0xffff;
    for (uint8_t i = 0; i < caps->numScanRate; ++i) {
        const uint16_t r = caps->scanRate[i];
        if (r <= target && bestRate < r) {
            bestRate = r;
            best = i;
        }
        if (r < lowestRate) {
            lowestRate = r;
            lowest = i;
        }
    }
    if (bestRate == 0) {
        *index = lowest;
        return true;
    }
    *index = best;
    return bestRate != target;
}

void ResetRate(uint8_t i) { g.scan.rate[i] = {}; }

void ResetFreqTable() {
    for (FreqEntry& f : g.scan.freq) f = {};
}

// AFE_UpdateCurrScanRateUnit @ 0x6bb2e106：帧上报的扫描率对应的索引；IDLE 外找不到时归零
void UpdateCurrRate() {
    const Caps* caps = CapsPtr();
    if (caps == nullptr) return;
    bool found = false;
    for (uint8_t i = 0; i < caps->numScanRate; ++i) {
        if (caps->scanRate[i] == AfeSlots().frame->scanRate) {
            found = true;
            g.scan.curRate = i;
            break;
        }
    }
    if (!IsInIdle() && !found) g.scan.curRate = 0;
}

// AFE_UpdateScanRate @ 0x6bb2e7c2
// 当前扫描率已标记所有频点都噪时降到更低的一档；有触点且无噪声满 250 帧后升回更高的一档（已在上限的不升）。
void UpdateScanRate() {
    const Caps& caps = *CapsPtr();
    Scan& sc = g.scan;
    const uint8_t cur = sc.curRate;
    RateEntry& e = sc.rate[cur];
    uint8_t pick = cur;
    if ((e.flags & 1) != 0) {
        uint16_t bestBelow = 0;
        for (uint8_t i = 0; i < caps.numScanRate; ++i) {
            const uint16_t r = caps.scanRate[i];
            if (r < caps.scanRate[cur] && r <= sc.maxRate && bestBelow < r) {
                bestBelow = r;
                pick = i;
            }
        }
    }
    if (!NoiseDetected()) {
        if (Touches().count != 0) ++e.quietFrames;
    } else {
        e.quietFrames = 0;
    }
    if (kQuietFramesToRaise < e.quietFrames && caps.scanRate[cur] == sc.maxRate) {
        --e.quietFrames;
    } else if (kQuietFramesToRaise < e.quietFrames) {
        e.quietFrames = 0;
        uint16_t bestAbove = 0xffff;
        for (uint8_t i = 0; i < caps.numScanRate; ++i) {
            const uint16_t r = caps.scanRate[i];
            if (caps.scanRate[cur] < r && r <= sc.maxRate && r < bestAbove) {
                bestAbove = r;
                pick = i;
            }
        }
    }
    sc.nextRate = pick;
    if (pick != cur) {
        sc.flags |= kScanSwitching;
        ResetFreqTable();
        ResetRate(pick);
    }
}

// AFE_NextScanRateIndexProcess @ 0x6bb2ec10
// 另外统计跳频频度：在最低扫描率上仍因噪声跳频的次数达到 2×频点数 + 1 时，帧数少于 4×频点数即判为频繁跳频。
void NextScanRateProcess() {
    const Caps* caps = CapsPtr();
    if (caps == nullptr || !IsManualFreqHopping()) return;
    TsaAssert(caps->numScanRate <= kMaxScanNum);
    if (kMaxScanNum < caps->numScanRate) return;
    const uint8_t prev = g.scan.curRate;
    UpdateScanRate();
    if (1000 < g.hopFrames) {
        g.hopFrames = 0;
        g.hopCount = 0;
    }
    uint8_t lowest = 0;
    for (uint8_t i = 0; i < caps->numScanRate; ++i) {
        if (caps->scanRate[i] < caps->scanRate[lowest]) lowest = i;
    }
    g.frequentlyHopping = false;
    ++g.hopFrames;
    if (prev == lowest && (g.scan.rate[prev].flags & 1) != 0 && g.scan.nextFreq != g.scan.curFreq) ++g.hopCount;
    if (caps->numScanFreq * 2 + 1 <= g.hopCount) {
        g.frequentlyHopping = g.hopFrames < caps->numScanFreq * 4;
        g.hopCount = 0;
        g.hopFrames = 0;
    }
}

// ── 频点 ────────────────────────────────────────────────────────────────

// AFE_IsFreqValid @ 0x6bb2e1ca：没检出过噪声，或启用后很久才变噪
bool IsFreqValid(uint8_t i) {
    const FreqEntry& f = g.scan.freq[i];
    return f.noisyTime == 0 || PrmtDynamic().freqNoisyValidMs < f.noisyTime - f.enableTime;
}

// AFE_UpdateCurrScanFreqUnit @ 0x6bb2dee2
// 帧上报的频点对应的索引（手动跳频时记下启用时间）；目标频点已到达时置 kScanShiftDone。
void UpdateCurrFreq() {
    const Caps* caps = CapsPtr();
    if (caps == nullptr) return;
    Scan& sc = g.scan;
    bool found = false;
    uint8_t i = 0;
    for (; i < caps->numScanFreq; ++i) {
        if (caps->scanFreq[i] == AfeSlots().frame->scanFreq) {
            found = true;
            break;
        }
    }
    if (found && sc.curFreq != i) {
        sc.curFreq = i;
        if (IsManualFreqHopping()) {
            TsaAssert(i < kMaxScanNum);
            if (i < kMaxScanNum) sc.freq[i] = {Now(), 0, sc.freq[i].maxChargerNoise};
        }
    }
    if (!found) sc.curFreq = 0;
    const uint32_t f = sc.flags;
    sc.flags &= ~kScanShiftDone;
    if (sc.nextFreq == sc.curFreq) {
        if ((f & (kScanSwitching | kScanStay)) != 0) sc.flags |= kScanShiftDone;
        sc.flags &= ~kScanSwitching;
    }
    sc.flags &= ~kScanStay;
}

// AFE_UpdateScanFreq @ 0x6bb2e2f4
// 当前频点检出噪声：按顺序找下一个可用频点；都不可用时取充电噪声最小的（充电且仍超过 fingerThold 时标记
// 当前扫描率应降档），当前噪声过大时直接换下一个。
void UpdateScanFreq() {
    const Caps& caps = *CapsPtr();
    Scan& sc = g.scan;
    constexpr int16_t noise = 0;  // 充电噪声检测（Exception_GetDetectedChargerNoise）在本移植里裁掉了
    FreqEntry& cur = sc.freq[sc.curFreq];
    if (cur.maxChargerNoise < noise) cur.maxChargerNoise = noise;
    if (cur.noisyTime == 0) cur.noisyTime = Now();
    uint8_t pick = 0;
    sc.nextFreq = kNone;
    for (uint8_t a = sc.curFreq + 1; a < caps.numScanFreq + sc.curFreq; ++a) {
        pick = a % caps.numScanFreq;
        if (IsFreqValid(pick)) {
            sc.nextFreq = pick;
            break;
        }
    }
    if (sc.nextFreq == kNone) {
        int16_t minNoise = INT16_MAX;
        for (uint8_t a = 0; a < caps.numScanFreq; ++a) {
            if (sc.freq[a].maxChargerNoise < minNoise) {
                minNoise = sc.freq[a].maxChargerNoise;
                pick = a;
            }
        }
        const FlashPrmt& fl = PrmtFlash();
        if (fl.fingerThold < minNoise && Sys::IsChargerPlugged()) sc.rate[sc.curRate].flags |= 1;
        sc.nextFreq = fl.peakFilterReferenceThreshold / 3 < noise ? static_cast<uint8_t>((sc.curFreq + 1) % caps.numScanFreq) : pick;
    }
    if (sc.nextFreq == sc.curFreq) {
        sc.flags |= kScanStay;
    } else {
        sc.flags |= kScanSwitching;
        sc.switchTime = Now();
    }
}

// AFE_NextScanFreqIndexProcess @ 0x6bb2e67d：已在目标频点且检出噪声时另选；切换超过 2 s 未到达时放弃
void NextScanFreqProcess() {
    const Caps* caps = CapsPtr();
    if (caps == nullptr) return;
    Scan& sc = g.scan;
    if (sc.nextFreq == sc.curFreq) {
        if (IsManualFreqHopping() && NoiseDetected()) {
            TsaAssert(caps->numScanFreq <= kMaxScanNum);
            if (caps->numScanFreq <= kMaxScanNum) UpdateScanFreq();
        }
    } else if (sc.switchTime != 0 && kFreqSwitchTimeoutMs < Now() - sc.switchTime) {
        sc.nextFreq = sc.curFreq;
        sc.switchTime = Now();
    }
}

// AFE_ScanInit @ 0x6bb2d3e8
void ScanInit() {
    g.scan = {};
    g.scan.maxRate = PrmtFlash().maxScanRate;
}

}  // namespace

// AFE_Init @ 0x6bb2ca4f（原版随后用模拟 AFE 顶上，宿主的 SetCaps / SetFrame 会把它整个覆盖掉）
void Init() {
    g.internalFrame = {};
    g.internalCaps = {};
    AfeSlots() = {nullptr, &g.internalCaps, &g.internalFrame};
    ScanInit();
}

// AFE_SetCaps @ 0x6bb2cb53
void SetCaps(const Caps* caps) {
    if (AfeSlots().caps == nullptr) AfeSlots().caps = &g.internalCaps;
    *AfeSlots().caps = *caps;
    TsaAssert(caps->numScanFreq <= kMaxScanNum);
    TsaAssert(caps->numScanRate <= kMaxScanNum);
}

// AFE_SetFrame @ 0x6bb2cb9d：原版还把矩阵与自容数据拷进自己的缓冲，矩阵只在本帧处理中读一次，直接用宿主的
void SetFrame(const Frame* frame) {
    if (AfeSlots().frame == nullptr) AfeSlots().frame = &g.internalFrame;
    *AfeSlots().frame = *frame;
}

void SetMaxScanRate(uint16_t rate) { g.scan.maxRate = rate; }
Frame* GetFrame() { return AfeSlots().frame; }
const Caps* GetCaps() { return AfeSlots().caps; }
uint32_t FrameStatus() { return AfeSlots().frame->status; }

// AFE_ScanPreProcess @ 0x6bb2ee5e
void PreProcess() {
    UpdateCurrRate();
    UpdateCurrFreq();
}

// AFE_Process @ 0x6bb2d032
void Process() {
    if (!IsValid() || Tsa::IsOkToEnterIdle()) return;
    NextScanFreqProcess();
    NextScanRateProcess();
}

// AFE_GetNextScanFreqIndex @ 0x6bb2d650
bool GetNextScanFreqIndex(uint8_t* index) {
    const Caps* caps = CapsPtr();
    const Scan& sc = g.scan;
    if (!IsManualFreqHopping()|| caps->numScanFreq <= sc.nextFreq) {
        *index = sc.curFreq;
        return true;
    }
    *index = sc.nextFreq;
    return sc.nextFreq == sc.curFreq;
}

// AFE_GetNextScanRateIndex @ 0x6bb2d8a1
// 全屏模式有大面积触摸、所有频点都噪、或侧边触摸可进 IDLE 时用 60 Hz；手动跳频时取调度的目标，
// 否则取上限内最高的一档（接近与双倍报点在本移植里都被裁掉）。
bool GetNextScanRateIndex(uint8_t* index) {
    const Caps* caps = CapsPtr();
    Scan& sc = g.scan;
    if (caps == nullptr) {
        *index = sc.curRate;
        return true;
    }
    const TsaStatic& s = Static();
    if (PrmtFlash().sensorArchitecture == 3 && Touches().liftOffCount != 0) {
        (void)TargetRateIndex(kIdleScanRate, index);
    } else if (Sys::IsAllFreqNoisy() ||
               ((s.featureFlags & kFeatSideTouch) != 0 && Sys::IsOkToEnterIdle() && (s.aftGateFlags & kAftIdleScanRate) == 0)) {
        (void)TargetRateIndex(kIdleScanRate, index);
    } else if (caps->freqHopMode == 1) {
        *index = sc.nextRate;
    } else {
        (void)TargetRateIndex(sc.maxRate, index);
    }
    if (caps->numScanRate <= *index || *index == sc.curRate) {
        *index = sc.curRate;
        return true;
    }
    return false;
}

uint16_t ColNum() { return CapsPtr() == nullptr ? 10 : CapsPtr()->numCol; }
uint16_t RowNum() { return CapsPtr() == nullptr ? 10 : CapsPtr()->numRow; }
bool IsInIdle() { return FrameFlag(kStatusIdle); }
bool IsInSideIdle() { return FrameFlag(kStatusSideIdle); }
bool IsProximityScreenOff() { return FrameFlag(kStatusProximityScreenOff); }
bool IsRecalRequested() { return FrameFlag(kStatusRecal); }
bool IsCoreTouchDetected() { return FrameFlag(kStatusCoreTouch); }
bool IsRawJump() { return FrameFlag(kStatusRawJump); }
bool IsRawUnstable() { return FrameFlag(kStatusRawUnstable); }
bool IsManualFreqHopping() { return CapsPtr() != nullptr && CapsPtr()->freqHopMode == 1; }
bool IsAutoCalibration() { return CapsPtr() != nullptr && CapsPtr()->calibrationMode == 2; }
bool IsAllFreqNoisy() { return IsManualFreqHopping() && (g.scan.rate[g.scan.curRate].flags & 1) != 0; }
bool IsFreqShiftDone() { return (g.scan.flags & kScanShiftDone) != 0; }
bool IsFrequentlyHopping() { return g.frequentlyHopping; }

}  // namespace Solvers::Tsa::Core::Afe
