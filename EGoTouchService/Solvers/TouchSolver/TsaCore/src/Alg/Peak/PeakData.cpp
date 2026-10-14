#include "Alg/Peak/Peak.h"

#include <climits>

#include "Alg/Touch/TouchData.h"
#include "App/TsaBuffer.h"
#include "Common/Assert.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Peak {

// Peak_Clean @ 0x6ba659b0
void Clean() {
    PeakTable& peaks = Peaks();
    ZeroFill(peaks);
    peaks.maxSig = INT16_MIN;
    peaks.minSig = INT16_MAX;
    peaks.maxZn = INT32_MIN;
}

// Peak_Insert @ 0x6ba65e68
uint32_t Insert(uint8_t dim1, uint8_t dim2) {
    PeakTable& peaks = Peaks();
    if (peaks.count < kMaxPeakNum) {
        Fill(peaks.count, dim1, dim2);
        if (!IsOnOuterEdge(peaks.count)) ++peaks.innerCount;
        ++peaks.count;
        return peaks.count - 1u;
    }
    // 峰表已满（原代码在此打日志，本版本已裁掉）
    uint32_t weakest = 0;
    for (uint32_t i = 1; i < kMaxPeakNum; ++i) {
        if (peaks.entry[i].sig < peaks.entry[weakest].sig) weakest = i;
    }
    if (peaks.entry[weakest].sig < Buffers().dif[GridIndex(dim1, dim2)]) {
        Fill(static_cast<uint8_t>(weakest), dim1, dim2);
        return weakest;
    }
    return kMaxPeakNum;
}

// Peak_Fill @ 0x6ba65cb8
void Fill(uint8_t idx, uint8_t dim1, uint8_t dim2) {
    PeakTable& peaks = Peaks();
    PeakEntry& e = peaks.entry[idx];
    const int16_t sig = Buffers().dif[GridIndex(dim1, dim2)];
    e.sig = sig;
    e.dim1 = dim1;
    e.dim2 = dim2;
    CalcZn(idx);
    if (peaks.maxSig < sig) peaks.maxSig = sig;
    if (sig < peaks.minSig) peaks.minSig = sig;
    if (peaks.maxZn < e.zn) peaks.maxZn = e.zn;
}

// Peak_Remove @ 0x6ba65b60
void Remove(uint8_t idx) {
    PeakTable& peaks = Peaks();
    TsaAssert(peaks.count <= kMaxPeakNum);
    if (peaks.count > kMaxPeakNum) peaks.count = kMaxPeakNum;
    if (idx >= peaks.count) return;
    for (int i = idx; i < peaks.count - 1; ++i) peaks.entry[i] = peaks.entry[i + 1];
    --peaks.count;
}

// Peak_IsTouchValid @ 0x6ba65a3e
// 注意：参数是触点下标。
bool IsTouchValid(uint8_t touchIdx) {
    return Touches().unit[touchIdx].event < 5;
}

// Peak_IsOnInnerEdgeDim1Near @ 0x6ba66764
bool IsOnInnerEdgeDim1Near(uint8_t idx) {
    return Peaks().entry[idx].dim1 == 1;
}

// Peak_IsOnInnerEdgeDim1Far @ 0x6ba6679b
bool IsOnInnerEdgeDim1Far(uint8_t idx) {
    return Peaks().entry[idx].dim1 == PrmtFlash().dim1Len - 2;
}

// Peak_IsOnInnerEdgeDim2Near @ 0x6ba667e6
bool IsOnInnerEdgeDim2Near(uint8_t idx) {
    return Peaks().entry[idx].dim2 == 1;
}

// Peak_IsOnInnerEdgeDim2Far @ 0x6ba6681d
bool IsOnInnerEdgeDim2Far(uint8_t idx) {
    return Peaks().entry[idx].dim2 == PrmtFlash().dim2Len - 2;
}

