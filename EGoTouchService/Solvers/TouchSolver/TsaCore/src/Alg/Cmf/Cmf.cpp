#include "Alg/Cmf/Cmf.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Alg/Baseline/Baseline.h"
#include "Alg/SelfData/SelfData.h"
#include "Alg/TouchThold/TouchThold.h"
#include "App/Scenario.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Assert.h"
#include "Common/Bss.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Cmf {

namespace {

constexpr int kDimRow = 1;                       // 按行（沿 Dim1）
constexpr int kDimCol = 2;                       // 按列（沿 Dim2）
constexpr uint32_t kMethod2d = 1;
constexpr uint32_t kSmartCoverFollowNormal = 2;  // cmfMethodSmartCover 为 2 时沿用常态的方法
constexpr int16_t kToeEdgeThold = 200;           // ToE 特殊门限时两端各两条线的门限
constexpr int16_t kGameThold = 0x4b0;
constexpr int16_t kGameGrad = 200;
constexpr int kMaxLineLen = 0x102;
constexpr size_t kMaxGridCells = 0x1324;  // TSA_MAX_DIM²

/// Dif 相近的一段连续平坦格。
struct NoiseCluster {
    int32_t sum;
    uint8_t count;
    uint8_t reserved005[3];
    int32_t last;       // 最近加入的格的 Dif
    int32_t weightSum;  // 权重之和（权重恒为 1）
};
static_assert(sizeof(NoiseCluster) == 0x10);

/// g_cmfNoise：一条线的聚类状态与结果。
struct NoiseState {
    uint16_t lastIdx;  // 上一个加入的格（0xffff 为无）
    uint8_t cur;       // 上一个格加入的簇
    uint8_t clusterNum;
    NoiseCluster cluster[10];
    uint8_t resultCount;  // 选中的簇的格数
    int32_t noise;        // 选中的簇的均值
    const int32_t* data;    // 原代码指向当前线的 int32 副本；这里直接读 Dif 的线，不再用（恒为空）
    const uint8_t* weight;  // 只有被裁掉的 SS 路径会传入，恒为空
};
static_assert(sizeof(NoiseState) == 0xc0);

NoiseState& Noise() { return Slot<NoiseState>(Bss().cmfNoise); }
uint8_t& PreCmfDifLogEnabled() { return Slot<uint8_t>(Bss().preCMFDiffLogEnabled); }

// CMF_InitNoiseData @ 0x6ba4d9e0
void InitNoiseData(NoiseState& st) {
    ZeroFill(st);
    st.lastIdx = 0xffff;
}

void AddToCluster(NoiseCluster& c, int32_t v) {
    c.sum += v;
    ++c.count;
    c.last = v;
    c.weightSum += 1;
}

// CMF_PushNoiseData @ 0x6ba4da6a
// 第 idx 格（Dif 为 v）紧接上一个格的归入同一簇；否则归入最近值与之相差不到 grad 的簇，都不是就开新簇
// （最多 10 簇，满了归入上一簇）。
void PushNoiseData(NoiseState& st, uint16_t idx, int32_t v, int16_t grad) {
    if (idx == st.lastIdx + 1) {
        AddToCluster(st.cluster[st.cur], v);
    } else {
        bool found = false;
        for (uint8_t k = 0; k < st.clusterNum; ++k) {
            const auto d = static_cast<int16_t>(static_cast<int16_t>(v) - static_cast<int16_t>(st.cluster[k].last));
            if (d < grad && -grad < d) {
                st.cur = k;
                AddToCluster(st.cluster[k], v);
                found = true;
                break;
            }
        }
        if (!found) {
            if (st.clusterNum < 10) {
                st.cur = st.clusterNum;
                NoiseCluster& c = st.cluster[st.cur];
                c.sum = v;
                c.count = 1;
                c.last = v;
                c.weightSum = 1;
                ++st.clusterNum;
            } else {
                AddToCluster(st.cluster[st.cur], v);
            }
        }
    }
    st.lastIdx = idx;
}

// CMF_NoiseStatistics @ 0x6ba4deb4（无权重的分支）
// 均值最小的簇格数更多、或均值为正时取它，否则取均值绝对值最小的簇。
void NoiseStatistics(NoiseState& st) {
    int16_t minAvg = 0x7fff;
    uint8_t minAvgCount = 0;
    int16_t minAbs = 0x7fff;
    st.resultCount = 0;
    for (uint8_t k = 0; k < st.clusterNum; ++k) {
        const NoiseCluster& c = st.cluster[k];
        const auto avg = static_cast<int16_t>(c.sum / static_cast<int>(c.count));
        if (avg < minAvg) {
            minAvgCount = c.count;
            minAvg = avg;
        }
        if (std::abs(avg) < std::abs(minAbs)) {
            st.resultCount = c.count;
            minAbs = avg;
        }
    }
    if (st.clusterNum == 0) {
        st.noise = 0;
        st.resultCount = 0;
    } else if (st.resultCount < minAvgCount || 0 < minAvg) {
        st.noise = minAvg;
        st.resultCount = minAvgCount;
    } else {
        st.noise = minAbs;
    }
}

// ── 进入聚类的标志 ──────────────────────────────────────────────────────────
// 原代码把每格分为高、低（超过 ±thold）、升、降（与前一格之差超过 ±grad）与平坦五类，连续两格都平坦时前一格
// 进入聚类。只有「平坦」参与判断，这里直接算每格是否进入聚类：线内 1 ≤ i < n - 1，且 i、i + 1 两格都平坦。
// 按格无分支地算（分类随噪声逐格随机，分支几乎总是预测失败），整屏一遍算完再逐线修线首线尾：一线只有几十格，
// 逐线算时向量循环的余数部分（最多 15 格的标量尾巴）比主体还贵。

/// int16 的 v 满足 -t ≤ v ≤ t 当且仅当 lo ≤ v ≤ hi（夹到 int16 范围），比较可以按 int16 宽度做。
struct Range16 {
    int16_t lo;
    int16_t hi;
};
Range16 Within(int t) {
    return {static_cast<int16_t>(std::clamp<int>(-t, INT16_MIN, INT16_MAX)), static_cast<int16_t>(std::clamp<int>(t, INT16_MIN, INT16_MAX))};
}

/// p 上 [begin, end) 各格的进入聚类标志：按 p[i - 1 .. i + 1] 判，不管线的边界。平坦：|v| ≤ thold 且与前一格
/// 之差（int16 回绕）在 ±grad 内。
/// 三个相邻格各取一个指针、共用一个下标：写成 p[i - 1] / p[i + 1] 时 -fwrapv 下下标可能回绕，循环不向量化。
void PushFlags(const int16_t* p, int begin, int end, int thold, int grad, uint8_t* push) {
    const Range16 a = Within(thold);
    const Range16 g = Within(grad);
    const int16_t* prev = p + begin - 1;
    const int16_t* cur = p + begin;
    const int16_t* next = p + begin + 1;
    uint8_t* out = push + begin;
    const int n = end - begin;
    for (int k = 0; k < n; ++k) {
        const int16_t v0 = prev[k];
        const int16_t v1 = cur[k];
        const int16_t v2 = next[k];
        const auto d1 = static_cast<int16_t>(v1 - v0);
        const auto d2 = static_cast<int16_t>(v2 - v1);
        out[k] = static_cast<uint8_t>((a.lo <= v1) & (v1 <= a.hi) & (g.lo <= d1) & (d1 <= g.hi) & (a.lo <= v2) &
                                      (v2 <= a.hi) & (g.lo <= d2) & (d2 <= g.hi));
    }
}

/// 一条连续存放的线（n 格）的进入聚类标志 push[0, n)。
void LinePushFlags(const int16_t* line, int n, int thold, int grad, uint8_t* push) {
    push[0] = 0;
    PushFlags(line, 1, n - 1, thold, grad, push);
    push[n - 1] = 0;
}

/// 整屏各行（cols 格一行）按同一门限的进入聚类标志：整屏一遍算，再把各行行首行尾置 0；其后补 8 个 0。
void GridPushFlags(const int16_t* dif, int cols, int rows, int thold, int grad, uint8_t* push) {
    const int n = cols * rows;
    push[0] = 0;
    PushFlags(dif, 1, n - 1, thold, grad, push);
    for (int r = 0; r < rows; ++r) {
        push[r * cols] = 0;
        push[r * cols + cols - 1] = 0;
    }
    std::fill_n(push + n, 8, uint8_t{0});
}

/// [i, end) 里第一个值为 want 的标志的下标，没有时为 end。一次读 8 字节按位找（标志只有 0 / 1），可能读到
/// end 之后最多 7 个字节（下一线的标志或补的 0），调用方保证可读；那些字节不影响结果。
int FindFlag(const uint8_t* p, int i, int end, bool want) {
    constexpr uint64_t kOnes = 0x0101010101010101ull;
    for (; i < end; i += 8) {
        uint64_t w;
        std::memcpy(&w, p + i, sizeof w);
        if (!want) w ^= kOnes;
        if (w != 0) return std::min(i + std::countr_zero(w) / 8, end);
    }
    return end;
}

// CMF_NoiseCal @ 0x6ba4e376
// 一条连续存放的线（n 格）按进入聚类的标志 push 聚类，求公共噪声。聚类在局部状态上做，结束时写回 g_cmfNoise。
// 进入聚类的格成段出现：段首按 PushNoiseData 找簇，段内其余格紧接上一个格，原代码逐个并入同一簇，这里整段
// 一次加上（个数按 uint8 回绕，与逐个加相同）。
int NoiseCal(const int16_t* line, const uint8_t* push, int n, int grad) {
    NoiseState st;
    InitNoiseData(st);
    const int end = n - 1;
    for (int i = FindFlag(push, 1, end, true); i < end; i = FindFlag(push, i, end, true)) {
        PushNoiseData(st, static_cast<uint16_t>(i), line[i], static_cast<int16_t>(grad));
        const int k = FindFlag(push, i + 1, end, false);
        const int len = k - i - 1;
        const int16_t* run = line + i + 1;
        int32_t sum = 0;
        for (int j = 0; j < len; ++j) sum += run[j];
        if (len != 0) {
            NoiseCluster& c = st.cluster[st.cur];
            c.sum += sum;
            c.count = static_cast<uint8_t>(c.count + len);
            c.last = line[k - 1];
            c.weightSum += len;
            st.lastIdx = static_cast<uint16_t>(k - 1);
        }
        i = k;
    }
    NoiseStatistics(st);
    Noise() = st;
    return st.noise;
}

// CMF_ProcessCoreWeight @ 0x6ba4e55b（经 CMF_ProcessCore）
// 一条连续存放的线的公共噪声；push 是按 thold 算好的进入聚类标志（其后 8 字节可读），重算时会被改写。
// 智能皮套且开了 cmfSmartCoverRefine 时，以 1.5 倍的噪声为门限（比原门限小时）再求一次。
int ProcessCore(const int16_t* line, uint8_t* push, int n, int thold, int grad) {
    int noise = NoiseCal(line, push, n, grad);
    if ((Static().featureFlags & kFeatSmartCover) != 0 && PrmtFlash().cmfSmartCoverRefine != 0) {
        const int refined = noise * 3 / 2;
        if (0 < refined && refined < thold) {
            LinePushFlags(line, n, refined, grad, push);
            noise = NoiseCal(line, push, n, grad);
        }
    }
    return noise;
}

// CMF_ProcessDimUnitOnBuf @ 0x6ba4e0ec
// 取第 line 条线（dim 1 为第 line 行，2 为第 line 列）做 CMF，减去公共噪声写回 dst（按 int16 回绕）。
// 行的格连续，直接在 src 上算；列先按步长取进局部副本。标志与副本都先写满再读，不必让编译器清零。
int ProcessDimUnitOnBuf(int16_t* dst, const int16_t* src, int dim, uint8_t line, int16_t thold, int16_t grad,
                        uint8_t cols, uint8_t rows) {
    [[clang::uninitialized]] std::array<uint8_t, kMaxLineLen + 8> push;
    if (dim == kDimRow) {
        const int start = cols * line;
        LinePushFlags(src + start, cols, thold, grad, push.data());
        std::fill_n(push.begin() + cols, 8, uint8_t{0});
        const int noise = ProcessCore(src + start, push.data(), cols, thold, grad);
        const int16_t* in = src + start;
        int16_t* out = dst + start;
        for (int i = 0; i < cols; ++i) out[i] = static_cast<int16_t>(in[i] - noise);
        return noise;
    }
    [[clang::uninitialized]] std::array<int16_t, kMaxLineLen> buf;
    const uint8_t n = rows;
    auto idx = static_cast<uint16_t>(line);
    for (uint8_t i = 0; i < n; ++i, idx = static_cast<uint16_t>(idx + cols)) buf[i] = src[idx];
    LinePushFlags(buf.data(), n, thold, grad, push.data());
    std::fill_n(push.begin() + n, 8, uint8_t{0});
    const int noise = ProcessCore(buf.data(), push.data(), n, thold, grad);
    idx = line;
    for (uint8_t i = 0; i < n; ++i, idx = static_cast<uint16_t>(idx + cols)) dst[idx] = static_cast<int16_t>(buf[i] - noise);
    return noise;
}

// CMF_ProcessDimUnit @ 0x6ba4e2af
int16_t ProcessDimUnit(int dim, uint8_t line, int16_t thold, int16_t grad, uint8_t cols, uint8_t rows) {
    int16_t* dif = Buffers().dif;
    const auto noise = static_cast<int16_t>(ProcessDimUnitOnBuf(dif, dif, dim, line, thold, grad, cols, rows));
    return noise;
}

// CMF_CopyCommonModeNoise @ 0x6ba4e7ee
void CopyCommonModeNoise(int dim, uint8_t line, uint8_t cols, uint8_t rows, int16_t noise) {
    int16_t* cmn = Buffers().commonModeNoise;
    if (dim == kDimRow) {
        std::fill_n(cmn + cols * line, cols, noise);
    } else {
        for (uint8_t r = 0; r < rows; ++r) cmn[cols * r + line] = noise;
    }
}

// CMF_ProcessDim @ 0x6ba4e8b5（自容辅助 SS_* 被裁掉，恒为 0）
// ToE 用特殊门限时，两端各两条线的门限改为 200。
// 按行做时先整屏算出各行按 thold 的进入聚类标志（各行只读写自己的格，先算后用结果相同），门限不同的行再重算。
void ProcessDim(int dim, int16_t thold, int16_t grad, uint8_t cols, uint8_t rows) {
    TsaPrpt& prpt = Prpt();
    prpt.cmfNoiseMin = 0x7fff;
    prpt.cmfNoiseMax = INT16_MIN;
    const uint8_t lines = dim == kDimRow ? rows : cols;
    const bool toe = TouchThold::ToeUseSpecialThold();
    int16_t* dif = Buffers().dif;
    [[clang::uninitialized]] std::array<uint8_t, kMaxGridCells + 8> push;
    if (dim == kDimRow) {
        TsaAssert(cols * rows <= static_cast<int>(kMaxGridCells));
        GridPushFlags(dif, cols, rows, thold, grad, push.data());
    }
    for (uint8_t line = 0; line < lines; ++line) {
        const int16_t lineThold = !toe || (1 < line && line < lines - 2) ? thold : kToeEdgeThold;
        int16_t noise;
        if (dim == kDimRow) {
            const int start = cols * line;
            uint8_t* rowPush = push.data() + start;
            if (lineThold != thold) LinePushFlags(dif + start, cols, lineThold, grad, rowPush);
            noise = static_cast<int16_t>(ProcessCore(dif + start, rowPush, cols, lineThold, grad));
            int16_t* row = dif + start;
            for (int i = 0; i < cols; ++i) row[i] = static_cast<int16_t>(row[i] - noise);
        } else {
            noise = ProcessDimUnit(dim, line, lineThold, grad, cols, rows);
        }
        CopyCommonModeNoise(dim, line, cols, rows, noise);
        if (noise < prpt.cmfNoiseMin) prpt.cmfNoiseMin = noise;
        if (prpt.cmfNoiseMax < noise) prpt.cmfNoiseMax = noise;
    }
}

// CMF_ProcessDimCommonNoise @ 0x6ba4ebf3（只以 dim 1 调用）
// 游戏场景下按行再求一遍公共噪声，记到 SelfData::commonNoise[dim1Len + 行]。
void GameCommonNoise() {
    const FlashPrmt& fl = PrmtFlash();
    SelfData& self = SelfBuf();
    for (uint8_t r = 0; r < fl.dim2Len; ++r) {
        self.commonNoise[fl.dim1Len + r] = ProcessDimUnit(kDimRow, r, kGameThold, kGameGrad, fl.dim1Len, fl.dim2Len);
    }
}

// CMF_SavePreCmfDif @ 0x6ba4f0c9
// 调试日志：按 CMFRemap 的映射表把 CMF 前的 Dif 追加到 preCMFDiff.csv。CMFRemap 被裁掉后表的行列数为 0，
// 只剩打开、关闭文件。
void SavePreCmfDif() {
    if (PreCmfDifLogEnabled() == 0) return;
    if (std::FILE* f = std::fopen("preCMFDiff.csv", "a")) std::fclose(f);
}

}  // namespace

