#include "Alg/TouchAction/TouchAction.h"

#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/TouchData.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TouchAction {

namespace {

constexpr uint32_t kPersistReported = 0x08;   // TouchUnit::persistFlags：曾经对外报点
constexpr uint16_t kFlickMinSqDistMm = 0x14;  // 轻扫测试：离第一次报点的最小距离平方（mm²）

/// g_flickTestFailed
uint8_t& FlickTestFailed() { return Slot<uint8_t>(Bss().flickTestFailed); }

// TouchAction_ProcessUnit @ 0x6ba77226
// 已是滑动的保持；按点击处理的触点，persistFlags bit8 置位或自按下消抖开始超过 clickDuration 后改为滑动。
// 原代码不检查历史下标：为 kNoTouch 时照样按下标读，落在历史表之后的内存上。
void ProcessUnit(uint8_t curIdx, uint8_t prevIdx) {
    TouchUnit& cur = Touches().unit[curIdx];
    const PrevTouchUnit& prev = PrevTouches().unit[prevIdx];
    const DynamicPrmt& dyn = PrmtDynamic();
    uint32_t action = prev.action;
    if (action == kNone) {
        action = dyn.defaultTouchAction;
    } else if (action != kSlide &&
               ((cur.persistFlags & 0x100) != 0 || dyn.clickDuration < cur.timestamp - prev.tdDebounceTime)) {
        action = kSlide;
    }
    cur.action = action;
}

}  // namespace

// TouchAction_Process @ 0x6ba7733c
void Process() {
    for (uint8_t i = 0; i < Touches().count; ++i) {
        const uint8_t prevIdx = PrevTouch::GetIdxBasedOnCurIdx(i);
        if ((Touches().unit[i].event & TouchEvent::kLiftOff) == 0) ProcessUnit(i, prevIdx);
    }
}

// TouchAction_PostProcess @ 0x6ba773eb
// 报过点的触点抬起时：persistFlags bit12 置位则下一个触点默认按滑动，否则按点击；
// bit7 未置位且离第一次报点不足 kFlickMinSqDistMm 时记轻扫测试失败。
void PostProcess() {
    TouchTable& touches = Touches();
    for (uint8_t i = 0; i < touches.count; ++i) {
        const TouchUnit& unit = touches.unit[i];
        if ((unit.event & TouchEvent::kLiftOff) == 0 || (unit.persistFlags & kPersistReported) == 0) continue;
        const uint32_t action = (unit.persistFlags & 0x1000) == 0 ? kClick : kSlide;
        if ((unit.persistFlags & 0x80) == 0 && unit.rptSqDistMmFromFirstRpt < kFlickMinSqDistMm) FlickTestFailed() = 1;
        PrmtDynamic().defaultTouchAction = action;
    }
}

// TouchAction_IsFlickCheckPassed @ 0x6ba773d9
bool IsFlickCheckPassed() { return FlickTestFailed() == 0; }

}  // namespace Solvers::Tsa::Core::TouchAction

namespace TA = Solvers::Tsa::Core::TouchAction;
