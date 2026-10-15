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
constexpr int kMaxDimLen = 256;    // dim1Len / dim2Len 是 uint8_t

// 一格对各累计量的贡献。都写成选择表达式而不是分支：Dif 的符号与强弱逐格随机，分支几乎每格都预测失败；
// 选择表达式让编译器生成无分支、可向量化的代码。整数加法与次序无关，结果与逐格分支累加相同。
int NonNeg(int v) { return v >= 0 ? v : 0; }
int Neg(int v) { return v < 0 ? v : 0; }
int StrongPos(int v) { return v >= kStrongPos ? v : 0; }
int StrongNeg(int v) { return v < kStrongNeg ? v : 0; }
int Abs(int v) { return v < 0 ? -v : v; }

/// 一条线的统计（Self_ProcessDimUnit 的累计量）。inner 只计线的中间格（不含两端）。
struct LineStat {
    int pos = 0;
    int neg = 0;
    int strongPos = 0;
    int strongNeg = 0;
    int innerStrongPos = 0;
    int innerStrongNeg = 0;
    int maxAbs = 0;
};

void Store(uint32_t line, const LineStat& a) {
    SelfData& s = SelfBuf();
    s.sum[line] = a.pos + a.neg;
    s.strongSum[line] = a.strongPos + a.strongNeg;
    s.innerStrongSum[line] = a.innerStrongPos + a.innerStrongNeg;
    s.strongAbsSum[line] = a.strongPos - a.strongNeg;
    s.innerStrongAbsSum[line] = a.innerStrongPos - a.innerStrongNeg;
    s.posSum[line] = a.pos;
    s.negSum[line] = a.neg;
    s.maxAbs[line] = static_cast<int16_t>(a.maxAbs);
}

// Self_ProcessDimUnit @ 0x6ba6a984（行方向：连续的 len 格）
LineStat RowStat(const int16_t* p, int len) {
    LineStat a;
    for (int k = 0; k < len; ++k) {
        const int v = p[k];
        a.pos += NonNeg(v);
        a.neg += Neg(v);
        a.strongPos += StrongPos(v);
        a.strongNeg += StrongNeg(v);
        a.maxAbs = std::max(a.maxAbs, Abs(v));
    }
    for (int k = 1; k < len - 1; ++k) {
        const int v = p[k];
        a.innerStrongPos += StrongPos(v);
        a.innerStrongNeg += StrongNeg(v);
    }
    return a;
}

/// 各列的累计量：原代码逐列按步长 dim1Len 取格，这里逐行扫、同时累计全部列，访存连续且可向量化。
struct ColumnStats {
    int pos[kMaxDimLen] = {};
    int neg[kMaxDimLen] = {};
    int strongPos[kMaxDimLen] = {};
    int strongNeg[kMaxDimLen] = {};
    int innerStrongPos[kMaxDimLen] = {};
    int innerStrongNeg[kMaxDimLen] = {};
    int maxAbs[kMaxDimLen] = {};

    void AddRow(const int16_t* row, int cols, bool inner) {
        for (int c = 0; c < cols; ++c) {
            const int v = row[c];
            pos[c] += NonNeg(v);
            neg[c] += Neg(v);
            strongPos[c] += StrongPos(v);
            strongNeg[c] += StrongNeg(v);
            maxAbs[c] = std::max(maxAbs[c], Abs(v));
        }
        if (!inner) return;
        for (int c = 0; c < cols; ++c) {
            const int v = row[c];
            innerStrongPos[c] += StrongPos(v);
            innerStrongNeg[c] += StrongNeg(v);
        }
    }
    [[nodiscard]] LineStat At(int c) const {
        return {pos[c], neg[c], strongPos[c], strongNeg[c], innerStrongPos[c], innerStrongNeg[c], maxAbs[c]};
    }
};

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
    const int16_t* dif = Buffers().dif;
    const int cols = fl.dim1Len;
    const int rows = fl.dim2Len;
    ColumnStats colStats;
    for (int r = 0; r < rows; ++r) {
        const int16_t* row = dif + static_cast<uint16_t>(r * cols);
        Store(static_cast<uint32_t>(r + cols), RowStat(row, cols));
        colStats.AddRow(row, cols, r != 0 && r != rows - 1);
    }
    for (int c = 0; c < cols; ++c) Store(static_cast<uint32_t>(c), colStats.At(c));
    GetProperty(0, fl.dim1Len, Prpt().selfCols);
    GetProperty(fl.dim1Len, fl.dim2Len, Prpt().selfRows);
}

// Self_Reset @ 0x6ba6a920
void Reset() { ZeroFill(SelfBuf()); }

}  // namespace Solvers::Tsa::Core::Self
