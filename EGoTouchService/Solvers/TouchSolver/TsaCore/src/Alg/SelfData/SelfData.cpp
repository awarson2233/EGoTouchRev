#include "Alg/SelfData/SelfData.h"

#include <algorithm>
#include <climits>

#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Self {

namespace {

constexpr int kStrongPos = 0x4b;   // 不小于它为强正信号
constexpr int kStrongNeg = -0x4a;  // 小于它为强负信号

// Self_ProcessDimUnit @ 0x6ba6a984
// 从 start 起按 stride 取 len 格 Dif，统计后写到第 line 条线。
void ProcessLine(uint16_t start, uint32_t stride, int len, uint32_t line) {
    const int16_t* p = Buffers().dif + start;
    int pos = 0;
    int neg = 0;
    int strongPos = 0;
    int strongNeg = 0;
    int innerStrongPos = 0;
    int innerStrongNeg = 0;
    int maxAbs = 0;
    int16_t innerStrongCount = 0;
    for (int k = 0; k < len; ++k, p += stride) {
        const int v = *p;
        const bool inner = k != 0 && k != len - 1;
        maxAbs = std::max({maxAbs, v, -v});
        if (v >= kStrongPos) {
            pos += v;
            if (inner) {
                ++innerStrongCount;
                innerStrongPos += v;
            }
            strongPos += v;
        } else if (v >= 0) {
            pos += v;
        } else if (v < kStrongNeg) {
            neg += v;
            if (inner) innerStrongNeg += v;
            strongNeg += v;
        } else {
            neg += v;
        }
    }
    SelfData& s = SelfBuf();
    s.sum[line] = pos + neg;
    s.innerStrongCount[line] = innerStrongCount;
    s.strongSum[line] = strongPos + strongNeg;
    s.innerStrongSum[line] = innerStrongPos + innerStrongNeg;
    s.strongAbsSum[line] = strongPos - strongNeg;
    s.innerStrongAbsSum[line] = innerStrongPos - innerStrongNeg;
    s.posSum[line] = pos;
    s.negSum[line] = neg;
    s.maxAbs[line] = static_cast<int16_t>(maxAbs);
}

// Self_GetPropertyDim @ 0x6ba6ac88
// 从第 first 条线起 count 条线的极值（最大与最小在同一条线时只记最大）与强信号总和。
void GetProperty(uint8_t first, uint8_t count, SelfPrpt& p) {
    const SelfData& s = SelfBuf();
    p.maxSum = INT32_MIN;
    p.minSum = INT32_MAX;
    p.maxPosSum = INT32_MIN;
    p.minNegSum = INT32_MAX;
    p.maxSumIdx = 0;
    p.minSumIdx = 0;
    p.minNegSumIdx = 0;
    p.maxPosSumIdx = 0;
    p.strongSumTotal = 0;
    for (uint8_t i = first; i < count + static_cast<uint32_t>(first); ++i) {
        const auto rel = static_cast<uint8_t>(i - first);
        if (p.maxSum < s.sum[i]) {
            p.maxSum = s.sum[i];
            p.maxSumIdx = rel;
        } else if (s.sum[i] < p.minSum) {
            p.minSum = s.sum[i];
            p.minSumIdx = rel;
        }
        if (p.maxPosSum < s.posSum[i]) {
            p.maxPosSum = s.posSum[i];
            p.maxPosSumIdx = rel;
        }
        if (s.negSum[i] < p.minNegSum) {
            p.minNegSum = s.negSum[i];
            p.minNegSumIdx = rel;
        }
        p.strongSumTotal += s.strongSum[i];
    }
}

}  // namespace

// Self_Process @ 0x6ba6aea8
// 先逐行（沿 Dim1，写到 dim1Len 之后），再逐列（沿 Dim2），最后统计两个方向的极值。
void Process() {
    const FlashPrmt& fl = PrmtFlash();
    for (uint8_t r = 0; r < fl.dim2Len; ++r) {
        ProcessLine(static_cast<uint16_t>(r * fl.dim1Len), 1, fl.dim1Len, static_cast<uint32_t>(r) + fl.dim1Len);
    }
    for (uint8_t c = 0; c < fl.dim1Len; ++c) ProcessLine(c, fl.dim1Len, fl.dim2Len, c);
    GetProperty(0, fl.dim1Len, Prpt().selfCols);
    GetProperty(fl.dim1Len, fl.dim2Len, Prpt().selfRows);
}

// Self_Reset @ 0x6ba6a920
void Reset() { ZeroFill(SelfBuf()); }

}  // namespace Solvers::Tsa::Core::Self
