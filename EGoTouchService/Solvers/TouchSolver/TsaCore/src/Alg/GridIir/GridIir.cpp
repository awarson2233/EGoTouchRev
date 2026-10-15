// 网格 IIR（原 Signal/GridIIR.c，原厂 src/Alg/GridIIR/GridIIR.c）：所有频点都有噪声时，对 Dif 逐格做时间上的 IIR。

#include "Alg/GridIir/GridIir.h"

#include "Alg/Filter/IirFilter.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "Common/Assert.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::GridIir {

// GridIIR_Process @ 0x6ba61098（GridIIR_ProcessCore @ 0x6ba60fd5 内联）
// 刚进入全频点噪声时以当前 Dif 为起点；此后 Dif 换成滤波值，并记作下一帧的起点。
void Process() {
    const TsaBufferTable& buf = Buffers();
    const uint16_t n = PrmtFlash().sensorNum;
    if (Sys::IsAllFreqBecomeNoisy() != 0) {
        CopyChecked(buf.gridIirDif, 0x2648, buf.dif, n * size_t{2});
    } else if (Sys::IsAllFreqNoisy() != 0) {
        const uint8_t weight = PrmtFlash().gridIirWeight;
        for (uint16_t i = 0; i < n; ++i) {
            buf.dif[i] = static_cast<int16_t>(Iir::ProcessConverging(buf.dif[i], buf.gridIirDif[i], weight, 8));
            buf.gridIirDif[i] = buf.dif[i];
        }
    }
}

}  // namespace Solvers::Tsa::Core::GridIir

// raw 的 TSA 还在调用的入口
extern "C" void GridIIR_Process(void) { Solvers::Tsa::Core::GridIir::Process(); }
