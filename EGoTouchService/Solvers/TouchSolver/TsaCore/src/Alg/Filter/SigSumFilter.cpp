// 按信号量保留触点（原 Signal/SigSumFilter.c，原厂 src/Alg/Filter/SigSumFilter/SigSumFilter_Process.c）。

#include "Alg/Filter/SigSumFilter.h"

#include <cstring>

#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"

namespace Solvers::Tsa::Core::SigSumFilter {

// SigSumFilter_ReserveTouch @ 0x6ba60977
// 触点按 field098 从大到小排序（选择交换，整单元按字节交换），再去掉第 keep 个之后的触点。keep 为 0 时不处理。
void ReserveTouch(int keep) {
    if (keep == 0) return;
    TouchTable& tt = Touches();
    for (int i = 0; i < tt.count - 1; ++i) {
        for (int j = i + 1; j < tt.count; ++j) {
            if (tt.unit[i].field098 < tt.unit[j].field098) {
                TouchUnit tmp;
                std::memcpy(&tmp, &tt.unit[i], sizeof(TouchUnit));
                std::memcpy(&tt.unit[i], &tt.unit[j], sizeof(TouchUnit));
                std::memcpy(&tt.unit[j], &tmp, sizeof(TouchUnit));
            }
        }
    }
    for (int k = tt.count - 1; k >= 0; --k) {
        if (keep <= k) Touch::UnitRemove(static_cast<uint8_t>(k));
    }
}

}  // namespace Solvers::Tsa::Core::SigSumFilter

// raw 的 TSA 还在调用的入口
extern "C" void SigSumFilter_ReserveTouch(int keep) { Solvers::Tsa::Core::SigSumFilter::ReserveTouch(keep); }
