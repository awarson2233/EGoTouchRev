#include "Alg/Peak/PrevPeak.h"

#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::PrevPeak {

// PrevPeak_Init @ 0x6ba68f10
void Init() {
    ZeroFill(PrevPeaks());
}

// PrevPeak_Process @ 0x6ba68f77
void Process() {
    const PeakTable& peaks = Peaks();
    PrevPeakTable& prev = PrevPeaks();
    prev.count = peaks.count;
    prev.maxSig = peaks.maxSig;
    for (uint8_t i = 0; i < peaks.count; ++i) {
        prev.entry[i].flags = peaks.entry[i].flags;
        prev.entry[i].zn = peaks.entry[i].zn;
        prev.entry[i].sig = peaks.entry[i].sig;
        prev.entry[i].dim1 = peaks.entry[i].dim1;
        prev.entry[i].dim2 = peaks.entry[i].dim2;
    }
}

// PrevPeak_IsOnOuterEdgeDim1Near @ 0x6ba69156
bool IsOnOuterEdgeDim1Near(uint8_t idx) {
    return PrevPeaks().entry[idx].dim1 == 0;
}

// PrevPeak_IsOnOuterEdgeDim1Far @ 0x6ba69188
bool IsOnOuterEdgeDim1Far(uint8_t idx) {
    return PrevPeaks().entry[idx].dim1 == PrmtFlash().dim1Len - 1;
}

// PrevPeak_IsOnOuterEdgeDim2Near @ 0x6ba691ce
bool IsOnOuterEdgeDim2Near(uint8_t idx) {
    return PrevPeaks().entry[idx].dim2 == 0;
}

// PrevPeak_IsOnOuterEdgeDim2Far @ 0x6ba69200
bool IsOnOuterEdgeDim2Far(uint8_t idx) {
    return PrevPeaks().entry[idx].dim2 == PrmtFlash().dim2Len - 1;
}

// PrevPeak_IsOnOuterEdgeDim1 @ 0x6ba69246
bool IsOnOuterEdgeDim1(uint8_t idx) {
    return IsOnOuterEdgeDim1Near(idx) || IsOnOuterEdgeDim1Far(idx);
}

// PrevPeak_IsOnOuterEdgeDim2 @ 0x6ba69283
bool IsOnOuterEdgeDim2(uint8_t idx) {
    return IsOnOuterEdgeDim2Near(idx) || IsOnOuterEdgeDim2Far(idx);
}

// PrevPeak_IsOnOuterEdge @ 0x6ba692c0
bool IsOnOuterEdge(uint8_t idx) {
    return IsOnOuterEdgeDim1(idx) || IsOnOuterEdgeDim2(idx);
}

}  // namespace Solvers::Tsa::Core::PrevPeak

// ══════════════════════════════════════════════════════════════════════
// 过渡期：未改写的 C 代码按原名调用。签名与 raw/include/TSAFuncDecls.h 一致。
// ══════════════════════════════════════════════════════════════════════
namespace PP = Solvers::Tsa::Core::PrevPeak;

extern "C" {
void PrevPeak_Process(void) { PP::Process(); }
}  // extern "C"
