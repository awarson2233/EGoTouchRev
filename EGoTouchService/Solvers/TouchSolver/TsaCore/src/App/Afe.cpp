#include "App/Afe.h"

#include <cstring>

#include "Alg/Rawdata/Rawdata.h"
#include "Alg/Touch/TouchData.h"
#include "App/Sys.h"
#include "App/Tsa.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Common/Image.h"
#include "Common/Memory.h"
#include "Extension/HardwareAnalyzer/HardwareAnalyzer.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"
#include "Prmt/TsaPrmt.h"

namespace Solvers::Tsa::Core::Afe {

namespace {

constexpr uint32_t kStatusIdle = 0x1;
constexpr uint32_t kStatusRecal = 0x80;
constexpr uint32_t kStatusProximityScreenOff = 0x400;
constexpr uint32_t kStatusFreqShiftDone = 0x4;
constexpr uint32_t kStatusSideIdle = 0x8000;
constexpr uint32_t kStatusRawJump = 0x20000;
constexpr uint32_t kStatusRawUnstable = 0x40000;
constexpr uint32_t kStatusCoreTouch = 0x100000;

constexpr uint32_t kSysDifBufPath = 0x10000;            // TsaStatic::systemFlags
constexpr uint32_t kSysProximityAutoCheck = 0x400000;
constexpr uint32_t kSysProximityEntered = 0x1000000;
constexpr uint32_t kSysProximityExited = 0x2000000;
constexpr uint32_t kExtProximity = 0x80;                // TouchTable::extFlags
constexpr uint32_t kExceptionNoise = 0x100;             // TouchTable::exceptionFlags
constexpr uint32_t kAftIdleScanRate = 0x4;              // TsaStatic::aftGateFlags

constexpr uint8_t kMaxScanNum = 10;
constexpr uint8_t kNone = 0xff;
constexpr uint16_t kIdleScanRate = 60;
constexpr uint64_t kFreqSwitchTimeoutMs = 2000;
constexpr uint16_t kQuietFramesToRaise = 0xfa;

/// TSACore 自己的 AFE 结构（g_thpAFEStruct，原布局：MISC 与 Sys 按偏移读 info 与 frame）。
struct Slots {
    const uint8_t* info;
    Caps* caps;
    Frame* frame;
};
Slots& Slot() { return Image::At<Slots>(0x6bbaa5c0); }

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

/// 自检模式的状态。
struct AutoCheck {
    bool active;
    bool toEnterIdle;          // 自检要求进 IDLE
    uint64_t prevTime;
    uint32_t frameStatus;
    // 频点遍历：两两组合 (from, to) 来回切换
    uint8_t freqLoop;
    uint8_t fromFreq, fromRate, toFreq, toRate;
    bool traversedDone;
    // 每个频点 / 扫描率组合上的 IDLE 进出
    uint8_t idleFreq, idleRate;
    int idleLoop;
    int idleCheckLoop;
    // 接近自检
    int proxState;
    uint8_t proxEnterFrames, proxExitFrames, proxLoop;
};

struct State {
    Scan scan;
    AutoCheck check;
    int32_t hopFrames;
    int32_t hopCount;
    bool frequentlyHopping;
    uint16_t* gridData;
    uint16_t* lineData;
    Frame internalFrame;
    Caps internalCaps;
    uint8_t simuInfo[0x60];
    Caps simuCaps;
    Frame simuFrame;
    uint16_t simuScanFreq[kMaxScanNum];
    uint16_t simuScanRate[kMaxScanNum];
};
State g;

uint64_t Now() { return Static().curTimeMs; }
const Caps* CapsPtr() { return Slot().caps; }
bool FrameFlag(uint32_t bit) { return Slot().frame != nullptr && (Slot().frame->status & bit) != 0; }
bool NoiseDetected() { return (Touches().exceptionFlags & kExceptionNoise) != 0; }

bool IsAlpsProject() {
    const char* name = Prmt::GetProjectName();
    return std::strcmp(name, "ALPS491000") == 0 || std::strcmp(name, "ALPS491600") == 0;
}

const char* VersionInfo() { return Slot().info == nullptr ? "UnKnown" : reinterpret_cast<const char*>(Slot().info + 0x40); }

bool IsValid() { return CapsPtr() != nullptr && Slot().frame != nullptr && Slot().info != nullptr; }

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
        if (caps->scanRate[i] == Slot().frame->scanRate) {
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
        if (caps->scanFreq[i] == Slot().frame->scanFreq) {
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
    const auto noise = static_cast<int16_t>(Exception_GetDetectedChargerNoise());
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

// AFE_Simulation @ 0x6bb2c8aa
// 宿主送来能力与帧之前的模拟 AFE（按机型给一组频点 / 扫描率）。
void Simulate() {
    struct Profile {
        int32_t hopMode;
        uint8_t numFreq;
        uint16_t freq[4];
        uint8_t numRate;
        uint16_t rate[6];
        const char* name;
    };
    const char* project = Prmt::GetProjectName();
    Profile p;
    if (IsAlpsProject()) {
        p = {2, 4, {0x62, 0xb9, 0x8e, 0x36}, 2, {0x78, 0x3c}, "Novatek"};
    } else if (std::strncmp(project, "JHSN", 4) == 0) {
        p = {2, 4, {0x60, 0x53, 0x52, 0x65}, 5, {0x78, 0x3c, 0x28, 0x1e, 0x0f}, "N"};
    } else {
        p = {PrmtFlash().afeFreqHopMode, 3, {0xc3, 0xc3, 0xb0}, 6, {0x78, 0xb4, 0x3c, 0x46, 0x41, 0x5a}, "S"};
    }
    Caps& c = g.simuCaps;
    c.freqHopMode = p.hopMode;
    c.numScanFreq = p.numFreq;
    std::memcpy(g.simuScanFreq, p.freq, p.numFreq * sizeof(uint16_t));
    c.scanFreq = g.simuScanFreq;
    c.numScanRate = p.numRate;
    std::memcpy(g.simuScanRate, p.rate, p.numRate * sizeof(uint16_t));
    c.scanRate = g.simuScanRate;
    c.calibrationMode = 1;
    g.simuFrame.scanFreq = p.freq[0];
    g.simuFrame.scanRate = p.rate[0];
    std::strncpy(reinterpret_cast<char*>(g.simuInfo), p.name, 0x20);
    Slot() = {g.simuInfo, &g.simuCaps, &g.simuFrame};
}

// ── 自检 ────────────────────────────────────────────────────────────────

// AFE_FreqShiftCheckReset @ 0x6bb29b90 / AFE_IDLECheckReset @ 0x6bb29c60
void AutoCheckInit() {
    AutoCheck& c = g.check;
    c.freqLoop = 0;
    c.fromFreq = c.fromRate = c.toFreq = c.toRate = kNone;
    c.traversedDone = true;
    c.active = false;
    c.prevTime = 0;
    c.toEnterIdle = false;
    c.idleFreq = c.idleRate = 0;
    c.idleCheckLoop = 0;
    c.idleLoop = 0;
}

// AFE_FreqShiftTraversedDoneInitFreqRate @ 0x6bb29d33：换下一个起点组合；全部遍历一轮后计一圈
bool NextFromIndex() {
    AutoCheck& c = g.check;
    const Caps& caps = *CapsPtr();
    if (c.fromFreq == kNone) {
        c.fromFreq = 0;
        c.fromRate = 0;
    } else if (caps.numScanFreq <= ++c.fromFreq) {
        if (caps.numScanRate <= ++c.fromRate) {
            ++c.freqLoop;
            c.fromRate = c.fromFreq = kNone;
            return false;
        }
        c.fromFreq = 0;
    }
    g.scan.nextFreq = c.fromFreq;
    g.scan.nextRate = c.fromRate;
    c.traversedDone = false;
    c.toFreq = c.toRate = kNone;
    return true;
}

// AFE_FreqShiftSetNextIndex @ 0x6bb29e17
bool NextToIndex() {
    AutoCheck& c = g.check;
    const Caps& caps = *CapsPtr();
    if (caps.numScanFreq <= ++c.toFreq) {
        if (caps.numScanRate <= ++c.toRate) {
            c.toRate = c.toFreq = kNone;
            c.traversedDone = true;
            return false;
        }
        c.toFreq = 0;
    }
    return true;
}

// AFE_FreqShiftAutoCheck @ 0x6bb29fef
// 在每对 (from, to) 组合间来回切换 51 圈；返回 false 表示遍历结束。
bool FreqShiftAutoCheck() {
    AutoCheck& c = g.check;
    Scan& sc = g.scan;
    if (0x33 <= c.freqLoop) return false;
    if (c.traversedDone) {
        NextFromIndex();
        return true;
    }
    if ((c.frameStatus & kStatusFreqShiftDone) == 0 && c.toFreq != kNone) return true;
    if (sc.curFreq == c.fromFreq && sc.curRate == c.fromRate) {
        if (c.toFreq == kNone) {
            c.toFreq = 0;
            c.toRate = 0;
        } else if (!NextToIndex()) {
            return true;
        }
        if (c.toRate == c.fromRate && c.toFreq == c.fromFreq && !NextToIndex()) return true;
        sc.nextFreq = c.toFreq;
        sc.nextRate = c.toRate;
    } else if (sc.curFreq == c.toFreq && sc.curRate == c.toRate) {
        sc.nextFreq = c.fromFreq;
        sc.nextRate = c.fromRate;
    }
    return true;
}

// AFE_IDLEAutoCheckUnit @ 0x6bb2a22b：100 帧后要求进 IDLE，进入 5 帧后要求退出；重复 100 次后完成
bool IdleAutoCheckUnit() {
    AutoCheck& c = g.check;
    if (100 < c.idleCheckLoop) {
        c.toEnterIdle = false;
        return true;
    }
    if (!c.toEnterIdle && !IsInIdle()) {
        if (c.idleLoop < 100) {
            ++c.idleLoop;
        } else {
            c.idleLoop = 0;
            if (100 < ++c.idleCheckLoop) {
                c.toEnterIdle = false;
                return true;
            }
            c.toEnterIdle = true;
        }
    } else if (c.toEnterIdle && IsInIdle()) {
        if (c.idleLoop < 5) {
            ++c.idleLoop;
        } else {
            c.idleLoop = 0;
            c.toEnterIdle = false;
        }
    }
    return false;
}

// AFE_IDLEAutoCheck @ 0x6bb2a466：逐个频点 / 扫描率组合做 IDLE 进出检查；返回 true 表示全部完成
bool IdleAutoCheck() {
    AutoCheck& c = g.check;
    Scan& sc = g.scan;
    const Caps& caps = *CapsPtr();
    if (caps.numScanRate <= c.idleRate) return true;
    bool unitDone = false;
    if (c.idleFreq < caps.numScanFreq) {
        sc.nextFreq = c.idleFreq;
        sc.nextRate = c.idleRate;
        unitDone = sc.nextFreq == sc.curFreq && sc.nextRate == sc.curRate && IdleAutoCheckUnit();
    }
    if (unitDone) {
        if (!IsInIdle()) {
            c.toEnterIdle = false;
            c.idleLoop = 0;
            if (caps.numScanFreq <= ++c.idleFreq) {
                if (caps.numScanRate <= ++c.idleRate) return true;
                c.idleFreq = 0;
            }
            sc.nextFreq = c.idleFreq;
            sc.nextRate = c.idleRate;
        } else {
            c.toEnterIdle = false;
        }
    }
    return false;
}

// AFE_ProximityAutoCheck @ 0x6bb2a9c8
// 强制进入接近（切到进入扫描率）→ 等系统进入 → 保持 10 帧 → 强制退出 → 等系统退出 → 保持 10 帧，循环 100 次。
void ProximityAutoCheck() {
    AutoCheck& c = g.check;
    if (100 <= c.proxLoop) return;
    uint32_t& ext = Touches().extFlags;
    const uint32_t sys = Static().systemFlags;
    switch (c.proxState) {
    case 0:
        ext |= kExtProximity;
        (void)TargetRateIndex(PrmtFlash().callScanRateProxOn, &g.scan.nextRate);
        c.proxState = 1;
        c.proxEnterFrames = 0;
        break;
    case 1:
        ext |= kExtProximity;
        if ((sys & kSysProximityEntered) != 0) c.proxState = 2;
        break;
    case 2:
        c.proxState = 3;
        break;
    case 3:
        ext |= kExtProximity;
        if (c.proxEnterFrames < 10) ++c.proxEnterFrames;
        else c.proxState = 4;
        break;
    case 4:
        ext &= ~kExtProximity;
        (void)TargetRateIndex(PrmtFlash().callScanRateProxOff, &g.scan.nextRate);
        c.proxState = 5;
        c.proxExitFrames = 0;
        break;
    case 5:
        ext &= ~kExtProximity;
        if ((sys & kSysProximityExited) != 0) c.proxState = 6;
        break;
    case 6:
        ext &= ~kExtProximity;
        if (c.proxExitFrames < 10) {
            ++c.proxExitFrames;
        } else {
            if (c.proxLoop < 100) ++c.proxLoop;
            c.proxState = 0;
        }
        break;
    default:
        break;
    }
}

// AFE_ScanAutoCheck @ 0x6bb2c027（只留驱动状态的部分，检查结果原本只写日志）
void ScanAutoCheck() {
    AutoCheck& c = g.check;
    const uint64_t prev = c.prevTime;
    c.prevTime = Now();
    if (prev == 0) return;
    if ((Static().systemFlags & kSysProximityAutoCheck) != 0) {
        ProximityAutoCheck();
    } else if (!FreqShiftAutoCheck() && !IdleAutoCheck()) {
        (void)IdleAutoCheck();
    }
}

}  // namespace

// AFE_Malloc @ 0x6bb2c9b5
bool Malloc() {
    g.gridData = static_cast<uint16_t*>(Mem::Allocate(GridBufSize() * 2u));
    if (g.gridData == nullptr) return false;
    g.lineData = static_cast<uint16_t*>(Mem::Allocate(LineBufSize() * 2u));
    return g.lineData != nullptr;
}

// AFE_Init @ 0x6bb2ca4f
void Init() {
    g.internalFrame = {};
    g.internalCaps = {};
    Slot() = {nullptr, &g.internalCaps, &g.internalFrame};
    Simulate();
    ScanInit();
    AutoCheckInit();
}

// AFE_SetCaps @ 0x6bb2cb53
void SetCaps(const Caps* caps) {
    if (Slot().caps == nullptr) Slot().caps = &g.internalCaps;
    *Slot().caps = *caps;
    TsaAssert(caps->numScanFreq <= kMaxScanNum);
    TsaAssert(caps->numScanRate <= kMaxScanNum);
}

// AFE_SetFrame @ 0x6bb2cb9d
void SetFrame(const Frame* frame) {
    if (Slot().frame == nullptr) Slot().frame = &g.internalFrame;
    Frame& f = *Slot().frame;
    f = *frame;
    if (f.rawData != nullptr) {
        std::memcpy(g.gridData, f.rawData, GridBufSize() * 2u);
        f.rawData = g.gridData;
    }
    if (f.lineData != nullptr) {
        std::memcpy(g.lineData, f.lineData, LineBufSize() * 2u);
        f.lineData = g.lineData;
    }
}

void SetInfo(const void* info) { Slot().info = static_cast<const uint8_t*>(info); }
void SetMaxScanRate(uint16_t rate) { g.scan.maxRate = rate; }
Frame* GetFrame() { return Slot().frame; }
const Caps* GetCaps() { return Slot().caps; }
uint32_t FrameStatus() { return Slot().frame->status; }

// AFE_ScanPreProcess @ 0x6bb2ee5e
void PreProcess() {
    UpdateCurrRate();
    UpdateCurrFreq();
}

// AFE_Process @ 0x6bb2d032
void Process() {
    if (!IsValid() || g.check.active || Tsa::IsOkToEnterIdle()) return;
    NextScanFreqProcess();
    NextScanRateProcess();
}

// AFE_GetNextScanFreqIndex @ 0x6bb2d650
bool GetNextScanFreqIndex(uint8_t* index) {
    const Caps* caps = CapsPtr();
    const Scan& sc = g.scan;
    const bool hop = caps != nullptr && (caps->freqHopMode == 1 || g.check.active);
    if (!hop || caps->numScanFreq <= sc.nextFreq) {
        *index = sc.curFreq;
        return true;
    }
    *index = sc.nextFreq;
    return sc.nextFreq == sc.curFreq;
}

// AFE_GetNextScanRateIndex @ 0x6bb2d8a1
// 自检时取自检的目标；全屏模式有大面积触摸、所有频点都噪、或侧边触摸可进 IDLE 时用 60 Hz；手动跳频时取
// 调度的目标，否则取上限内最高的一档（接近与双倍报点在本移植里都被裁掉）。
bool GetNextScanRateIndex(uint8_t* index) {
    const Caps* caps = CapsPtr();
    Scan& sc = g.scan;
    if (caps == nullptr) {
        *index = sc.curRate;
        return true;
    }
    if (g.check.active) {
        *index = sc.nextRate;
    } else {
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
    }
    if (caps->numScanRate <= *index || *index == sc.curRate) {
        *index = sc.curRate;
        return true;
    }
    return false;
}

// AFE_AutoCheckProcessInternal @ 0x6bb2c45b
void AutoCheckProcess(const uint16_t* data, uint16_t size, uint32_t ctrl, uint64_t timeMs) {
    TsaStatic& s = Static();
    if (!g.check.active) {
        AutoCheckInit();
        g.check.active = true;
        s.frameCounter = 0;
    }
    Sys::UpdateCtrlFlags(ctrl);
    s.curTimeMs = timeMs;
    ++s.frameCounter;
    if (Sys::IsHwReset()) Tsa::MsReset();
    TsaBufferTable& b = Buffers();
    const size_t bytes = PrmtFlash().sensorNum * 2u;
    if (data != nullptr) CopyChecked((s.systemFlags & kSysDifBufPath) == 0 ? static_cast<void*>(b.raw) : b.dif, bytes, data, size);
    Rawdata::Process();
    g.check.frameStatus = Slot().frame->status;
    PreProcess();
    if (100u < static_cast<uint32_t>(s.frameCounter)) {
        HardwareAnalyzer::GetRaw2RawPrpt();
        ScanAutoCheck();
    }
    CopyChecked(b.preHwRaw, GridBufSize() * 2u, b.raw, bytes);
}

bool IsInAutoCheck() { return g.check.active; }
void ClearAutoCheckFlag() { g.check.active = false; }
bool AutoCheckToEnterIdle() { return g.check.toEnterIdle; }

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
bool IsAlwaysNoisy() { return IsAlpsProject() && Static().maxRfNoiseFrames != 0; }
bool IsRawUnifiedToTargetValue() { return IsAlpsProject() && std::strcmp(VersionInfo(), "ALPS491600_P5007_T91") != 0; }
bool IsIndividualBaselineForEachFreq() { return IsRawUnifiedToTargetValue(); }

}  // namespace Solvers::Tsa::Core::Afe

// raw 的调用方
extern "C" {
namespace A = Solvers::Tsa::Core::Afe;
A::Frame* AFE_GetFrame(void) { return A::GetFrame(); }
uint32_t AFE_IsAutoCalibration(void) { return A::IsAutoCalibration() ? 1 : 0; }
}  // extern "C"
