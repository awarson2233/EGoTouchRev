#include "Alg/Touch/PrevTouch.h"

#include <cstring>

#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/Touch.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Extension/Thenar/ThenarData.h"

namespace Solvers::Tsa::Core::PrevTouch {

namespace {

/// 历史下标越界时报软异常；返回下标是否可用。
bool CheckIdx(uint8_t prevIdx) {
    TsaAssert(prevIdx < kMaxTouchNum);
    return prevIdx < kMaxTouchNum;
}

// PrevTouch_HistoryQueuePush_internal @ 0x6ba6e448
// 与队尾格子相同时不入队；队满时丢掉队头。
bool QueuePushCell(uint8_t prevIdx, uint16_t dim1, uint16_t dim2) {
    PrevTouchUnit& unit = PrevTouches().unit[prevIdx];
    const int head = unit.queueHead;
    const int tail = unit.queueTail;
    if (head != tail) {
        const int last = (tail + 15) % 16;
        if (unit.queueDim1[last] == dim1 && unit.queueDim2[last] == dim2) return false;
    }
    unit.queueDim1[tail] = static_cast<uint8_t>(dim1);
    unit.queueDim2[tail] = static_cast<uint8_t>(dim2);
    unit.queueTail = static_cast<int8_t>((tail + 1) % 16);
    if (unit.queueTail == head) unit.queueHead = static_cast<int8_t>((head + 1) % 16);
    return true;
}

}  // namespace

// PrevTouch_Init @ 0x6ba6db42
void Init() {
    Release();
}

// PrevTouch_Release @ 0x6ba6da90
void Release() {
    PrevTouchTable& prev = PrevTouches();
    ZeroFill(prev);
    if (ThenarExternal().side == 4) prev.extFlags |= 2;
    if (ThenarExternal().side == 8) prev.extFlags |= 4;
}

// PrevTouch_AssignCurIdxBasedOnID @ 0x6ba6dbc9
void AssignCurIdxBasedOnId() {
    PrevTouchTable& prev = PrevTouches();
    const TouchTable& cur = Touches();
    for (uint8_t i = 0; i < prev.count; ++i) {
        uint8_t j = 0;
        while (j < cur.count && cur.unit[j].id != prev.unit[i].id) ++j;
        prev.unit[i].curIdx = j < cur.count ? j : kNoTouch;
    }
}

// PrevTouch_GetIdxBasedOnID @ 0x6ba6db56
uint8_t GetIdxBasedOnId(uint8_t id) {
    const PrevTouchTable& prev = PrevTouches();
    for (uint8_t i = 0; i < prev.count; ++i) {
        if (prev.unit[i].id == id) return i;
    }
    return kNoTouch;
}

// PrevTouch_GetIdxBasedOnCurIdx @ 0x6ba6dcc4
uint8_t GetIdxBasedOnCurIdx(uint8_t curIdx) {
    return Touches().unit[curIdx].prevIdx;
}

// PrevTouch_IsValidId @ 0x6ba6e909
bool IsValidId(uint8_t id) {
    const PrevTouchTable& prev = PrevTouches();
    for (uint8_t i = 0; i < prev.count; ++i) {
        if (prev.unit[i].id == id) return false;
    }
    const RetiredTouchIds& retired = RetiredIds();
    for (uint8_t i = 0; i < retired.count; ++i) {
        if (retired.id[i] == id) return false;
    }
    return true;
}

// PrevTouch_IsAnyExceptionDetected @ 0x6ba6e898
bool IsAnyExceptionDetected() {
    const PrevTouchTable& prev = PrevTouches();
    for (uint8_t i = 0; i < prev.count; ++i) {
        if (prev.unit[i].exceptionType != 0) return true;
    }
    return prev.exceptionFlags != 0;
}

// PrevTouch_PostProcess @ 0x6ba6e09d
void PostProcess() {
    TouchTable& cur = Touches();
    PrevTouchTable& prev = PrevTouches();

    cur.stateFlags &= ~2u;
    if (cur.liftOffCount == 0 && prev.liftOffCount != 0) cur.stateFlags |= 2;
    cur.stateFlags &= ~4u;
    if (cur.liftOffCount != 0 && prev.liftOffCount == 0) cur.stateFlags |= 4;

    prev.liftOffCount = cur.liftOffCount;
    prev.field004 = cur.field004;
    cur.newExtFlags = cur.extFlags & ~prev.extFlags;
    prev.extFlags = cur.extFlags;
    prev.newExtFlags = cur.newExtFlags;
    prev.reportedCount = cur.reportedCount;
    prev.field002 = cur.field002;
    prev.maxCoreEllipseRate = cur.maxCoreEllipseRate;
    prev.field005 = cur.field005;
    prev.coreTouchCount = cur.coreTouchCount;
    prev.reportedThisFrame = cur.reportedThisFrame;
    prev.lastReportedId = cur.lastReportedId;
    prev.gripReportedThisFrame = cur.gripReportedThisFrame;
    prev.field00B = cur.field00B;
    prev.field00C = cur.field00C;
    prev.field00D = cur.field00D;
    prev.field00E = cur.field00E;
    prev.field00F = cur.field00F;
    prev.abnormalShapeCount = cur.abnormalShapeCount;
    prev.abnormalShape2Count = cur.abnormalShape2Count;
    prev.abnormalShape3Count = cur.abnormalShape3Count;
    prev.abnormalShape4Count = cur.abnormalShape4Count;
    prev.exceptionFlags = cur.exceptionFlags & ~0x400u;
    prev.stateFlags = cur.stateFlags;
    prev.field138 = cur.field138;
    prev.lastTouchDownTime = cur.lastTouchDownTime;
    prev.lastLiftOffTime = cur.lastLiftOffTime;
    if (cur.lastLargeObjectTime != 0) prev.lastLargeObjectTime = cur.lastLargeObjectTime;

    RemoveOff();
}

// PrevTouch_RemoveOff @ 0x6ba6de46
void RemoveOff() {
    TsaStatic& stat = Static();
    RetiredTouchIds& retired = RetiredIds();
    if ((stat.processingState & kProcMsActive) != 0) retired.count = 0;

    for (uint8_t i = 0; i < Touches().count; ++i) {
        const uint8_t prevIdx = GetIdxBasedOnCurIdx(i);
        if (prevIdx >= kMaxTouchNum) continue;
        const TouchUnit& cur = Touches().unit[i];

        if ((stat.processingState & kProcMsActive) != 0 && (cur.event & TouchEvent::kLiftOffHold) != 0 &&
            retired.count < kMaxTouchNum) {
            retired.id[retired.count] = cur.id;
            ++retired.count;
        }
        if ((cur.event & (TouchEvent::kLiftOffHold | TouchEvent::kLiftOff | TouchEvent::kWindowsPadLiftOff)) == 0) {
            UnitCopy(prevIdx, i);
        } else {
            if ((cur.persistFlags & 0x20000000) != 0) {
                stat.markedLiftOffTime = stat.curTimeMs;
                stat.markedLiftOffPos = cur.pos;
            }
            UnitRemove(prevIdx);
            Touch::AssignPreIdxBasedOnId();
        }
    }
    for (uint8_t i = PrevTouches().count; i < kMaxTouchNum; ++i) UnitRelease(i);
}

// PrevTouch_UnitCopy @ 0x6ba6d300
// 只拷公共前缀，历史表独有的部分保留。
void UnitCopy(uint8_t prevIdx, uint8_t curIdx) {
    TsaAssert(prevIdx < kMaxTouchNum);
    TsaAssert(curIdx < kMaxTouchNum);
    if (prevIdx < kMaxTouchNum && curIdx < kMaxTouchNum) {
        std::memcpy(&PrevTouches().unit[prevIdx], &Touches().unit[curIdx], kTouchCommonSize);
    }
}

// PrevTouch_UnitRelease @ 0x6ba6da04
void UnitRelease(uint8_t prevIdx) {
    if (prevIdx < kMaxTouchNum) ZeroFill(PrevTouches().unit[prevIdx]);
}

// PrevTouch_UnitRemove @ 0x6ba6dcf3
void UnitRemove(uint8_t prevIdx) {
    PrevTouchTable& prev = PrevTouches();
    TsaAssert(prev.count <= kMaxTouchNum);
    if (prev.count > kMaxTouchNum) prev.count = kMaxTouchNum;
    if (prevIdx >= prev.count) return;
    for (int i = prevIdx; i < prev.count - 1; ++i) prev.unit[i] = prev.unit[i + 1];
    --prev.count;
}

// PrevTouch_UnitUpdate @ 0x6ba6d5ed
void UnitUpdate(uint8_t prevIdx, uint8_t curIdx) {
    UnitUpdateCommon(prevIdx, curIdx);
    PrevTouchUnit& prev = PrevTouches().unit[prevIdx];
    const TouchUnit& cur = Touches().unit[curIdx];
    if (cur.event == TouchEvent::kLiftOffDebounce || cur.event == TouchEvent::kLiftOff) return;

    const auto rawDelta1 = static_cast<int16_t>(cur.rawPos.dim1 - prev.rawPos.dim1);
    const auto rawDelta2 = static_cast<int16_t>(cur.rawPos.dim2 - prev.rawPos.dim2);
    const int16_t dt = cur.dt == 0 ? int16_t{1} : cur.dt;
    const auto velocity1 = static_cast<int16_t>(rawDelta1 / dt);
    const auto velocity2 = static_cast<int16_t>(rawDelta2 / dt);
    prev.rawDelta = {rawDelta1, rawDelta2};
    prev.rawAccel.dim1 = static_cast<int16_t>(velocity1 - prev.rawVelocity.dim1);
    prev.rawAccel.dim2 = static_cast<int16_t>(velocity2 - prev.rawVelocity.dim2);
    prev.rawVelocity = {velocity1, velocity2};

    const auto posDelta1 = static_cast<int16_t>(cur.pos.dim1 - prev.pos.dim1);
    const auto posDelta2 = static_cast<int16_t>(cur.pos.dim2 - prev.pos.dim2);
    prev.posDelta2.dim1 = static_cast<int16_t>(posDelta1 - prev.posDelta.dim1);
    prev.posDelta2.dim2 = static_cast<int16_t>(posDelta2 - prev.posDelta.dim2);
    prev.posDelta = {posDelta1, posDelta2};
    prev.posDeltaSumDim1 += posDelta1;
    prev.posDeltaSumDim2 += posDelta2;

    if (prev.maxSqDistMmFromPrev < cur.sqDistMmFromPrev) prev.maxSqDistMmFromPrev = cur.sqDistMmFromPrev;
    // 0 表示尚未记录
    if (prev.minField068 == 0 || cur.field068 < prev.minField068) prev.minField068 = cur.field068;
    if (prev.minField06A == 0 || cur.field06A < prev.minField06A) prev.minField06A = cur.field06A;
    if (prev.maxField068 == 0 || prev.maxField068 < cur.field068) prev.maxField068 = cur.field068;
    if (prev.maxField06A == 0 || prev.maxField06A < cur.field06A) prev.maxField06A = cur.field06A;

    if (cur.maxPeakZn <= Peaks().entry[cur.peakIdx].zn * 100 / 70) prev.strongZnTime = Static().curTimeMs;
}

// PrevTouch_UnitUpdateCommon @ 0x6ba6d427
void UnitUpdateCommon(uint8_t prevIdx, uint8_t curIdx) {
    PrevTouchUnit& prev = PrevTouches().unit[prevIdx];
    if (prev.age != 0) return;
    const PeakEntry& peak = Peaks().entry[Touches().unit[curIdx].peakIdx];
    const uint16_t cell = GridIndex(peak.dim1, peak.dim2);
    prev.minSigOffAtTouchDown = Buffers().minSigOff[cell];
    prev.timeOffAtTouchDown = Buffers().timeOff[cell];
    prev.timeRptAtTouchDown = Buffers().timeRpt[cell];
}

// PrevTouch_HistoryBufferGetNum @ 0x6ba6e9dc
uint8_t HistoryBufferGetNum(uint8_t prevIdx) {
    if (!CheckIdx(prevIdx)) return 0;
    return PrevTouches().unit[prevIdx].historyCount;
}

// PrevTouch_IsHistoryBufferFull @ 0x6ba6f3ef
bool IsHistoryBufferFull(uint8_t prevIdx) {
    if (!CheckIdx(prevIdx)) return false;
    return PrevTouches().unit[prevIdx].historyCount == kHistorySize;
}

// PrevTouch_HistoryBufferClean @ 0x6ba6e9b0
void HistoryBufferClean(uint8_t prevIdx) {
    PrevTouches().unit[prevIdx].historyCount = 0;
}

// PrevTouch_HistoryBufferPush @ 0x6ba6ea52
bool HistoryBufferPush(uint8_t prevIdx, const HistoryEntry& entry) {
    if (!CheckIdx(prevIdx)) return false;
    PrevTouchUnit& unit = PrevTouches().unit[prevIdx];
    if (unit.historyCount >= kHistorySize) {
        TsaAssert(unit.historyCount <= kHistorySize);
        unit.historyCount = kHistorySize;
        HistoryBufferRemove(prevIdx, 0);
    }
    unit.history[unit.historyCount] = entry;
    ++unit.historyCount;
    return true;
}

// PrevTouch_HistoryBufferPop @ 0x6ba6ec0d
bool HistoryBufferPop(uint8_t prevIdx, HistoryEntry& out) {
    ZeroFill(out);
    if (!CheckIdx(prevIdx)) return false;
    PrevTouchUnit& unit = PrevTouches().unit[prevIdx];
    if (unit.historyCount == 0) return false;
    out = unit.history[0];
    for (uint8_t i = 1; i < unit.historyCount; ++i) unit.history[i - 1] = unit.history[i];
    --unit.historyCount;
    return true;
}

// PrevTouch_HistoryBufferGet @ 0x6ba6ee1d
bool HistoryBufferGet(uint8_t prevIdx, uint8_t pos, HistoryEntry& out) {
    ZeroFill(out);
    if (!CheckIdx(prevIdx)) return false;
    const PrevTouchUnit& unit = PrevTouches().unit[prevIdx];
    if (pos >= unit.historyCount) return false;
    out = unit.history[pos];
    return true;
}

// PrevTouch_HistoryBufferInsert @ 0x6ba6ef44
// 已满时最后一个被挤掉，个数不再增加。
void HistoryBufferInsert(uint8_t prevIdx, uint8_t pos, const HistoryEntry& entry) {
    if (!CheckIdx(prevIdx)) return;
    PrevTouchUnit& unit = PrevTouches().unit[prevIdx];
    // 原代码用 signed char 倒数
    for (auto i = static_cast<int8_t>(unit.historyCount - 1); pos <= i; --i) {
        if (i + 1 < kHistorySize) unit.history[i + 1] = unit.history[i];
    }
    unit.history[pos] = entry;
    if (unit.historyCount < kHistorySize) ++unit.historyCount;
}

// PrevTouch_HistoryBufferRemove @ 0x6ba6f11a
void HistoryBufferRemove(uint8_t prevIdx, uint8_t pos) {
    if (!CheckIdx(prevIdx)) return;
    PrevTouchUnit& unit = PrevTouches().unit[prevIdx];
    TsaAssert(unit.historyCount <= kHistorySize);
    if (unit.historyCount > kHistorySize) unit.historyCount = kHistorySize;
    if (pos >= unit.historyCount) return;
    for (int i = pos; i < unit.historyCount - 1; ++i) unit.history[i] = unit.history[i + 1];
    --unit.historyCount;
}

// PrevTouch_HistoryQueuePush @ 0x6ba6e650
bool HistoryQueuePush(uint8_t prevIdx, uint16_t dim1, uint16_t dim2) {
    const PrevTouchUnit& unit = PrevTouches().unit[prevIdx];
    bool midPushed = false;
    const int tail = unit.queueTail;
    if (unit.queueHead != tail) {
        const int last = (tail + 15) % 16;
        midPushed = QueuePushCell(prevIdx, static_cast<uint16_t>(((dim1 >> 8) + unit.queueDim1[last]) / 2),
                                  static_cast<uint16_t>(((dim2 >> 8) + unit.queueDim2[last]) / 2));
    }
    const bool pushed = QueuePushCell(prevIdx, dim1 >> 8, dim2 >> 8);
    return midPushed || pushed;
}

}  // namespace Solvers::Tsa::Core::PrevTouch

