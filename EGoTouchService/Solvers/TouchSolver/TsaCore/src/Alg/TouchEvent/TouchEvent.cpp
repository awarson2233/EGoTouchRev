#include "Alg/TouchEvent/TouchEvent.h"

#include <array>

#include "Alg/Debounce/Debounce.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/Touch.h"
#include "App/Sys.h"
#include "App/TsaStatic.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TouchEvent {

namespace {

constexpr uint32_t kSysNoStrongTap = 0xc00000;  // TsaStatic::systemFlags 置任一位时 strongTapCount 清零
constexpr uint16_t kStrongTapMaxMajorMm = 0x15;
constexpr uint16_t kStrongTapCountMax = 5;

TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }

bool WindowsPadEnabled() { return static_cast<char>(TSA_IsWindowsPadFeatureEnabled()) != 0; }

/// 本帧时间与 t 之差的低 32 位。
uint32_t MsSince(uint64_t t) { return static_cast<uint32_t>(Static().curTimeMs - t); }

/// 把历史触点拷到当前表末尾的新槽位并互相记下下标，返回新槽位。调用前确认当前表未满。
uint8_t AppendFromPrev(uint8_t prevIdx) {
    TouchTable& touches = Touches();
    const uint8_t curIdx = touches.count;
    Touch::UnitCopy(curIdx, prevIdx);
    return curIdx;
}
void LinkAppended(uint8_t curIdx, uint8_t prevIdx) {
    Cur(curIdx).prevIdx = prevIdx;
    Prev(prevIdx).curIdx = curIdx;
    ++Touches().count;
}

/// 按下防抖期间判出快速点击、且无异常的触点抬起时先保持一帧（kLiftOffHold），否则直接是 liftOffEvent。
uint32_t LiftOffEventOf(uint8_t curIdx, uint8_t prevIdx, uint32_t liftOffEvent) {
    const PrevTouchUnit& prev = Prev(prevIdx);
    if ((prev.persistFlags & Debounce::kPersistFastClick) == 0 || prev.event != kTouchDownDebounce ||
        Touch::IsException(curIdx, prevIdx)) {
        return liftOffEvent;
    }
    return kLiftOffHold;
}

// TE_GetNextID @ 0x6ba77536
// 从 prevOccupiedID（原代码里没有写入者，恒为 0）之后轮询 1..20，取第一个历史表与当前表都没在用的 ID；都占用时为 0。
uint8_t GetNextId() {
    const uint8_t base = Image::At<uint8_t>(0x6bb80f90);
    const TouchTable& touches = Touches();
    for (uint8_t step = 1; step <= kMaxTouchNum; ++step) {
        auto id = static_cast<uint8_t>(step + base);
        if (id > kMaxTouchNum) id = static_cast<uint8_t>(id - (kMaxTouchNum - 1));
        bool used = !PrevTouch::IsValidId(id);
        for (int8_t i = 0; i < touches.count; ++i) {
            if (touches.unit[i].id == id) {
                used = true;
                break;
            }
        }
        if (!used) return id;
    }
    return 0;
}

// TE_LiftOffReportProcess @ 0x6ba77d4b
// 报抬起：拷进当前表并记 kLiftOff（快速点击先 kLiftOffHold；WindowsPad 下一律 kLiftOff），并统计 strongTapCount。
// 当前表已满时返回 kNoTouch。
uint8_t LiftOffReportProcess(uint8_t prevIdx) {
    if (Touches().count >= kMaxTouchNum) return kNoTouch;
    const uint8_t curIdx = AppendFromPrev(prevIdx);
    TouchUnit& cur = Cur(curIdx);
    cur.event = LiftOffEventOf(curIdx, prevIdx, kLiftOff);
    // WindowsPad 下走到这里时历史事件只会是 kWindowsPadLiftOff / kLiftOffDebounce，上一行已是 kLiftOff；照原代码保留
    if (WindowsPadEnabled()) cur.event = kLiftOff;
    LinkAppended(curIdx, prevIdx);

    TsaStatic& stat = Static();
    const bool strongTap = (stat.systemFlags & kSysNoStrongTap) == 0 && PrmtFlash().fingerThold * 2 < cur.maxPeakSig &&
                           (cur.persistFlags & 0x80) == 0 && cur.maxMajorMm < kStrongTapMaxMajorMm;
    if (stat.strongTapCount < kStrongTapCountMax) stat.strongTapCount = strongTap ? stat.strongTapCount + 1 : 0;
    return curIdx;
}

