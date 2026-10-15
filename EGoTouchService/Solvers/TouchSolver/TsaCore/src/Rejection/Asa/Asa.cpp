#include "Rejection/Asa/Asa.h"

#include <cstddef>

#include "Alg/Peak/PrevPeak.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "App/Afe.h"
#include "App/TsaStatic.h"
#include "Extension/HardwareAnalyzer/HardwareAnalyzer.h"

namespace Solvers::Tsa::Core::Asa {

namespace {

/// AFE 帧 +0x40 指向的笔帧（himax_thp_drv 的 g_StylusOut）。
struct StylusFrame {
    const uint16_t* tx1LineData;
    const uint16_t* tx2LineData;
    uint16_t mainFreq;
    uint16_t auxFreq;
    uint16_t pressure;
    uint16_t pad16;
    uint32_t buttons;
    uint32_t aux;  // kAux*
};
static_assert(sizeof(StylusFrame) == 0x20);

constexpr uint32_t kAuxHpp2 = 0x1;
constexpr uint32_t kAuxHpp3 = 0x2;
constexpr uint32_t kAuxNoData = 0x4;     // 本帧没有笔数据（网格模式）
constexpr uint32_t kAuxMode8 = 0x8;
constexpr uint32_t kAuxActive = 0x10;    // 笔信号有效
constexpr uint32_t kAuxModeMask = kAuxNoData | kAuxMode8 | kAuxActive;

constexpr uint32_t kRptHover = 0x1;      // 报告状态：悬停 / 书写；低 3 位非零即笔在场
constexpr uint32_t kRptInk = 0x2;
constexpr uint32_t kRptInkAlt = 0x4;
constexpr uint32_t kRptInRangeMask = 0x7;

constexpr uint32_t kHpp3TouchEnable = 0x2;  // ASA 参数的 HPP3 特性位：笔在场时手指照常报点

constexpr uint32_t kFeatAsaEnabled = 0x10000;    // TsaStatic::featureFlags
constexpr uint32_t kFeatAltEvents = 0x1000000;
constexpr uint32_t kFeatStylusAft = 0x100;
constexpr uint32_t kSuppressByStylus = 0x80;    // TouchUnit::suppressFlags：笔进入时撤下的手指

constexpr uint8_t kDisableTouchFrames = 3;
constexpr uint64_t kBtIdleTimeoutMs = 60000;

/// ASA 参数表里用到的项。
struct FlashPrmt {
    uint8_t pad000[0xa40];
    uint32_t hpp3Features[2];
    uint8_t pad048[0xa68 - 0xa48];
    uint16_t dim1Res;
    uint16_t dim2Res;
};
static_assert(offsetof(FlashPrmt, hpp3Features) == 0xa40);
static_assert(offsetof(FlashPrmt, dim1Res) == 0xa68);

/// 本帧笔报告（g_curASOut 里手指侧读取的部分）。
struct Report {
    uint16_t x, y;
    int16_t tiltX, tiltY;
    uint16_t pressure;
    uint32_t status;
};

/// 笔模式状态（g_asaStatic）。
struct State {
    uint16_t activeFrames;   // 进入主动笔模式以来的帧数
    uint32_t status;         // 本帧报告状态
    uint32_t aux;            // 本帧 AFE 笔帧状态
    uint32_t prevAux;
    uint32_t hpp3Features;
};

struct Host {
    uint32_t status;
    uint16_t x, y;
    int16_t tiltX, tiltY;
    uint16_t pressure;
};

const FlashPrmt* g_flash = nullptr;
uint16_t g_dim1Res = 0, g_dim2Res = 0;
State g_state{};
Report g_cur{}, g_prev{};
Host g_host{};
uint64_t g_realtimeMs = 0;
uint64_t g_lastBtCmdMs = 0;
bool g_prevInRange = false;
uint8_t g_disableTouchFrames = 0;

/// 蓝牙连接状态（0 断开、1 连接、2 超时）与宿主送来的笔状态字。
int32_t g_btConnect = 0;
uint8_t g_btPenStatus = 0;

bool AsaEnabled() { return (Static().featureFlags & kFeatAsaEnabled) != 0; }

// ASA_Reset @ 0x6bac4e3b（ASACalibration_Init 只复位校准本体的状态）
void Reset() { g_cur = {}; }

// HPP3_BtStatusProcess @ 0x6bab778a：一分钟没有蓝牙消息时置为超时
void BtStatusProcess() {
    if (g_lastBtCmdMs + kBtIdleTimeoutMs < g_realtimeMs && g_btConnect == 1) g_btConnect = 2;
}

// ValidJudgment @ 0x6bac4efc：只能选一种协议；模式位不能全空或全有；HPP2 非网格模式时要有 TX1 线数据
bool IsFrameValid(const StylusFrame& f) {
    const bool hpp2 = (f.aux & kAuxHpp2) != 0;
    const bool hpp3 = (f.aux & kAuxHpp3) != 0;
    if (hpp2 == hpp3) return false;
    const uint32_t mode = f.aux & kAuxModeMask;
    if (mode == 0 || mode == kAuxModeMask) return false;
    return f.tx1LineData != nullptr || (f.aux & kAuxNoData) != 0;
}

// EnterActiveStylusMode @ 0x6bac858b / ExitActiveStylusMode @ 0x6bac87b1
void UpdateActiveMode(uint32_t aux) {
    g_state.status = 0;
    if ((aux & kAuxActive) != 0) {
        ++g_state.activeFrames;
        return;
    }
    g_state.activeFrames = 0;
    g_cur = {};
    g_prev = g_cur;
}

// ReleaseASAReportExitStylus @ 0x6bac527b（宿主版）：书写中掉信号时先报一帧撤掉压力的悬停，悬停中掉信号直接清空。
// 返回 true 表示已经完全退出。
bool ReleaseReport() {
    bool released = false;
    if ((g_prev.status & (kRptInk | kRptInkAlt)) == 0) {
        released = true;
        g_cur = {};
        g_state.activeFrames = 0;
    } else {
        g_cur = g_prev;
        g_cur.status = kRptHover;
        g_cur.pressure = 0;
    }
    g_prev = g_cur;
    return released;
}

// ASA_MainProcess @ 0x6bac6b5f 的宿主版：本帧报告取自宿主（TSAHost_SetStylusReport），控制流与原函数一致。
void MainProcess(const Afe::Frame& frame, const StylusFrame& f) {
    g_realtimeMs = static_cast<uint64_t>(frame.timestampSec) * 1000 + frame.timestampUsec / 1000;
    g_state.aux = f.aux;
    if ((f.aux & kAuxHpp3) != 0) BtStatusProcess();
    if (!IsFrameValid(f)) {
        Reset();
        return;
    }
    if ((g_state.prevAux & kAuxActive) != 0 && (g_state.aux & kAuxActive) == 0) {
        if (ReleaseReport()) g_state.prevAux = g_state.aux;
        return;
    }
    UpdateActiveMode(f.aux);
    g_state.prevAux = g_state.aux;
    if ((f.aux & kAuxNoData) != 0) return;
    g_cur = {};
    g_state.status = g_host.status;
    if ((g_host.status & kRptInRangeMask) != 0) {
        g_cur.x = g_host.x;
        g_cur.y = g_host.y;
        g_cur.tiltX = g_host.tiltX;
        g_cur.tiltY = g_host.tiltY;
        g_cur.pressure = g_host.pressure;
    }
    g_cur.status = g_state.status;
    g_prev = g_cur;
}

// ASA_IsTouchNull @ 0x6bac704c：HPP2 有笔数据，或 HPP3 有笔数据且未开 HPP3 手指报点时，手指不报点
bool IsTouchNull(uint32_t aux) {
    if ((aux & kAuxHpp2) != 0 && (aux & kAuxNoData) == 0) return true;
    return (g_state.hpp3Features & kHpp3TouchEnable) == 0 && (aux & kAuxHpp3) != 0 && (aux & kAuxNoData) == 0 &&
           (aux & kAuxMode8) == 0;
}

/// 笔刚进入时清掉手指：StylusAft 开启时，笔从无到有撤下现有触点，笔持续在场则重新初始化触点；
/// 否则（未开 HPP3 手指报点时）直接重新初始化。
void DisableTouches() {
    const uint32_t feat = Static().featureFlags;
    const auto reinit = [] {
        PrevTouch::Init();
        Touch::Init();
        PrevPeak::Init();
    };
    if ((feat & kFeatAltEvents) == 0 && (feat & kFeatStylusAft) != 0) {
        const bool inRange = RptInRange();
        if (inRange && !g_prevInRange) {
            TouchTable& tt = Touches();
            for (uint8_t k = 0; k < tt.count; ++k) tt.unit[k].suppressFlags |= kSuppressByStylus;
        }
        if (inRange && g_prevInRange) reinit();
    } else if ((g_state.hpp3Features & kHpp3TouchEnable) == 0) {
        reinit();
    }
}

}  // namespace

// ASA_InitAFEProject @ 0x6bac4eb9（ASA_LoadProjectPrmt + ASAStaticInit + ASA_Reset）
void InitAfeProject(const void* caps, const uint8_t* flash) {
    g_flash = reinterpret_cast<const FlashPrmt*>(flash);
    if (g_flash != nullptr) {
        g_dim1Res = g_flash->dim1Res;
        g_dim2Res = g_flash->dim2Res;
    }
    if (caps != nullptr) {
        g_state = {};
        g_state.hpp3Features = g_flash->hpp3Features[0] | g_flash->hpp3Features[1];
    }
    Reset();
}

// TSA_ASAProcess @ 0x6ba99e8d
// 原函数在解算之后还有 StylusRecheck 的门；宿主送来的报告已经是复核后的结果，这里只留复核通过的那一支。
void Process(uint32_t ctrl) {
    const Afe::Frame& frame = *Afe::GetFrame();
    const auto* f = static_cast<const StylusFrame*>(frame.stylusData);
    if (f == nullptr || f->aux == 0) {
        Reset();
    } else {
        const uint32_t feat = Static().featureFlags;
        if ((feat & kFeatAltEvents) == 0 && (feat & kFeatStylusAft) != 0) g_prevInRange = RptInRange();
        MainProcess(frame, *f);
        if (IsTouchNull(f->aux)) g_disableTouchFrames = kDisableTouchFrames;
        if (g_disableTouchFrames != 0) {
            --g_disableTouchFrames;
            DisableTouches();
        }
        HardwareAnalyzer::ReportToTsa();
    }
    if ((ctrl & 0xf) != 0) Reset();
}

// ASA_GetRpt* @ 0x6bac757f..
bool RptInRange() { return (g_cur.status & kRptInRangeMask) != 0; }
uint16_t RptX() { return g_cur.x; }
uint16_t RptY() { return g_cur.y; }
int16_t RptTiltX() { return g_cur.tiltX; }
int16_t RptTiltY() { return g_cur.tiltY; }
uint16_t RptPressure() { return g_cur.pressure; }
uint16_t Dim1Res() { return g_dim1Res; }
uint16_t Dim2Res() { return g_dim2Res; }

// ASA_GetBluetoothConnectStatus @ 0x6bac77ec
int32_t BtConnectStatus() { return g_btConnect; }
uint8_t BtPenStatus() { return g_btPenStatus; }
void ClearBtPenStatus() { g_btPenStatus = 0; }

// ASA_IsEnterStylusHPP2 / HPP3 @ 0x6bac6fe8
bool IsHpp2() { return (g_state.aux & kAuxHpp2) != 0 && g_state.activeFrames != 0; }
bool IsHpp3() { return (g_state.aux & kAuxHpp3) != 0 && g_state.activeFrames != 0; }

}  // namespace Solvers::Tsa::Core::Asa

