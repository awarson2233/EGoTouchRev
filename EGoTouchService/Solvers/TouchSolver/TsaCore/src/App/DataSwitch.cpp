// 数据集切换（原 src/App/DataSwitch.c）：区域图、触点表、峰表都有屏幕网格（Grid）与手势（Hand）
// 两套，各模块经 Buffers().tz / Touches() / PrevTouches() / Peaks() / PrevPeaks() 的当前指针访问；
// 处理手势前切到 Hand，处理完切回 Grid。

#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/TouchData.h"
#include "App/DataSwitch.h"
#include "App/TsaBuffer.h"
#include "Common/Bss.h"

namespace Solvers::Tsa::Core {

// 当前数据集，初值是 Grid
TouchTable* g_touches = &Slot<TouchTable>(Bss().touchesGrid);
PrevTouchTable* g_prevTouches = &Slot<PrevTouchTable>(Bss().prevTouchesGrid);
PeakTable* g_peaks = &Slot<PeakTable>(Bss().peaksGrid);
PrevPeakTable* g_prevPeaks = &Slot<PrevPeakTable>(Bss().prevPeaksGrid);

namespace DataSwitch {

namespace {

struct DataSet {
    int8_t* const& tz;  // 区域图缓冲指针
    TouchTable& touches;
    PrevTouchTable& prevTouches;
    PeakTable& peaks;
    PrevPeakTable& prevPeaks;
};

void Switch(const DataSet& d) {
    Buffers().tz = d.tz;
    g_touches = &d.touches;
    g_prevTouches = &d.prevTouches;
    g_peaks = &d.peaks;
    g_prevPeaks = &d.prevPeaks;
}

}  // namespace

// DataSwitch_ToGrid @ 0x6ba906d0
void ToGrid() {
    Switch({Buffers().tzGrid, Slot<TouchTable>(Bss().touchesGrid), Slot<PrevTouchTable>(Bss().prevTouchesGrid),
            Slot<PeakTable>(Bss().peaksGrid), Slot<PrevPeakTable>(Bss().prevPeaksGrid)});
}
// DataSwitch_ToHand @ 0x6ba907ce
void ToHand() {
    Switch({Slot<int8_t*, 0x8>(Bss().handBuffers), Slot<TouchTable>(Bss().touchesHand), Slot<PrevTouchTable>(Bss().prevTouchesHand),
            Slot<PeakTable>(Bss().peaksHand), Slot<PrevPeakTable>(Bss().prevPeaksHand)});
}

}  // namespace DataSwitch

}  // namespace Solvers::Tsa::Core