// Peak_IsOnInnerEdgeDim1 @ 0x6ba66868
bool IsOnInnerEdgeDim1(uint8_t idx) {
    return IsOnInnerEdgeDim1Near(idx) || IsOnInnerEdgeDim1Far(idx);
}

// Peak_IsOnOuterEdgeDim1Near @ 0x6ba6691f
bool IsOnOuterEdgeDim1Near(uint8_t idx) {
    return Peaks().entry[idx].dim1 == 0;
}

// Peak_IsOnOuterEdgeDim1Far @ 0x6ba66956
bool IsOnOuterEdgeDim1Far(uint8_t idx) {
    return Peaks().entry[idx].dim1 == PrmtFlash().dim1Len - 1;
}

// Peak_IsOnOuterEdgeDim2Near @ 0x6ba669a1
bool IsOnOuterEdgeDim2Near(uint8_t idx) {
    return Peaks().entry[idx].dim2 == 0;
}

// Peak_IsOnOuterEdgeDim2Far @ 0x6ba669d8
bool IsOnOuterEdgeDim2Far(uint8_t idx) {
    return Peaks().entry[idx].dim2 == PrmtFlash().dim2Len - 1;
}

// Peak_IsOnOuterEdgeDim1 @ 0x6ba66a23
bool IsOnOuterEdgeDim1(uint8_t idx) {
    return IsOnOuterEdgeDim1Near(idx) || IsOnOuterEdgeDim1Far(idx);
}

// Peak_IsOnOuterEdgeDim2 @ 0x6ba66a60
bool IsOnOuterEdgeDim2(uint8_t idx) {
    return IsOnOuterEdgeDim2Near(idx) || IsOnOuterEdgeDim2Far(idx);
}

// Peak_IsOnOuterEdge @ 0x6ba66a9d
bool IsOnOuterEdge(uint8_t idx) {
    return IsOnOuterEdgeDim1(idx) || IsOnOuterEdgeDim2(idx);
}

// Peak_IsInNxNBox @ 0x6ba67a7e
bool IsInNxNBox(uint8_t dim1, uint8_t dim2, uint8_t boxSize) {
    const PeakTable& peaks = Peaks();
    for (uint8_t i = 0; i < peaks.count; ++i) {
        if (static_cast<uint8_t>(Sensor_IsInNxNBox(peaks.entry[i].dim1, peaks.entry[i].dim2, dim1, dim2, boxSize)) != 0) {
            return true;
        }
    }
    return false;
}

// Peak_GetIdxBySig @ 0x6ba666f3
uint8_t GetIdxBySig(int16_t sig) {
    const PeakTable& peaks = Peaks();
    for (uint8_t i = 0; i < peaks.count; ++i) {
        if (peaks.entry[i].sig == sig) return i;
    }
    return kMaxPeakNum;
}

// Peak_GetZ9 @ 0x6ba65a72
int GetZ9(uint8_t idx) {
    const PeakEntry& e = Peaks().entry[idx];
    return e.z8 + e.sig;
}

// Peak_GetDim1DistToEdge @ 0x6ba67b4d
uint8_t GetDim1DistToEdge(uint8_t idx) {
    const uint8_t dim1 = Peaks().entry[idx].dim1;
    const uint8_t len = PrmtFlash().dim1Len;
    return len - 1 - dim1 < dim1 ? static_cast<uint8_t>(len - dim1 - 1) : dim1;
}

// Peak_GetDim2DistToEdge @ 0x6ba67bca
uint8_t GetDim2DistToEdge(uint8_t idx) {
    const uint8_t dim2 = Peaks().entry[idx].dim2;
    const uint8_t len = PrmtFlash().dim2Len;
    return len - 1 - dim2 < dim2 ? static_cast<uint8_t>(len - dim2 - 1) : dim2;
}

