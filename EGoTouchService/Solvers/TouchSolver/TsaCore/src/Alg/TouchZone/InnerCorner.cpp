#include "Alg/TouchZone/InnerCorner.h"

#include <algorithm>

#include "Alg/TouchZone/TouchZoneData.h"
#include "App/TsaBuffer.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::InnerCorner {

namespace {

// TouchZone_GetInnerCornerSigRatio @ 0x6ba883a8
// 两个边缘格中较小者相对内角格的比值（×16，封顶 255）；调用方保证 corner > 0。
uint8_t SigRatio(int16_t corner, int16_t a, int16_t b) {
    return static_cast<uint8_t>(std::min((std::min(a, b) << 4) / corner, 0xff));
}

}  // namespace

// TouchZone_InnerCornerHandler @ 0x6ba88419
// (dim1, dim2) 是离屏幕某个角各一格的内角格时，取它同列的边缘格与同行的边缘格；三者都为正时
// 记下边缘格相对内角格的信号比。列号为 1 时算左角（否则右角），行号为 1 时算上角（否则下角），
// 与原代码四个分支的先后顺序一致。
void Process(TouchZoneUnit& zone, uint8_t dim1, uint8_t dim2) {
    const FlashPrmt& fl = PrmtFlash();
    const int cols = fl.dim1Len;
    const int rows = fl.dim2Len;
    const int16_t* dif = Buffers().dif;
    const int16_t corner = dif[GridIndex(dim1, dim2)];
    if ((dim1 != 1 && dim1 != cols - 2) || (dim2 != 1 && dim2 != rows - 2)) return;
    const int edgeCol = dim1 == 1 ? 0 : cols - 1;
    const int edgeRow = dim2 == 1 ? 0 : rows - 1;
    const int16_t sameCol = dif[static_cast<uint16_t>(dim1 + edgeRow * cols)];
    const int16_t sameRow = dif[static_cast<uint16_t>(edgeCol + dim2 * cols)];
    if (sameCol > 0 && sameRow > 0 && corner > 0) zone.innerCornerSigRatio = SigRatio(corner, sameCol, sameRow);
}

}  // namespace Solvers::Tsa::Core::InnerCorner

// raw TZ 还在调用的入口
extern "C" void TouchZone_InnerCornerHandler(void* zone, uint8_t dim1, uint8_t dim2) {
    Solvers::Tsa::Core::InnerCorner::Process(*static_cast<Solvers::Tsa::Core::TouchZoneUnit*>(zone), dim1, dim2);
}