// TE_LiftOffProcess @ 0x6ba78511
// 抬起防抖：防抖帧数未满时以 kLiftOffDebounce 留在当前表，满了报抬起。
void LiftOffProcess(uint8_t prevIdx) {
    PrevTouchUnit& prev = Prev(prevIdx);
    uint8_t curIdx = kNoTouch;
    if (prev.event != kLiftOffDebounce) prev.debounceFrames = 0;
    const uint16_t frames = Debounce::LiftOffNum(prevIdx);
    if (prev.debounceFrames < frames) {
        ++prev.debounceFrames;
        if (Touches().count < kMaxTouchNum) {
            curIdx = AppendFromPrev(prevIdx);
            Cur(curIdx).event = kLiftOffDebounce;
            Cur(curIdx).age = static_cast<uint16_t>(prev.age + 1);
            LinkAppended(curIdx, prevIdx);
        }
    } else {
        curIdx = LiftOffReportProcess(prevIdx);
    }
    if (curIdx != kNoTouch) PrevTouch::UnitUpdate(prevIdx, curIdx);
}

// TE_WindowsLiftOffProcess @ 0x6ba78033
// WindowsPad 的抬起：防抖帧数多算一帧，最后一帧以 kWindowsPadLiftOff（快速点击为 kLiftOffHold，并把这一帧退回去）
// 留在当前表，之后报抬起。
void WindowsLiftOffProcess(uint8_t prevIdx) {
    PrevTouchUnit& prev = Prev(prevIdx);
    uint8_t curIdx = kNoTouch;
    if (prev.event != kWindowsPadLiftOff && prev.event != kLiftOffDebounce) prev.debounceFrames = 0;
    const auto frames = static_cast<uint16_t>(Debounce::LiftOffNum(prevIdx) + 1);
    const int lastFrame = frames - 1;
    if (prev.debounceFrames < lastFrame) {
        ++prev.debounceFrames;
        if (Touches().count < kMaxTouchNum) {
            curIdx = AppendFromPrev(prevIdx);
            Cur(curIdx).event = kLiftOffDebounce;
            Cur(curIdx).age = static_cast<uint16_t>(prev.age + 1);
            LinkAppended(curIdx, prevIdx);
        }
    } else if (prev.debounceFrames == lastFrame || prev.event == kLiftOffHold) {
        ++prev.debounceFrames;
        if (Touches().count < kMaxTouchNum) {
            curIdx = AppendFromPrev(prevIdx);
            Cur(curIdx).event = LiftOffEventOf(curIdx, prevIdx, kWindowsPadLiftOff);
            if (Cur(curIdx).event == kLiftOffHold) --prev.debounceFrames;
            Cur(curIdx).age = static_cast<uint16_t>(prev.age + 1);
            LinkAppended(curIdx, prevIdx);
        }
    } else {
        curIdx = LiftOffReportProcess(prevIdx);
    }
    if (curIdx != kNoTouch) PrevTouch::UnitUpdate(prevIdx, curIdx);
}

// TE_TouchDownDebounce @ 0x6ba7772b
// 按下防抖：未满 DB 给出的帧数时 kTouchDownDebounce，满了 kTouchDown 并记 persistFlags2 bit10。
void TouchDownDebounce(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Cur(curIdx);
    PrevTouchUnit& prev = Prev(prevIdx);
    prev.tdDebouncePos = cur.pos;
    prev.tdDebounceTime = Static().curTimeMs;
    Touch::UnitUpdate(curIdx, prevIdx);
    const uint16_t frames = Debounce::TouchDownNum(prevIdx, curIdx);
    if (prev.debounceFrames < frames) {
        ++prev.debounceFrames;
        cur.event = kTouchDownDebounce;
    } else {
        cur.event = kTouchDown;
        cur.persistFlags2 |= 0x400;
    }
    cur.age = static_cast<uint16_t>(prev.age + 1);
    cur.durationMs = MsSince(prev.tdDebounceTime);
    PrevTouch::UnitUpdate(prevIdx, curIdx);
}

