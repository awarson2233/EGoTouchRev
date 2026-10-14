#pragma once
// 内角格的信号比（原 Detection/TouchZone.c）：峰落在离屏幕角各一格的位置时，TZ 记下边缘格相对它的信号比。

#include <cstdint>

namespace Solvers::Tsa::Core {
struct TouchZoneUnit;
}

namespace Solvers::Tsa::Core::InnerCorner {

/// (dim1, dim2) 是内角格且它与同列、同行的边缘格信号都为正时，写 zone.innerCornerSigRatio。
void Process(TouchZoneUnit& zone, uint8_t dim1, uint8_t dim2);

}  // namespace Solvers::Tsa::Core::InnerCorner
