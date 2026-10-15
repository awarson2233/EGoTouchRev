#include "Alg/EdgeRecovery/EdgeRecovery.h"

#include <cstdlib>

#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/Touch/TouchGeometry.h"
#include "Alg/TouchSize/TouchSize.h"
#include "App/Report.h"
#include "App/TsaOut.h"
#include "Common/Image.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::EdgeRecovery {

namespace {

constexpr uint32_t kPersistReported = 0x8;        // PrevTouchUnit::persistFlags：报过点
constexpr uint32_t kPersistEdgeCorrected = 0x80;  // TouchUnit::persistFlags：做过边缘修正（DB 不再做抬起防抖）
constexpr uint32_t kPersistMoveInPending = 0x400; // 等下一帧再做 move-in 修正
constexpr uint32_t kVolatileMoveIn = 0x20;        // TouchUnit::volatileFlags：本帧做了 move-in 修正
constexpr uint32_t kVolatileMoveOut = 0x40;       // 本帧做了 move-out 修正
// PrevTouchUnit::edgeFlags：外推位置越过的逻辑边
constexpr uint32_t kOutDim1Near = 0x10;
constexpr uint32_t kOutDim1Far = 0x20;
constexpr uint32_t kOutDim2Near = 0x40;
constexpr uint32_t kOutDim2Far = 0x80;

constexpr uint16_t kMaxMoveInMajorMm = 0x1d;      // 长轴不超过它才做 move-in 修正
constexpr int16_t kBottomMoveInMinPeakSig = 1000;
constexpr int kBottomMoveInMinRise = 0x32;        // 底边上划：相对历史末点至少上移的距离
constexpr int kBottomMoveInMinTotalRise = 100;    // 相对起点至少上移的距离
constexpr int kBottomMoveInMaxDrift = 0x100;      // 横向偏移不超过一格
constexpr uint32_t kMoveInMinStepSq = 0x9c4;      // move-in：历史两点、末点到当前点的最小平方距离
constexpr uint32_t kMoveInMinSpanSq = 10000;      // 首点到当前点的最小平方距离
constexpr int kBottomZoneHeight = 0x36;           // 底部区域：显示坐标离底边的距离
constexpr int kBottomZoneSideMargin = 100;        // 以及离左右边的距离

TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
// 原代码不检查历史下标：为 kNoTouch 时照样按下标读写，落在历史表之后的内存上。
PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }

uint16_t& LimitMm() { return Image::At<uint16_t>(0x6bb806c0); }                 // g_erLimit
uint8_t& MoveInCorrectedButDisabled() { return Image::At<uint8_t>(0x6bb806c2); }  // g_flagMoveInCorrectedButDisabled

Point PosOf(const HistoryEntry& e) { return {static_cast<uint16_t>(e.pos.dim1), static_cast<uint16_t>(e.pos.dim2)}; }

void MarkMoveInCorrected(TouchUnit& cur) {
    cur.volatileFlags |= kVolatileMoveIn;
    cur.persistFlags |= kPersistEdgeCorrected;
}

// ER_GetPrmt @ 0x6ba56ec0
// 所在边的 move-in 修正区间；顺带记下这条边的边缘宽度（mm）。
const ErEdgePrmt& EdgePrmt(uint32_t dimEdge) {
    const auto side = Report::DimEdgeToSide(dimEdge);
    const auto edge = Report::SideToEdge(side);
    LimitMm() = PrmtFlash().erEdgeMm[edge].minDistMm;
    return PrmtFlashConst().erEdge[edge];
}

// ER_MMPrmtToPixelPrmt @ 0x6ba56e50
// mm 换成显示像素，往里收 5 个像素，再换回所在维的 Q8.8 传感器坐标。
uint16_t MmToEdgePixel(uint8_t mm, int side) {
    const auto displayPx = static_cast<uint16_t>(TouchSize::MmToDisplayPixel(mm));
    const auto pos = static_cast<uint16_t>((displayPx >> 4) - 5);
    const auto dimEdge = static_cast<int>(Report::SideToDimEdge(side));
    return static_cast<uint16_t>(dimEdge == 1 || dimEdge == 2 ? Out::RevertDim1Pos(pos) : Out::RevertDim2Pos(pos));
}

// ER_IsInBottomZone @ 0x6ba572bf
// 显示坐标在底边 kBottomZoneHeight 以内，且离左右两边都超过 kBottomZoneSideMargin。
bool IsInBottomZone(Point pos) {
    const FlashPrmt& fl = PrmtFlash();
    const uint16_t x = Out::Pixel2DisplayDim1(pos.dim1);
    const uint16_t y = Out::Pixel2DisplayDim2(pos.dim2);
    return y >= static_cast<uint16_t>(fl.field09A) - kBottomZoneHeight &&
           x >= static_cast<uint16_t>(fl.field094) + kBottomZoneSideMargin &&
           x < static_cast<uint16_t>(fl.field096) - kBottomZoneSideMargin;
}