// CMF_Malloc @ 0x6ba4d880（CMFPca / CMFRemap 被裁掉）
bool Malloc() { return true; }

// CMF_Free @ 0x6ba4d8a8
void Free() {}

// CMF_Init @ 0x6ba4d8c1（2D 滤波与 CMFRemap 的初始化被裁掉：CMFRemap 的状态无人读）
void Init() {}

// CMF_GetCmfMethod @ 0x6ba4d903
uint32_t GetMethod() {
    const FlashPrmt& fl = PrmtFlash();
    if ((Static().featureFlags & kFeatSmartCover) == 0 || fl.cmfMethodSmartCover == kSmartCoverFollowNormal) {
        return fl.cmfMethodNormal;
    }
    return fl.cmfMethodSmartCover;
}

// CMF_GetNoiseValue @ 0x6ba4d97a
int16_t GetNoiseValue(uint8_t dim1, uint8_t dim2) {
    const FlashPrmt& fl = PrmtFlash();
    if (dim1 >= fl.dim1Len || dim2 >= fl.dim2Len) return 0;
    return Buffers().commonModeNoise[GridIndex(dim1, dim2)];
}

// CMF_ProcessDimCommonOnBuf @ 0x6ba4eb1d
void ProcessDimCommonOnBuf(int16_t* dst, const int16_t* src, int dim, int16_t thold, int16_t grad, uint8_t cols,
                           uint8_t rows) {
    if (GetMethod() == kMethod2d) return;  // 2D 滤波被裁掉
    const uint8_t lines = dim == kDimRow ? rows : cols;
    for (uint8_t line = 0; line < lines; ++line) ProcessDimUnitOnBuf(dst, src, dim, line, thold, grad, cols, rows);
}

