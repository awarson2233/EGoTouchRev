#include "Alg/SelfData/SelfData.h"

#include <algorithm>

#include "App/TsaBuffer.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Self {

namespace {

constexpr int kStrongPos = 0x4b;   // 不小于它为强正信号
constexpr int kStrongNeg = -0x4a;  // 小于它为强负信号
constexpr int kMaxDimLen = 256;    // dim1Len / dim2Len 是 uint8_t

// 一格对各累计量的贡献。都写成选择表达式而不是分支：Dif 的符号与强弱逐格随机，分支几乎每格都预测失败；
// 选择表达式让编译器生成无分支、可向量化的代码。整数加法与次序无关，结果与逐格分支累加相同。
int StrongPos(int v) { return v >= kStrongPos ? v : 0; }
int StrongNeg(int v) { return v < kStrongNeg ? v : 0; }
int Abs(int v) { return v < 0 ? -v : v; }

/// 各列的累计量：原代码逐列按步长 dim1Len 取格，这里逐行扫、同时累计全部列，访存连续且可向量化。
struct ColumnStats {
    int sum[kMaxDimLen] = {};
    int strongPos[kMaxDimLen] = {};
    int strongNeg[kMaxDimLen] = {};
    int innerStrongPos[kMaxDimLen] = {};
    int innerStrongNeg[kMaxDimLen] = {};
    int maxAbs[kMaxDimLen] = {};

    void AddRow(const int16_t* row, int cols, bool inner) {
        for (int c = 0; c < cols; ++c) {
            const int v = row[c];
            sum[c] += v;
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
};

}  // namespace

// Self_Process @ 0x6ba6aea8（Self_ProcessDimUnit @ 0x6ba6a984 逐线统计，Self_GetPropertyDim @ 0x6ba6ac88 求极值）
// 原代码对每条线（行与列）统计和、正负和、强信号与中间格强信号的和与绝对值和、|Dif| 最大值，再求两个方向
// 的极值写进 TsaPrpt。这里只算有模块读的：行的和与中间格强信号；列的和、强信号、中间格强信号与 |Dif| 最大值。
// 极值（selfCols / selfRows）、正负和、行的强信号与最大值没有模块读，不算。
void Process() {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    const int cols = fl.dim1Len;
    const int rows = fl.dim2Len;
    SelfData& s = SelfBuf();
    ColumnStats colStats;
    for (int r = 0; r < rows; ++r) {
        const int16_t* row = dif + static_cast<uint16_t>(r * cols);
        const bool inner = r != 0 && r != rows - 1;
        int sum = 0;
        for (int c = 0; c < cols; ++c) sum += row[c];
        int innerPos = 0;
        int innerNeg = 0;
        for (int c = 1; c < cols - 1; ++c) {
            innerPos += StrongPos(row[c]);
            innerNeg += StrongNeg(row[c]);
        }
        const auto line = static_cast<uint32_t>(r + cols);
        s.sum[line] = sum;
        s.innerStrongSum[line] = innerPos + innerNeg;
        s.innerStrongAbsSum[line] = innerPos - innerNeg;
        colStats.AddRow(row, cols, inner);
    }
    for (int c = 0; c < cols; ++c) {
        s.sum[c] = colStats.sum[c];
        s.strongSum[c] = colStats.strongPos[c] + colStats.strongNeg[c];
        s.innerStrongSum[c] = colStats.innerStrongPos[c] + colStats.innerStrongNeg[c];
        s.strongAbsSum[c] = colStats.strongPos[c] - colStats.strongNeg[c];
        s.maxAbs[c] = static_cast<int16_t>(colStats.maxAbs[c]);
    }
}

// Self_Reset @ 0x6ba6a920
void Reset() { ZeroFill(SelfBuf()); }

}  // namespace Solvers::Tsa::Core::Self