bool Within(int a, int b, int range) { return std::abs(a - b) < range; }

// ER_BottomMoveInCorrection @ 0x6ba57355
// 底边上划：开关打开、触点不大且信号够强时，看历史里的起点是否在底部区域、之后一路上移且横向基本不动。
// 成立时把历史截到只剩 move-in 计数那么多点。
bool BottomMoveInCorrection(uint8_t curIdx, uint8_t prevIdx) {
    const TouchUnit& cur = Cur(curIdx);
    if (PrmtFlash().erEdgeMm[0].field4 == 0 || cur.majorMm > kMaxMoveInMajorMm || cur.maxPeakSig < kBottomMoveInMinPeakSig) {
        return false;
    }
    const uint16_t frames = Prev(prevIdx).debounceFrames;
    const Point now = cur.rptPos;
    HistoryEntry entry;
    bool hit = false;
    if (frames == 1) {
        const uint8_t num = PrevTouch::HistoryBufferGetNum(prevIdx);
        if (num != 0) {
            PrevTouch::HistoryBufferGet(prevIdx, static_cast<uint8_t>(num - 1), entry);
            const Point last = PosOf(entry);
            hit = IsInBottomZone(last) && last.dim2 - now.dim2 > kBottomMoveInMinRise &&
                  Within(now.dim1, last.dim1, kBottomMoveInMaxDrift);
        }
    } else if (frames > 1 && frames < 4 && frames <= PrevTouch::HistoryBufferGetNum(prevIdx)) {
        const uint8_t num = PrevTouch::HistoryBufferGetNum(prevIdx);
        PrevTouch::HistoryBufferGet(prevIdx, static_cast<uint8_t>(num - frames), entry);
        const Point first = PosOf(entry);
        PrevTouch::HistoryBufferGet(prevIdx, static_cast<uint8_t>(num - 1), entry);
        const Point last = PosOf(entry);
        hit = IsInBottomZone(first) && last.dim2 - now.dim2 > kBottomMoveInMinRise &&
              first.dim2 - now.dim2 > kBottomMoveInMinTotalRise && Within(first.dim1, last.dim1, kBottomMoveInMaxDrift) &&
              Within(last.dim1, now.dim1, kBottomMoveInMaxDrift) && Within(now.dim1, first.dim1, kBottomMoveInMaxDrift);
    }
    if (hit) {
        while (PrevTouch::HistoryBufferGetNum(prevIdx) > Prev(prevIdx).debounceFrames &&
               PrevTouch::HistoryBufferPop(prevIdx, entry)) {
        }
    }
    return hit;
}

// ER_IsTouchNeedMoveInCorrection @ 0x6ba5719b
// 到所在逻辑边的距离落在这条边的 (minDist, maxDist) 内、长轴不大、且所在位置允许修正。
// minDist 为 0 表示这条边不做 move-in 修正，此时记下 MoveInCorrectedButDisabled。
bool IsTouchNeedMoveInCorrection(uint8_t curIdx) {
    const TouchUnit& cur = Cur(curIdx);
    const Point pos = cur.rptPos;
    MoveInCorrectedButDisabled() = 0;
    const uint32_t dimEdge = Report::GetDimEdge(pos.dim1, pos.dim2);
    const ErEdgePrmt& prmt = EdgePrmt(dimEdge);
    const auto dist = static_cast<uint16_t>(Touch::GetDistToLogicEdge(static_cast<int>(dimEdge), static_cast<int16_t>(pos.dim1),
                                                                      static_cast<int16_t>(pos.dim2)));
    if (prmt.minDist == 0) {
        MoveInCorrectedButDisabled() = 1;
        return false;
    }
    return prmt.minDist < dist && dist < prmt.maxDist && cur.majorMm <= kMaxMoveInMajorMm;
}

