// 基线的 IIR 跟踪与 Dif 计算（原 Baseline/BLIIR.c，原厂 src/Alg/Baseline/BaselineIIR.c）。

#include "Alg/Baseline/Baseline.h"
#include "Alg/Baseline/BaselineInternal.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Baseline {

namespace {

constexpr size_t kBlBytes = 0x2648;  // 基线缓冲的容量（TSA_MAX_DIM²）

size_t GridBytes() { return PrmtFlash().sensorNum * size_t{2}; }

}  // namespace

// BLIIR_CalcDiffCommon @ 0x6ba46f4c
void CalcDiffCommon(int16_t* dif, const int16_t* raw, const int16_t* bl) {
    const uint16_t n = PrmtFlash().sensorNum;
    if (Static().signalReverted == 0) {
        for (uint16_t i = 0; i < n; ++i) dif[i] = static_cast<int16_t>(bl[i] - raw[i]);
    } else {
        for (uint16_t i = 0; i < n; ++i) dif[i] = static_cast<int16_t>(raw[i] - bl[i]);
    }
}

// BLIIR_CalcRawCommon @ 0x6ba4701b
void CalcRawCommon(int16_t* raw, const int16_t* bl, const int16_t* dif) {
    const uint16_t n = PrmtFlash().sensorNum;
    if (Static().signalReverted == 0) {
        for (uint16_t i = 0; i < n; ++i) raw[i] = static_cast<int16_t>(bl[i] - dif[i]);
    } else {
        for (uint16_t i = 0; i < n; ++i) raw[i] = static_cast<int16_t>(dif[i] + bl[i]);
    }
}

namespace Iir {

namespace {

// BLIIR_DoUpdate @ 0x6ba47141
// 每格 raw 与基线之差（按无符号值相减）达到 ±step 时，基线向 raw 走一步。
// 写成无条件读写加选择（不动的格写回原值），循环才能向量化。
void DoUpdate(int16_t step) {
    const TsaBufferTable& buf = Buffers();
    const auto* src = reinterpret_cast<const uint16_t*>(PrmtFlash().blUpdateFromPreCmfRaw == 0 ? buf.raw : buf.preCmfRaw);
    auto* bl = reinterpret_cast<uint16_t*>(buf.bl);
    const int n = PrmtFlash().sensorNum;
    for (int i = 0; i < n; ++i) {
        const uint16_t b = bl[i];
        const int d = src[i] - b;
        bl[i] = d >= step ? static_cast<uint16_t>(b + step) : d <= -step ? static_cast<uint16_t>(b - step) : b;
    }
    Static().blIirUpdated = 1;
}

}  // namespace

// BLIIR_CalcDiff @ 0x6ba46efc
void CalcDiff() {
    const TsaBufferTable& buf = Buffers();
    CalcDiffCommon(buf.dif, buf.raw, buf.bl);
}

// BLIIR_ResetDiff @ 0x6ba46e76
void ResetDiff() { ClearChecked(Buffers().dif, GridBytes(), GridBytes()); }

// BLIIR_Update @ 0x6ba470e7
// 正向极值不超过 thold、且负向极值不低于 -thold（force 时不看负向）才步进。
void Update(bool force, int16_t thold, int16_t step) {
    const int16_t maxSig = MaxSig();
    const int16_t minSig = MinSig();
    if ((force || -thold <= minSig) && maxSig <= thold) DoUpdate(step);
}

// BLIIR_Reset @ 0x6ba4721f
// 基线取 CMF 前的 raw（原版在 raw 已归一到目标值的 ALPS 机型上整屏取目标值）；再同步到接近检测的基线
// （接近检测被裁掉，判定恒为未检出）。
void Reset() {
    const TsaBufferTable& buf = Buffers();
    CopyChecked(buf.bl, kBlBytes, buf.preCmfRaw, GridBytes());
    if (ProxBl() != nullptr) CopyChecked(ProxBl(), kBlBytes, buf.bl, GridBytes());
}

}  // namespace Iir

}  // namespace Solvers::Tsa::Core::Baseline