// Peak_GetDistToEdge @ 0x6ba67c47
uint8_t GetDistToEdge(uint8_t idx) {
    const uint8_t d1 = GetDim1DistToEdge(idx);
    const uint8_t d2 = GetDim2DistToEdge(idx);
    return d1 <= d2 ? d1 : d2;
}

// Peak_GetDistance @ 0x6ba66bbd
uint8_t GetDistance(uint8_t idxA, uint8_t idxB) {
    const PeakEntry& a = Peaks().entry[idxA];
    const PeakEntry& b = Peaks().entry[idxB];
    return static_cast<uint8_t>(Sensor_GetDistance(a.dim1, a.dim2, b.dim1, b.dim2));
}

// Peak_GetSquareDistance @ 0x6ba66c86
int GetSquareDistance(uint8_t idxA, uint8_t idxB) {
    const PeakEntry& a = Peaks().entry[idxA];
    const PeakEntry& b = Peaks().entry[idxB];
    return Sensor_GetSquareDistance(a.dim1, a.dim2, b.dim1, b.dim2);
}

}  // namespace Solvers::Tsa::Core::Peak

// ══════════════════════════════════════════════════════════════════════
// 过渡期：未改写的 C 代码按原名调用。签名与 raw/include/TSAFuncDecls.h 一致。
// 返回 undefined4 的边缘判定在原代码里是 CONCAT31：高 24 位带着比较对象的高位，这里照样拼出来。
// ══════════════════════════════════════════════════════════════════════
namespace P = Solvers::Tsa::Core::Peak;
using Solvers::Tsa::Core::PrmtFlash;
using Solvers::Tsa::Core::Touches;

namespace {
uint32_t Concat31(uint32_t high, bool low) {
    return (high & 0xffffff00u) | static_cast<uint32_t>(low);
}
}  // namespace

extern "C" {
void Peak_Process(void) { P::Process(); }
void Peak_Clean(void) { P::Clean(); }
void Peak_DetectInRange(int16_t thold, uint8_t firstRow, uint8_t lastRow, uint8_t firstCol, uint8_t lastCol) {
    P::DetectInRange(thold, firstRow, lastRow, firstCol, lastCol);
}
void Peak_SortInSigAscending(void) { P::SortInSigAscending(); }
uint64_t Peak_IsToExcludeSideRegion(void) { return P::IsToExcludeSideRegion(); }
uint64_t Peak_needUDFPLargeFingerWorkaround(void) { return P::NeedUdfpLargeFingerWorkaround(); }
uint32_t Peak_Insert(uint8_t dim1, uint8_t dim2) { return P::Insert(dim1, dim2); }
uint32_t Peak_IsTouchValid(uint8_t touchIdx) {
    return Concat31(Touches().unit[touchIdx].event, P::IsTouchValid(touchIdx));
}
uint64_t Peak_IsOnInnerEdgeDim1(uint8_t idx) { return P::IsOnInnerEdgeDim1(idx); }
uint64_t Peak_IsOnOuterEdge(uint8_t idx) { return P::IsOnOuterEdge(idx); }
uint64_t Peak_IsOnOuterEdgeDim1(uint8_t idx) { return P::IsOnOuterEdgeDim1(idx); }
uint8_t Peak_IsInNxNBox(uint8_t dim1, uint8_t dim2, uint8_t boxSize) { return P::IsInNxNBox(dim1, dim2, boxSize); }
uint8_t Peak_GetIdxBySig(int16_t sig) { return P::GetIdxBySig(sig); }
int Peak_GetZ9(uint8_t idx) { return P::GetZ9(idx); }
uint8_t Peak_GetDistToEdge(uint8_t idx) { return P::GetDistToEdge(idx); }
uint8_t Peak_GetDistance(uint8_t idxA, uint8_t idxB) { return P::GetDistance(idxA, idxB); }
int Peak_GetSquareDistance(uint8_t idxA, uint8_t idxB) { return P::GetSquareDistance(idxA, idxB); }
}  // extern "C"