// ER_MoveInCorrection @ 0x6ba57786
// 历史只留最新两点，按这两点的位置与时间步长往前（向边缘方向）外推，逐点插到历史开头，
// 直到碰到逻辑边缘、步长有一维为 0 或历史满。
void MoveInCorrection(uint8_t prevIdx) {
    if (PrevTouch::HistoryBufferGetNum(prevIdx) < 2) return;
    HistoryEntry entry;
    while (PrevTouch::HistoryBufferGetNum(prevIdx) > 2) PrevTouch::HistoryBufferPop(prevIdx, entry);
    PrevTouch::HistoryBufferGet(prevIdx, 0, entry);
    SPoint pt = entry.pos;
    uint64_t time = entry.time;
    PrevTouch::HistoryBufferGet(prevIdx, 1, entry);
    const SPoint step = {static_cast<int16_t>(entry.pos.dim1 - pt.dim1), static_cast<int16_t>(entry.pos.dim2 - pt.dim2)};
    const int32_t timeStep = static_cast<int32_t>(entry.time) - static_cast<int32_t>(time);

    const FlashConstPrmt& fc = PrmtFlashConst();
    const int dim1Last = fc.dim1FarEdge - 1;
    const int dim2Last = fc.dim2FarEdge - 1;
    while (step.dim1 != 0 && step.dim2 != 0 && !PrevTouch::IsHistoryBufferFull(prevIdx)) {
        pt.dim1 = static_cast<int16_t>(pt.dim1 - step.dim1);
        pt.dim2 = static_cast<int16_t>(pt.dim2 - step.dim2);
        entry.time = time - timeStep;
        if (pt.dim1 < fc.dim1NearEdge) pt.dim1 = static_cast<int16_t>(fc.dim1NearEdge);
        if (pt.dim2 < fc.dim2NearEdge) pt.dim2 = static_cast<int16_t>(fc.dim2NearEdge);
        if (pt.dim1 > dim1Last) pt.dim1 = static_cast<int16_t>(dim1Last);
        if (pt.dim2 > dim2Last) pt.dim2 = static_cast<int16_t>(dim2Last);
        entry.pos = pt;
        time = entry.time;
        PrevTouch::HistoryBufferInsert(prevIdx, 0, entry);
        if (pt.dim1 == fc.dim1NearEdge || pt.dim2 == fc.dim2NearEdge || pt.dim1 == dim1Last || pt.dim2 == dim2Last) return;
    }
}

// ER_MoveInCheck @ 0x6ba57a07
// 历史末两点与当前点都拉得够开，且首点离当前点比离边缘远（确实是从边缘外滑进来的）时做 move-in 修正。
bool MoveInCheck(uint8_t curIdx, uint8_t prevIdx) {
    const uint8_t num = PrevTouch::HistoryBufferGetNum(prevIdx);
    if (num < 2) return false;
    HistoryEntry entry;
    PrevTouch::HistoryBufferGet(prevIdx, static_cast<uint8_t>(num - 2), entry);
    const Point first = PosOf(entry);
    PrevTouch::HistoryBufferGet(prevIdx, static_cast<uint8_t>(PrevTouch::HistoryBufferGetNum(prevIdx) - 1), entry);
    const Point last = PosOf(entry);
    const Point now = Cur(curIdx).rptPos;
    const uint32_t firstToLast = Touch::GetSqDistPixel(first.dim1, first.dim2, last.dim1, last.dim2);
    const uint32_t lastToNow = Touch::GetSqDistPixel(last.dim1, last.dim2, now.dim1, now.dim2);
    const uint32_t firstToNow = Touch::GetSqDistPixel(first.dim1, first.dim2, now.dim1, now.dim2);
    const auto firstToEdge = static_cast<uint32_t>(Touch::GetSqDistPixelToEdge(
        static_cast<int16_t>(first.dim1), static_cast<int16_t>(first.dim2), static_cast<int16_t>(last.dim1), static_cast<int16_t>(last.dim2)));
    const bool moveIn = firstToLast >= kMoveInMinStepSq && lastToNow >= kMoveInMinStepSq && firstToNow >= kMoveInMinSpanSq &&
                        firstToNow >= firstToEdge;
    if (moveIn) MoveInCorrection(prevIdx);
    return moveIn;
}

// ER_TouchDownProcess @ 0x6ba57ffd
// 刚报按下的触点：底边上划或 move-in 判定成立时修正。在修正区间内但 move-in 计数不到 2 时，
// 压回 kTouchDownDebounce 并挂起，下一帧（仍是按下）再判 move-in。
void TouchDownProcess(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    PrevTouchUnit& prev = Prev(prevIdx);
    if ((cur.persistFlags & kPersistMoveInPending) != 0) {
        cur.persistFlags &= ~kPersistMoveInPending;
        if (BottomMoveInCorrection(curIdx, prevIdx) || MoveInCheck(curIdx, prevIdx)) MarkMoveInCorrected(cur);
        return;
    }
    if (BottomMoveInCorrection(curIdx, prevIdx)) {
        MarkMoveInCorrected(cur);
    } else if (IsTouchNeedMoveInCorrection(curIdx)) {
        if (prev.debounceFrames < 2) {
            cur.persistFlags |= kPersistMoveInPending;
            cur.event = TouchEvent::kTouchDownDebounce;
            ++prev.debounceFrames;
            if (cur.field0F8 < 2) cur.field0F8 = 2;
        } else if (MoveInCheck(curIdx, prevIdx)) {
            MarkMoveInCorrected(cur);
        }
    }
}

