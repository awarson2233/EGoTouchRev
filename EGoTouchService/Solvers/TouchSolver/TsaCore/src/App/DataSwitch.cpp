// 数据集切换（原 src/App/DataSwitch.c）：区域图、触点表、峰表都有屏幕网格（Grid）与手势（Hand）
// 两套，各模块经 Buffers().tz / Touches() / PrevTouches() / Peaks() / PrevPeaks() 的当前指针访问；
// 处理手势前切到 Hand，处理完切回 Grid。

#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/TouchData.h"
#include "App/DataSwitch.h"
#include "App/TsaBuffer.h"
#include "Common/Image.h"

namespace Solvers::Tsa::Core::DataSwitch {

namespace {

struct DataSet {
    uint64_t tz;          // 区域图缓冲指针所在的槽
    uint64_t touches;
    uint64_t prevTouches;
    uint64_t peaks;
    uint64_t prevPeaks;
};
constexpr DataSet kGrid{0x6bb81360, 0x6bbc6900, 0x6bbc0e40, 0x6bbc0980, 0x6bbc0d00};
constexpr DataSet kHand{0x6bba3508, 0x6bc84f40, 0x6bc86f00, 0x6bc86d00, 0x6bc84e40};

void Switch(const DataSet& d) {
    Buffers().tz = Image::At<int8_t*>(d.tz);
    Image::At<TouchTable*>(0x6bb4f1d0) = &Image::At<TouchTable>(d.touches);
    Image::At<PrevTouchTable*>(0x6bb4f1c0) = &Image::At<PrevTouchTable>(d.prevTouches);
    Image::At<PeakTable*>(0x6bb4f100) = &Image::At<PeakTable>(d.peaks);
    Image::At<PrevPeakTable*>(0x6bb4f110) = &Image::At<PrevPeakTable>(d.prevPeaks);
}

}  // namespace

// DataSwitch_ToGrid @ 0x6ba906d0
void ToGrid() { Switch(kGrid); }
// DataSwitch_ToHand @ 0x6ba907ce
void ToHand() { Switch(kHand); }

}  // namespace Solvers::Tsa::Core::DataSwitch

extern "C" {
void DataSwitch_ToGrid(void) { Solvers::Tsa::Core::DataSwitch::ToGrid(); }
void DataSwitch_ToHand(void) { Solvers::Tsa::Core::DataSwitch::ToHand(); }
}