// TE_TouchDownProcess @ 0x6ba779f3
// 新触点：分配 ID；历史表未满时在末尾开一项，记下起点等初值后进入按下防抖。
void TouchDownProcess(uint8_t curIdx) {
    const uint8_t id = GetNextId();
    if (id == 0) return;
    TouchUnit& cur = Cur(curIdx);
    cur.id = id;
    PrevTouchTable& prevTable = PrevTouches();
    if (prevTable.count >= kMaxTouchNum) return;
    const uint8_t prevIdx = prevTable.count;
    PrevTouch::UnitCopy(prevIdx, curIdx);
    PrevTouchUnit& prev = Prev(prevIdx);
    prev.startPos = cur.pos;
    prev.field360 = cur.field084;
    prev.field3D4 = Point{cur.field068, cur.field06A};
    prev.smoothEllipseRate = cur.ellipseRate;
    prev.curIdx = curIdx;
    prev.age = 0;
    prev.field380 = Static().curTimeMs;
    prev.field44C = 0;
    prev.field44D = 0;
    cur.prevIdx = prevIdx;
    TouchDownDebounce(curIdx, prevIdx);
    ++prevTable.count;
}

// TE_MoveProcess @ 0x6ba775fa
void MoveProcess(uint8_t curIdx, uint8_t prevIdx) {
    Touch::UnitUpdate(curIdx, prevIdx);
    TouchUnit& cur = Cur(curIdx);
    PrevTouchUnit& prev = Prev(prevIdx);
    prev.debounceFrames = 0;
    cur.event = kMove;
    cur.age = static_cast<uint16_t>(prev.age + 1);
    cur.durationMs = MsSince(prev.tdDebounceTime);
    PrevTouch::UnitUpdate(prevIdx, curIdx);
}

// TE_AssertProcess @ 0x6ba7872d
// 两张表的个数相同，且下标互相覆盖：每个历史触点都映射到一个当前下标、每个当前触点都映射到一个历史下标。
// 不成立时返回 true（原代码另外打印日志）。两张表的个数不超过 kMaxTouchNum。
bool AssertProcess() {
    const uint8_t prevCount = PrevTouches().count;
    const uint8_t curCount = Touches().count;
    bool broken = prevCount != curCount;
    std::array<uint8_t, kMaxTouchNum> covered{};
    for (uint8_t i = 0; i < PrevTouches().count; ++i) {
        const uint8_t curIdx = Touch::GetIdxBasedOnPreIdx(i);
        if (curIdx < kMaxTouchNum) covered[curIdx] = 1;
        else broken = true;
    }
    for (uint8_t i = 0; i < PrevTouches().count; ++i) {
        if (covered[i] != 1) broken = true;
    }
    covered.fill(0);
    for (uint8_t i = 0; i < Touches().count; ++i) {
        const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(i);
        if (prevIdx < kMaxTouchNum) covered[prevIdx] = 1;
        else broken = true;
    }
    for (uint8_t i = 0; i < Touches().count; ++i) {
        if (covered[i] != 1) broken = true;
    }
    return broken;
}

}  // namespace

// TE_Process @ 0x6ba789d9
void Process() {
    const uint8_t curCount = Touches().count;  // 抬起流程追加到表尾的触点不再参与下面的按下 / 移动
    for (uint8_t prevIdx = 0; prevIdx < PrevTouches().count; ++prevIdx) {
        if (Touch::GetIdxBasedOnPreIdx(prevIdx) != kNoTouch) continue;
        if (WindowsPadEnabled()) {
            WindowsLiftOffProcess(prevIdx);
        } else {
            LiftOffProcess(prevIdx);
        }
    }
    for (uint8_t curIdx = 0; curIdx < curCount; ++curIdx) {
        if (Cur(curIdx).id == 0) {
            TouchDownProcess(curIdx);
            continue;
        }
        const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(curIdx);
        if ((Prev(prevIdx).event & kTouchDownDebounce) == 0) {
            MoveProcess(curIdx, prevIdx);
        } else {
            TouchDownDebounce(curIdx, prevIdx);
        }
    }
    if (AssertProcess()) {
        Touch::Clean();
        PrevTouch::Release();
    }
    if ((Static().processingState & kProcMsActive) != 0) {
        Touch::ProcessAftTouchEvent();
        Sys::ProcessAftTouchEvent();
    }
}

}  // namespace Solvers::Tsa::Core::TouchEvent

extern "C" {
void TE_Init(void) {}
void TE_Process(void) { Solvers::Tsa::Core::TouchEvent::Process(); }
}  // extern "C"