// ER_MoveOutCheck @ 0x6ba57bd3
// 按当前位移与位移变化外推下一帧的位置，记下越过的逻辑边。
// Dim2 的近端也和 dim1NearEdge 比（原代码如此）。
bool MoveOutCheck(uint8_t prevIdx) {
    PrevTouchUnit& prev = Prev(prevIdx);
    const FlashConstPrmt& fc = PrmtFlashConst();
    bool out = false;
    const auto next1 = static_cast<int16_t>(prev.posDelta2.dim1 + prev.pos.dim1 + prev.posDelta.dim1);
    if (next1 <= fc.dim1NearEdge) {
        prev.edgeFlags |= kOutDim1Near;
        out = true;
    } else if (next1 >= fc.dim1FarEdge - 1) {
        prev.edgeFlags |= kOutDim1Far;
        out = true;
    }
    const auto next2 = static_cast<int16_t>(prev.posDelta2.dim2 + prev.pos.dim2 + prev.posDelta.dim2);
    if (next2 <= fc.dim1NearEdge) {
        prev.edgeFlags |= kOutDim2Near;
        out = true;
    } else if (next2 >= fc.dim2FarEdge - 1) {
        prev.edgeFlags |= kOutDim2Far;
        out = true;
    }
    return out;
}

// ER_MoveOutCorrection @ 0x6ba57e7d
// 把上报坐标贴到越过的逻辑边上。
void MoveOutCorrection(uint8_t prevIdx, uint8_t curIdx) {
    const uint32_t flags = Prev(prevIdx).edgeFlags;
    const FlashConstPrmt& fc = PrmtFlashConst();
    Point& pos = Cur(curIdx).rptPos;
    if ((flags & kOutDim1Near) != 0) pos.dim1 = fc.dim1NearEdge;
    if ((flags & kOutDim1Far) != 0) pos.dim1 = static_cast<uint16_t>(fc.dim1FarEdge - 1);
    if ((flags & kOutDim2Near) != 0) pos.dim2 = fc.dim2NearEdge;
    if ((flags & kOutDim2Far) != 0) pos.dim2 = static_cast<uint16_t>(fc.dim2FarEdge - 1);
}

}  // namespace

// ER_Init @ 0x6ba56f76
void Init() {
    const FlashPrmt& fl = PrmtFlash();
    FlashConstPrmt& fc = PrmtFlashConst();
    constexpr int kSides[] = {1, 2, 4, 8};  // 上、下、左、右
    for (int edge = 0; edge < 4; ++edge) {
        fc.erEdge[edge].minDist = MmToEdgePixel(static_cast<uint8_t>(fl.erEdgeMm[edge].minDistMm), kSides[edge]);
        fc.erEdge[edge].maxDist = MmToEdgePixel(static_cast<uint8_t>(fl.erEdgeMm[edge].maxDistMm), kSides[edge]);
    }
    (void)EdgePrmt(Report::SideToDimEdge(1));
}

// ER_Process @ 0x6ba584b3
void Process() {
    for (uint8_t curIdx = 0; curIdx < Touches().count; ++curIdx) {
        const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(curIdx);
        TouchUnit& cur = Cur(curIdx);
        if ((cur.event & TouchEvent::kTouchDown) != 0) {
            TouchDownProcess(curIdx, prevIdx);
        } else if ((cur.event & TouchEvent::kLiftOff) != 0 && (cur.persistFlags & kPersistMoveInPending) != 0) {
            // 挂起 move-in 的触点没等到下一帧就抬起：当作修正过
            cur.persistFlags &= ~kPersistMoveInPending;
            MarkMoveInCorrected(cur);
        }
        if ((cur.event == TouchEvent::kLiftOffDebounce || cur.event == TouchEvent::kLiftOff) &&
            (Prev(prevIdx).persistFlags & kPersistReported) != 0 && MoveOutCheck(prevIdx)) {
            cur.volatileFlags |= kVolatileMoveOut;
            MoveOutCorrection(prevIdx, curIdx);
            if (cur.event == TouchEvent::kLiftOff) cur.event = TouchEvent::kLiftOffHold;
        }
    }
}

bool IsMoveInCorrectedButDisabled() { return MoveInCorrectedButDisabled() != 0; }

// ER_GetLimit @ 0x6ba57101
uint16_t GetLimit() { return static_cast<uint16_t>(static_cast<uint16_t>(TouchSize::MmToDisplayPixel(static_cast<uint8_t>(LimitMm()))) >> 4); }

}  // namespace Solvers::Tsa::Core::EdgeRecovery
