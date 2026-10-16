#include "Rejection/StylusAft/StylusAft.h"

#include <cstdint>
#include <cstdlib>

#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/TouchData.h"
#include "App/Afe.h"
#include "App/Scenario.h"
#include "App/TsaOut.h"
#include "App/TsaStatic.h"
#include "Common/TsaMath.h"
#include "Prmt/Prmt.h"
#include "Rejection/Asa/Asa.h"

namespace Solvers::Tsa::Core::StylusAft {

namespace {

constexpr uint32_t kFeatAltEvents = 0x1000000;   // TsaStatic::featureFlags：按蓝牙笔状态压低手掌门限
constexpr uint32_t kEventRptOff = 0x20;          // TouchUnit::event
constexpr uint32_t kSuppressed = 0x40;           // TouchUnit::suppressFlags
constexpr uint32_t kWithdrawn = 0x80;
constexpr uint32_t kPersistReported = 0x8;       // TouchUnit::persistFlags：曾经对外报点
constexpr uint32_t kPersistWithdrawable = 0x20000000;
constexpr uint32_t kVolatileWithdraw = 0x8000000;  // TouchUnit::volatileFlags：上一帧要求撤下
constexpr uint32_t kAsaStylusInRange = 0x10;     // AFE 笔帧状态

constexpr uint8_t kNoiseLevelOff = 3;            // 噪声等级到 3 时关闭
constexpr uint16_t kMaxAge = 0xf0;               // 超过这个帧龄且未在压制中的触点不处理
constexpr uint8_t kSuppressFrames = 40;
constexpr uint8_t kSuppressFramesPalm = 100;
constexpr uint64_t kStylusRecentMs = 200;        // 笔刚离开的这段时间里一律撤下
constexpr uint64_t kStylusNearMs = 800;          // 笔离开不久，扇形区域判据有效
constexpr uint64_t kPalmRecentMs = 200;
constexpr uint16_t kMovedFar = 150;              // 自按下移动的距离（mm²）超过它就不再按手掌处理
constexpr uint16_t kEdgeDistNear = 700;
constexpr uint8_t kPalmMargin = 50;
constexpr uint8_t kOtherPalmMargin = 200;

/// 笔的最近一次报告（显示坐标、倾角）与最近一次判为手掌的位置。
struct State {
    uint64_t lastStylusTime;
    int32_t stylusX, stylusY, tiltX, tiltY;
    uint64_t lastPalmTime;
    uint32_t palmX, palmY;
};
State g{};

bool IsSpecialApp() { return (Static().aftGateFlags & Scenario::kSpecialApp) != 0; }
uint64_t SinceStylus() { return Static().curTimeMs - g.lastStylusTime; }

TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
PrevTouchUnit& Prev(uint8_t prevIdx) { return PrevTouches().unit[prevIdx]; }

// StylusAft_DistBetweenPoints @ 0x6bb20d54
int Dist(int x1, int y1, int x2, int y2) {
    return Math::SqrtUint32(static_cast<uint32_t>((y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1)));
}

// StylusAft_BtStatusPenAttach / StylusAft_BtStatusPenHibernate
bool PenAttached() { return (Asa::BtPenStatus() & 2) != 0; }
bool PenHibernating() { return (Asa::BtPenStatus() & 1) != 0 || Asa::BtConnectStatus() != 1; }
bool PenAway() { return !PenAttached() && !PenHibernating(); }

// StylusAft_PalmLocationJudge @ 0x6bb20c5e：离屏幕边缘至少 margin
bool AwayFromBorder(uint8_t idx, uint8_t margin) {
    const FlashPrmt& fl = PrmtFlash();
    const int x = Out::DisplayXPos(idx);
    const int y = Out::DisplayYPos(idx);
    return margin < x && x < fl.dim1Res - margin && margin < y && y < fl.dim2Res - margin;
}

// StylusAft_InEdge @ 0x6bb21cc0：落在最外 3 格之内
bool InEdge(uint8_t idx) {
    const FlashPrmt& fl = PrmtFlash();
    const uint32_t x = Out::DisplayXPos(idx);
    const uint32_t y = Out::DisplayYPos(idx);
    const int cols = fl.dim1Len;
    const int rows = fl.dim2Len;
    if (x <= fl.dim1Res * 3u / cols) return true;
    if (static_cast<int>(x) >= static_cast<int>((cols - 3) * static_cast<uint32_t>(fl.dim1Res)) / cols) return true;
    if (y <= fl.dim2Res * 3u / rows) return true;
    return static_cast<int>(y) >= static_cast<int>((rows - 3) * static_cast<uint32_t>(fl.dim2Res)) / rows;
}

// StylusAft_GetPalmLevel @ 0x6bb209fb：0..3，越大越像手掌
uint8_t PalmLevel(uint8_t idx) {
    const TouchUnit& u = Cur(idx);
    const int major = u.maxMajorMm;
    const int ellipse = u.ellipseRate;
    if ((Static().featureFlags & kFeatAltEvents) != 0) {
        if (PenAway() && major > 20) return 3;
        if (major > 30) return 3;
    }
    if (!IsSpecialApp()) return major < 39 ? 0 : 3;
    if (!(ellipse + major < 41 || (major < 26 && ellipse < 26))) return 3;
    if (!(major < 23 || ellipse < 19)) return 2;
    return major < 26 && ellipse < 16 ? 0 : 1;
}

// StylusAft_PalmJudge @ 0x6bb20eb3：手掌等级 3，并记下手掌位置
bool PalmJudge(uint8_t idx) {
    if (PalmLevel(idx) < 3) return false;
    g.lastPalmTime = Static().curTimeMs;
    g.palmX = Out::DisplayXPos(idx);
    g.palmY = Out::DisplayYPos(idx);
    return true;
}

// StylusAft_InFanArea @ 0x6bb20d9b：离笔尖足够近，且离沿倾角方向外推的点也足够近
bool InFanArea(int x, int y) {
    const int tx = g.tiltX * 6;
    const int ty = g.tiltY * 6;
    const auto tilt = static_cast<uint32_t>(Math::SqrtUint32(static_cast<uint32_t>(ty * ty + tx * tx)));
    uint32_t nearRadius = tilt * 7;
    if (nearRadius < 800) nearRadius = 800;
    uint32_t farRadius = tilt * 5;
    if (farRadius < 0x236) farRadius = 0x236;
    const int farX = g.stylusX + g.tiltX * 30;
    const int farY = g.stylusY + g.tiltY * 30;
    return static_cast<uint32_t>(Dist(x, y, g.stylusX, g.stylusY)) < nearRadius &&
           static_cast<uint32_t>(Dist(x, y, farX, farY)) < farRadius;
}

// StylusAft_FanAreaJudge @ 0x6bb20f58
bool FanAreaJudge(uint8_t idx) {
    if (kMovedFar < Cur(idx).maxSqDistMmFromStart || kStylusNearMs <= SinceStylus()) return false;
    return InFanArea(Out::DisplayXPos(idx), Out::DisplayYPos(idx)) && PalmLevel(idx) != 0;
}

// StylusAft_CurrentDomainJudge @ 0x6bb21047：附近（主轴 16 倍以内）有手掌、正在压制或握持中的触点
bool CurrentDomainJudge(uint8_t idx) {
    if (kMovedFar < Cur(idx).maxSqDistMmFromStart) return false;
    const TouchTable& tt = Touches();
    for (uint8_t k = 0; k < tt.count; ++k) {
        if (k == idx) continue;
        const TouchUnit& o = tt.unit[k];
        const int d = Dist(Out::DisplayXPos(idx), Out::DisplayYPos(idx), Out::DisplayXPos(k), Out::DisplayYPos(k));
        if (d > static_cast<int>(o.maxMajorMm * 16u) || !AwayFromBorder(k, kOtherPalmMargin)) continue;
        if (PalmLevel(k) > 2) return true;
        if (std::abs(static_cast<int>(o.age) - static_cast<int>(Cur(idx).age)) > 100) return false;
        if (o.prevIdx != kMaxTouchNum && Prev(o.prevIdx).stylusSuppressFrames != 0) return true;
        if (o.gripStage == 1 || o.gripStage == 2) return true;
    }
    return false;
}

// StylusAft_PostDomainJudge @ 0x6bb21364：离刚判出的手掌不到主轴 5 倍
bool PostDomainJudge(uint8_t idx) {
    if (kPalmRecentMs <= Static().curTimeMs - g.lastPalmTime) return false;
    const int d = Dist(Out::DisplayXPos(idx), Out::DisplayYPos(idx), static_cast<int>(g.palmX), static_cast<int>(g.palmY));
    return d < Cur(idx).maxMajorMm * 5;
}

// StylusAft_EarlyPalmJudge @ 0x6bb2145c
bool EarlyPalmJudge(uint8_t idx) { return Cur(idx).age < 24 && PalmLevel(idx) > 1; }

// StylusAft_EdgePalmJudge @ 0x6bb214eb（只在特殊应用场景）
bool EdgePalmJudge(uint8_t idx) {
    const TouchUnit& u = Cur(idx);
    if (kMovedFar < u.maxSqDistMmFromStart || !IsSpecialApp()) return false;
    if (u.edgeDistDim1 < kEdgeDistNear || u.edgeDistDim2 < kEdgeDistNear) {
        const uint8_t level = PalmLevel(idx);
        return level > 1 || (level != 0 && u.age > 24);
    }
    return u.ellipseRate > 19;
}

// StylusAft_ProcessWithdraw @ 0x6bb216da
// 可撤下的触点（或本帧有抬起时）：上一帧要求撤下、或任一手掌判据成立时撤下整条轨迹。
void ProcessWithdraw(uint8_t idx, uint8_t prevIdx, bool& withdrawn) {
    TouchUnit& u = Cur(idx);
    PrevTouchUnit& p = Prev(prevIdx);
    if ((p.persistFlags & kPersistWithdrawable) == 0 && Touches().liftOffCount == 0) return;
    if ((p.volatileFlags & kVolatileWithdraw) != 0) {
        p.stylusWithdrawn = 1;
        u.volatileFlags &= ~kVolatileWithdraw;
    }
    if (PalmJudge(idx) || FanAreaJudge(idx) || CurrentDomainJudge(idx) || EarlyPalmJudge(idx) || EdgePalmJudge(idx)) {
        p.stylusWithdrawn = 1;
    }
    if (p.stylusWithdrawn != 0) {
        u.suppressFlags |= kWithdrawn;
        withdrawn = true;
    }
}

// StylusAft_ProcessSuppress @ 0x6bb21ac8：判据成立时压制若干帧；走远了且笔已离开时压制帧数减半
void ProcessSuppress(uint8_t idx, uint8_t prevIdx) {
    PrevTouchUnit& p = Prev(prevIdx);
    if (kMovedFar < Cur(idx).maxSqDistMmFromStart && kStylusNearMs < SinceStylus()) {
        p.stylusSuppressFrames = static_cast<int8_t>(static_cast<uint8_t>(p.stylusSuppressFrames) >> 1);
    }
    if (PalmJudge(idx)) {
        p.stylusSuppressFrames = kSuppressFramesPalm;
    } else if (FanAreaJudge(idx) || CurrentDomainJudge(idx) || PostDomainJudge(idx) || EarlyPalmJudge(idx) ||
               EdgePalmJudge(idx)) {
        p.stylusSuppressFrames = kSuppressFrames;
    }
}

// StylusAft_SuppressBaseContext @ 0x6bb21dfd：刚按下、椭圆度大的触点压住；笔不在附近时门限更低
void SuppressByContext(uint8_t idx, bool penAway) {
    TouchUnit& u = Cur(idx);
    const int base = penAway ? 5 : 10;
    if (penAway && u.age < 2) {
        u.suppressFlags |= kSuppressed;
        return;
    }
    const int ellipse = u.ellipseRate;
    const uint16_t age = u.age;
    if ((ellipse >= base + 2 && age <= 2) || (ellipse >= base + 4 && age <= 3) || (ellipse >= base + 6 && age <= 4) ||
        (ellipse >= base + 8 && age < 6)) {
        u.suppressFlags |= kSuppressed;
    }
}

// StylusAft_PostProcessUnit @ 0x6bb223ef（蓝牙连着笔时才处理）
void ProcessUnit(uint8_t idx) {
    const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(idx);
    if (static_cast<uint8_t>(Asa::BtConnectStatus()) == 0) return;
    TouchUnit& u = Cur(idx);
    PrevTouchUnit& p = Prev(prevIdx);
    if (kMaxAge < u.age && p.stylusSuppressFrames == 0) return;
    const TsaStatic& s = Static();
    if (kNoiseLevelOff <= s.noiseLevel) {
        p.stylusSuppressFrames = 0;
        p.stylusWithdrawn = 0;
        u.suppressFlags &= ~kSuppressed;
        return;
    }
    bool withdrawn = false;
    if (p.stylusWithdrawn == 0) {
        if ((s.noiseLevel < 2 || IsSpecialApp()) && AwayFromBorder(idx, kPalmMargin)) ProcessWithdraw(idx, prevIdx, withdrawn);
    } else {
        p.stylusSuppressFrames = kSuppressFrames;
    }
    if (!withdrawn) ProcessSuppress(idx, prevIdx);
    if (p.stylusSuppressFrames != 0) {
        --p.stylusSuppressFrames;
        u.suppressFlags |= kSuppressed;
    }
    if ((s.featureFlags & kFeatAltEvents) != 0 && !InEdge(idx)) SuppressByContext(idx, PenAway());
    if (IsSpecialApp() && u.age < 3) u.suppressFlags |= kSuppressed;
    if (g.lastStylusTime < s.curTimeMs && SinceStylus() < kStylusRecentMs) {
        p.stylusWithdrawn = 1;
        u.suppressFlags |= kSuppressed;
    }
    if ((u.suppressFlags & kSuppressed) != 0) u.persistFlags &= ~kPersistReported;
}

// TSA_ASAAFEStatus @ 0x6ba9c607：AFE 帧里笔的状态，没有笔帧时为 -10
uint32_t AfeStylusStatus() {
    const StylusFrame* stylus = Afe::GetFrame()->stylus;
    return stylus == nullptr ? static_cast<uint32_t>(-10) : stylus->status;
}

}  // namespace

// StylusAft_Init @ 0x6bb208d0
void Init() {
    g = {};
    Asa::ClearBtPenStatus();
}

// TSA_StylusAftProcess @ 0x6bb22b1c：笔坐标换算到显示分辨率
void Process() {
    if ((static_cast<uint16_t>(AfeStylusStatus()) & kAsaStylusInRange) != 0 && Asa::RptInRange()) {
        g.lastStylusTime = Static().curTimeMs;
    }
    if (Asa::RptPressure() != 0) {
        const FlashPrmt& fl = PrmtFlash();
        g.stylusX = static_cast<int32_t>(static_cast<uint32_t>(Asa::RptX()) * fl.dim1Res) / static_cast<int32_t>(Asa::Dim1Res());
        g.stylusY = static_cast<int32_t>(static_cast<uint32_t>(Asa::RptY()) * fl.dim2Res) / static_cast<int32_t>(Asa::Dim2Res());
        g.tiltX = Asa::RptTiltX();
        g.tiltY = Asa::RptTiltY();
    }
    const TouchTable& tt = Touches();
    for (uint8_t k = 0; k < tt.count; ++k) {
        if ((tt.unit[k].event & kEventRptOff) == 0) ProcessUnit(k);
    }
}

}  // namespace Solvers::Tsa::Core::StylusAft