// 宿主 API
extern "C" {
namespace As = Solvers::Tsa::Core::Asa;

// TSAHost_SetStylusReport：宿主送来的本帧笔报告（ASA 解算本体的替代）
void TSAHost_SetStylusReport(uint32_t status, uint16_t x, uint16_t y, int16_t tiltX, int16_t tiltY, uint16_t pressure) {
    As::g_host = {status, x, y, tiltX, tiltY, pressure};
}

// TSA_SetASABluetoothConnectStatus @ 0x6ba9cd02（ASA_SetBluetoothStatus）
void TSA_SetASABluetoothConnectStatus(uint32_t status) {
    if (!As::AsaEnabled()) return;
    As::g_lastBtCmdMs = As::g_realtimeMs;
    As::g_btConnect = (status & 1) != 0 ? 1 : 0;
}

// TSA_SetASABluetoothPenStatus @ 0x6ba9cc2d
void TSA_SetASABluetoothPenStatus(uint8_t status) {
    if (As::AsaEnabled()) As::g_btPenStatus = status;
}

// TSA_SetASABluetoothPressure @ 0x6ba9cc65：压力包只给 ASA 解算本体用，这里只记下蓝牙在线
void TSA_SetASABluetoothPressure(uint16_t, uint16_t, uint16_t, uint16_t) {
    if (!As::AsaEnabled()) return;
    As::g_lastBtCmdMs = As::g_realtimeMs;
    As::g_btConnect = 1;
}

// TSA_RptASAInRange @ 0x6ba9c7e9
uint64_t TSA_RptASAInRange(void) { return As::AsaEnabled() && As::RptInRange() ? 1 : 0; }

}  // extern "C"
