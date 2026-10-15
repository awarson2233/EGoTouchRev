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

/// 一格的强信号（不小于 kStrongPos 或小于 kStrongNeg 的 Dif，否则 0）。
int Strong(int v) { return StrongPos(v) + StrongNeg(v); }

/// 各列的累计量：原代码逐列按步长 dim1Len 取格，这里逐行扫、同时累计全部列，访存连续且可向量化。
/// 中间格（去掉首末两行）的强信号由全列的减去首末两行的得到，少两组逐行累计。
struct ColumnStats {
    int sum[kMaxDimLen] = {};
    int strong[kMaxDimLen] = {};     // 强信号之和
    int strongAbs[kMaxDimLen] = {};  // 强信号绝对值之和
    int maxAbs[kMaxDimLen] = {};

    void AddRow(const int16_t* row, int cols) {
        for (int c = 0; c < cols; ++c) {
            const int v = row[c];
            sum[c] += v;
            strong[c] += Strong(v);
            strongAbs[c] += Abs(Strong(v));
            maxAbs[c] = std::max(maxAbs[c], Abs(v));
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
        colStats.AddRow(row, cols);
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
    }
    const int16_t* first = dif;
    const int16_t* last = dif + static_cast<uint16_t>((rows - 1) * cols);
    for (int c = 0; c < cols; ++c) {
        // 只有一行时首末是同一行，中间格为空
        const int edge = rows == 1 ? Strong(first[c]) : Strong(first[c]) + Strong(last[c]);
        s.sum[c] = colStats.sum[c];
        s.strongSum[c] = colStats.strong[c];
        s.innerStrongSum[c] = colStats.strong[c] - edge;
        s.strongAbsSum[c] = colStats.strongAbs[c];
        s.maxAbs[c] = static_cast<int16_t>(colStats.maxAbs[c]);
    }
}

// Self_Reset @ 0x6ba6a920
void Reset() { ZeroFill(SelfBuf()); }

}  // namespace Solvers::Tsa::Core::Self
