#include "Alg/Cmf/Cmf.h"

#include <array>
#include <cstdio>
#include <cstdlib>

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
    const int32_t* data;
    const uint8_t* weight;  // 只有被裁掉的 SS 路径会传入，恒为空
};
static_assert(sizeof(NoiseState) == 0xc0);

NoiseState& Noise() { return Slot<NoiseState>(Bss().cmfNoise); }
uint8_t& PreCmfDifLogEnabled() { return Slot<uint8_t>(Bss().preCMFDiffLogEnabled); }

// CMF_InitNoiseData @ 0x6ba4d9e0
void InitNoiseData(NoiseState& st, const int32_t* data) {
    ZeroFill(st);
    st.data = data;
    st.weight = nullptr;
    st.lastIdx = 0xffff;
}

void AddToCluster(NoiseCluster& c, int32_t v) {
    c.sum += v;
    ++c.count;
    c.last = v;
    c.weightSum += 1;
}

// CMF_PushNoiseData @ 0x6ba4da6a
// 紧接上一个格的归入同一簇；否则归入最近值与之相差不到 grad 的簇，都不是就开新簇（最多 10 簇，满了归入上一簇）。
void PushNoiseData(NoiseState& st, uint16_t idx, int16_t grad) {
    const int32_t v = st.data[idx];
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

// CMF_NoiseCal @ 0x6ba4e376
// 每格分为高、低（超过 ±thold）、升、降（与前一格之差超过 ±grad）与平坦五类；连续两格都平坦时前一格进入
// 聚类。只有「平坦」参与判断，这里直接算平坦标志（无分支：分类随噪声逐格随机，分支几乎总是预测失败）。
// 聚类在局部状态上做，结束时写回 g_cmfNoise。
void NoiseCal(const int32_t* line, int n, int thold, int grad) {
    std::array<uint8_t, kMaxLineLen> flat;
    auto prev = static_cast<int16_t>(line[0]);
    for (int i = 1; i < n; ++i) {
        const auto v = static_cast<int16_t>(line[i]);
        const auto d = static_cast<int16_t>(v - prev);
        flat[i] = (v <= thold) & (-thold <= v) & (d <= grad) & (-grad <= d);
        prev = v;
    }
    NoiseState st;
    InitNoiseData(st, line);
    for (int i = 1; i < n - 1; ++i) {
        if ((flat[i] & flat[i + 1]) != 0) PushNoiseData(st, static_cast<uint16_t>(i), static_cast<int16_t>(grad));
    }
    NoiseStatistics(st);
    Noise() = st;
}

// CMF_ProcessCoreWeight @ 0x6ba4e55b（经 CMF_ProcessCore）
// 智能皮套且开了 cmfSmartCoverRefine 时，以 1.5 倍的噪声为门限（比原门限小时）再求一次。
int ProcessCore(int32_t* line, int n, int thold, int grad) {
    NoiseCal(line, n, thold, grad);
    if ((Static().featureFlags & kFeatSmartCover) != 0 && PrmtFlash().cmfSmartCoverRefine != 0) {
        const int refined = Noise().noise * 3 / 2;
        if (0 < refined && refined < thold) NoiseCal(line, n, refined, grad);
    }
    const int noise = Noise().noise;
    for (uint8_t i = 0; i < n; ++i) line[i] -= noise;
    return noise;
}

// CMF_ProcessDimUnitOnBuf @ 0x6ba4e0ec
// 取第 line 条线（dim 1 为第 line 行，2 为第 line 列）做 CMF，写回 dst。
int ProcessDimUnitOnBuf(int16_t* dst, const int16_t* src, int dim, uint8_t line, int16_t thold, int16_t grad,
                        uint8_t cols, uint8_t rows) {
    uint16_t start;
    uint8_t step;
    uint8_t n;
    if (dim == kDimRow) {
        start = static_cast<uint16_t>(cols * line);
        step = 1;
        n = cols;
    } else {
        start = line;
        step = cols;
        n = rows;
    }
    std::array<int32_t, kMaxLineLen> buf;
    uint16_t idx = start;
    for (uint8_t i = 0; i < n; ++i, idx = static_cast<uint16_t>(idx + step)) buf[i] = src[idx];
    const int noise = ProcessCore(buf.data(), n, thold, grad);
    idx = start;
    for (uint8_t i = 0; i < n; ++i, idx = static_cast<uint16_t>(idx + step)) dst[idx] = static_cast<int16_t>(buf[i]);
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
        for (uint8_t c = 0; c < cols; ++c) cmn[cols * line + c] = noise;
    } else {
        for (uint8_t r = 0; r < rows; ++r) cmn[cols * r + line] = noise;
    }
}

// CMF_ProcessDim @ 0x6ba4e8b5（自容辅助 SS_* 被裁掉，恒为 0）
// ToE 用特殊门限时，两端各两条线的门限改为 200。
void ProcessDim(int dim, int16_t thold, int16_t grad, uint8_t cols, uint8_t rows) {
    TsaPrpt& prpt = Prpt();
    prpt.cmfNoiseMin = 0x7fff;
    prpt.cmfNoiseMax = INT16_MIN;
    const uint8_t lines = dim == kDimRow ? rows : cols;
    const bool toe = TouchThold::ToeUseSpecialThold();
    for (uint8_t line = 0; line < lines; ++line) {
        const int16_t lineThold = !toe || (1 < line && line < lines - 2) ? thold : kToeEdgeThold;
        const int16_t noise = ProcessDimUnit(dim, line, lineThold, grad, cols, rows);
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