// CMF_Process @ 0x6ba4f2c8
// dim1Axis 为 0 时按列做，否则按行做；之后补偿基线，游戏场景下另求各行公共噪声。
void Process() {
    const FlashPrmt& fl = PrmtFlash();
    const DynamicPrmt& dyn = PrmtDynamic();
    TsaBufferTable& buf = Buffers();
    // CMF 前的 Dif（g_tsaBufPreCMFDif）、一阶 / 二阶差（CMF_SaveOrderDiff）、Dim1 后与全部 CMF 后的 Dif 都只写不读，不做。
    SavePreCmfDif();
    if (Static().cmfBlocked != 0) return;
    if (GetMethod() != kMethod2d) {  // 2D 滤波被裁掉
        const uint8_t cols = fl.dim1Len;
        const uint8_t rows = fl.dim2Len;
        ClearChecked(buf.commonModeNoise, GridMapBufSize() * size_t{2}, static_cast<size_t>(rows * cols) * 2);
        ProcessDim(fl.dim1Axis == 0 ? kDimCol : kDimRow, dyn.cmfThold.amp, dyn.cmfThold.grad, cols, rows);
        if (Static().blRefreshRequest == 0) Baseline::Compensation();
        if (dyn.cmfThold.gameCommonNoise != 0 && Scenario::IsGame()) GameCommonNoise();
    }
}

}  // namespace Solvers::Tsa::Core::Cmf