// ══════════════════════════════════════════════════════════════════════
// 过渡期：未改写的 C 代码按原名调用。签名与 raw/include/TSAFuncDecls.h 一致。
// ══════════════════════════════════════════════════════════════════════
namespace PT = Solvers::Tsa::Core::PrevTouch;
using Solvers::Tsa::Core::HistoryEntry;

extern "C" {
void PrevTouch_Release(void) { PT::Release(); }
void PrevTouch_AssignCurIdxBasedOnID(void) { PT::AssignCurIdxBasedOnId(); }
uint8_t PrevTouch_GetIdxBasedOnID(char id) { return PT::GetIdxBasedOnId(static_cast<uint8_t>(id)); }
uint8_t PrevTouch_GetIdxBasedOnCurIdx(uint8_t curIdx) { return PT::GetIdxBasedOnCurIdx(curIdx); }
uint64_t PrevTouch_IsValidId(char id) { return PT::IsValidId(static_cast<uint8_t>(id)); }
void PrevTouch_PostProcess(void) { PT::PostProcess(); }
void PrevTouch_UnitCopy(uint8_t prevIdx, uint8_t curIdx) { PT::UnitCopy(prevIdx, curIdx); }
void PrevTouch_UnitUpdate(uint8_t prevIdx, uint8_t curIdx) { PT::UnitUpdate(prevIdx, curIdx); }
void PrevTouch_HistoryBufferClean(uint8_t prevIdx) { PT::HistoryBufferClean(prevIdx); }
uint64_t PrevTouch_HistoryBufferPush(uint8_t prevIdx, uint64_t* entry) {
    return PT::HistoryBufferPush(prevIdx, *reinterpret_cast<const HistoryEntry*>(entry));
}
uint64_t PrevTouch_HistoryBufferPop(uint8_t prevIdx, uint64_t* out) {
    return PT::HistoryBufferPop(prevIdx, *reinterpret_cast<HistoryEntry*>(out));
}
uint64_t PrevTouch_HistoryBufferGet(uint8_t prevIdx, uint8_t pos, uint64_t* out) {
    return PT::HistoryBufferGet(prevIdx, pos, *reinterpret_cast<HistoryEntry*>(out));
}
void PrevTouch_HistoryBufferInsert(uint8_t prevIdx, uint8_t pos, uint64_t* entry) {
    PT::HistoryBufferInsert(prevIdx, pos, *reinterpret_cast<const HistoryEntry*>(entry));
}
uint64_t PrevTouch_HistoryQueuePush(uint8_t prevIdx, uint16_t dim1, uint16_t dim2) {
    return PT::HistoryQueuePush(prevIdx, dim1, dim2);
}
}  // extern "C"
