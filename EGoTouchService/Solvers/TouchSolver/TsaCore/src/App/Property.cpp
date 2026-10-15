#include "App/Property.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Bss.h"
#include "Common/Memory.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Property {

namespace {

constexpr uint32_t kSysDifBufPath = 0x10000;     // TsaStatic::systemFlags
constexpr uint32_t kAfeRawUnstable = 0x40000;    // TsaStatic::afeStatus
constexpr int16_t kBigSig = 100;

/// 区块：0 内部；1 最外两列；2 次外两列（不含上下两行）；3 / 4 次外两列与上 / 下行的交点；5 / 6 上 / 下行的其余格。
enum Block : uint8_t { kInner, kOuterCols, kSecondCols, kTopCorners, kBottomCorners, kTopRow, kBottomRow, kBlockNum };
constexpr uint32_t Mask(std::initializer_list<Block> blocks) {
    uint32_t m = 0;
    for (Block b : blocks) m |= 1u << b;
    return m;
}

/// 一个区块的累计量。
struct BlockAcc {
    int16_t maxSig = INT16_MIN;
    int16_t minSig = INT16_MAX;
    uint8_t maxDim1 = 0, maxDim2 = 0, minDim1 = 0, minDim2 = 0;
    int16_t posCount = 0;
    int16_t negCount = 0;
    int32_t negSum = 0;
    int32_t posSum = 0;
    uint16_t bigCount = 0;
};
using Blocks = std::array<BlockAcc, kBlockNum>;

uint8_t*& BlockIdx() { return Slot<uint8_t*>(Bss().blockIdx); }  // g_tsaBlockIdx

/// 同一行里属于同一区块的一段连续格 [begin, end)（格下标）。由 Init 按 g_tsaBlockIdx 切出，网格尺寸不变就不变。
struct Run {
    uint16_t begin;
    uint16_t end;
    uint8_t row;
    uint8_t block;
};
std::vector<Run> g_runs;

/// 第一次出现 v 的格，[begin, end) 里一定有。
int FirstIndexOf(const int16_t* p, int begin, int v) {
    int i = begin;
    while (p[i] != v) ++i;
    return i;
}

// TSA_GetDifPrptCommon @ 0x6baa1035
// |Dif| 不低于噪声门限的格按正负累计和与个数。原代码逐格更新所在区块的累计量；这里按区块的连续段累计：
// 段内的和与计数可向量化，极值先求段内最值、比当前值更优时再找它第一次出现的格，结果与逐格更新相同
// （极值都取扫描顺序第一次出现的格）。
void Accumulate(Blocks& blocks) {
    const int thold = Prpt().noiseThold;
    const int16_t* dif = Buffers().dif;
    const int cols = PrmtFlash().dim1Len;
    for (const Run& run : g_runs) {
        if (run.end - run.begin <= 2) {
            // 最外两列、次外两列的段只有一两格，逐格更新（原代码的做法）比向量化的固定开销小
            BlockAcc& b = blocks[run.block];
            for (int i = run.begin; i < run.end; ++i) {
                const int16_t v = dif[i];
                if (b.maxSig < v) {
                    b.maxSig = v;
                    b.maxDim1 = static_cast<uint8_t>(i - run.row * cols);
                    b.maxDim2 = run.row;
                }
                if (v < b.minSig) {
                    b.minSig = v;
                    b.minDim1 = static_cast<uint8_t>(i - run.row * cols);
                    b.minDim2 = run.row;
                }
                if (thold <= v) {
                    b.posSum += v;
                    ++b.posCount;
                } else if (v <= -thold) {
                    b.negSum += v;
                    ++b.negCount;
                }
                if (kBigSig < v || v < -kBigSig) ++b.bigCount;
            }
            continue;
        }
        int mx = INT16_MIN;
        int mn = INT16_MAX;
        int posSum = 0, negSum = 0, posCount = 0, negCount = 0, bigCount = 0;
        for (int i = run.begin; i < run.end; ++i) {
            const int v = dif[i];
            mx = std::max(mx, v);
            mn = std::min(mn, v);
            const bool pos = thold <= v;
            const bool neg = !pos && v <= -thold;
            posSum += pos ? v : 0;
            posCount += pos ? 1 : 0;
            negSum += neg ? v : 0;
            negCount += neg ? 1 : 0;
            bigCount += kBigSig < v || v < -kBigSig ? 1 : 0;
        }
        BlockAcc& b = blocks[run.block];
        const int rowStart = run.row * cols;
        if (b.maxSig < mx) {
            b.maxSig = static_cast<int16_t>(mx);
            b.maxDim1 = static_cast<uint8_t>(FirstIndexOf(dif, run.begin, mx) - rowStart);
            b.maxDim2 = run.row;
        }
        if (mn < b.minSig) {
            b.minSig = static_cast<int16_t>(mn);
            b.minDim1 = static_cast<uint8_t>(FirstIndexOf(dif, run.begin, mn) - rowStart);
            b.minDim2 = run.row;
        }
        b.posSum += posSum;
        b.negSum += negSum;
        b.posCount = static_cast<int16_t>(b.posCount + posCount);
        b.negCount = static_cast<int16_t>(b.negCount + negCount);
        b.bigCount = static_cast<uint16_t>(b.bigCount + bigCount);
    }
}

/// 极值相等时取扫描顺序靠前的格。
bool Before(uint8_t dim1, uint8_t dim2, uint8_t refDim1, uint8_t refDim2) {
    const uint16_t cols = PrmtFlash().dim1Len;
    return static_cast<uint16_t>(dim2 * cols + dim1) < static_cast<uint16_t>(refDim1 + refDim2 * cols);
}

// TSA_CombineBlock @ 0x6baa0de5
// mask 里的区块组合成一组 DifPrpt；其后的 4 字节一并清零（原样写 32 字节）。
void Combine(DifPrpt& out, const Blocks& blocks, uint32_t mask) {
    DifPrpt p{};
    p.maxSig = INT16_MIN;
    p.minSig = INT16_MAX;
    bool first = true;
    for (uint32_t k = 0; k < kBlockNum; ++k) {
        if ((mask & (1u << k)) == 0) continue;
        const BlockAcc& b = blocks[k];
        if (first) {
            p.maxDim1 = b.maxDim1;
            p.maxDim2 = b.maxDim2;
            p.minDim1 = b.minDim1;
            p.minDim2 = b.minDim2;
            first = false;
        }
        if (p.maxSig < b.maxSig || (b.maxSig == p.maxSig && Before(b.maxDim1, b.maxDim2, p.maxDim1, p.maxDim2))) {
            p.maxSig = b.maxSig;
            p.maxDim1 = b.maxDim1;
            p.maxDim2 = b.maxDim2;
        }
        if (b.minSig < p.minSig || (b.minSig == p.minSig && Before(b.minDim1, b.minDim2, p.minDim1, p.minDim2))) {
            p.minSig = b.minSig;
            p.minDim1 = b.minDim1;
            p.minDim2 = b.minDim2;
        }
        p.posSum += b.posSum;
        p.negSum += b.negSum;
        p.posCount = static_cast<int16_t>(p.posCount + b.posCount);
        p.negCount = static_cast<int16_t>(p.negCount + b.negCount);
        p.bigCount = static_cast<uint16_t>(p.bigCount + b.bigCount);
    }
    p.sum = p.negSum + p.posSum;
    p.count = static_cast<int16_t>(p.posCount + p.negCount);
    out = p;
    std::memset(reinterpret_cast<uint8_t*>(&out) + sizeof(DifPrpt), 0, 4);
}

/// 第一次出现 v 的格，[begin, n) 里一定有。
int FirstIndexOf(const uint16_t* p, int begin, uint16_t v) {
    int i = begin;
    while (p[i] != v) ++i;
    return i;
}

// TSA_GetRawPrpt @ 0x6baa0839
// 整屏 raw 的极值与所在格；内圈（去掉最外一圈）的极值与 raw / 基线平均。
// 原代码逐格「先比上界（初值 0），不是新高时才比下界（初值 0xffff）」，都是严格比较。新高与新低只可能在下界
// 第一次更新之前同时成立：那之后下界不高于上界，新高不会低于下界。所以上界是第一次出现的最大值；下界是
// 从第一个不是新高的格 p 起第一次出现的最小值。
void GetRawPrpt(const uint16_t* raw, const uint16_t* bl) {
    const FlashPrmt& fl = PrmtFlash();
    const int cols = fl.dim1Len;
    const int rows = fl.dim2Len;
    const int n = cols * rows;
    TsaPrpt& p = Prpt();
    int first = 0;
    for (uint16_t run = 0; first < n && run < raw[first]; ++first) run = raw[first];
    uint16_t hi = 0;
    uint16_t lo = 0xffff;
    for (int i = 0; i < n; ++i) hi = std::max(hi, raw[i]);
    for (int i = first; i < n; ++i) lo = std::min(lo, raw[i]);
    p.boundaryPositive = hi;
    p.boundaryNegative = lo;
    p.boundaryPositiveDim1 = 0;
    p.boundaryPositiveDim2 = 0;
    p.boundaryNegativeDim1 = 0;
    p.boundaryNegativeDim2 = 0;
    if (hi != 0) {
        const int i = FirstIndexOf(raw, 0, hi);
        p.boundaryPositiveDim1 = static_cast<uint8_t>(i % cols);
        p.boundaryPositiveDim2 = static_cast<uint8_t>(i / cols);
    }
    if (lo != 0xffff) {
        const int i = FirstIndexOf(raw, first, lo);
        p.boundaryNegativeDim1 = static_cast<uint8_t>(i % cols);
        p.boundaryNegativeDim2 = static_cast<uint8_t>(i / cols);
    }
    uint16_t innerHi = 0, innerLo = 0xffff;
    uint32_t rawSum = 0, blSum = 0;
    for (int r = 1; r < rows - 1; ++r) {
        for (int i = r * cols + 1; i < r * cols + cols - 1; ++i) {
            rawSum += raw[i];
            blSum += bl[i];
            innerHi = std::max(innerHi, raw[i]);
            innerLo = std::min(innerLo, raw[i]);
        }
    }
    const uint32_t inner = (fl.dim1Len - 2u) * (fl.dim2Len - 2u);
    p.blAvg = static_cast<uint16_t>(blSum / inner);
    p.rawMax = innerHi;
    p.rawMin = innerLo;
    p.rawAvg = static_cast<uint16_t>(rawSum / inner);
}

// TSA_GetDifPrpt @ 0x6baa0f9a
// 原代码另把 dif 拷一份快照、再组合一组边缘统计（difSnapshot / difEdge），没有模块读，不做。
void GetDifPrpt() {
    Blocks blocks{};
    Accumulate(blocks);
    TsaPrpt& p = Prpt();
    const TsaStatic& s = Static();
    const bool wide = (s.featureFlags & kFeatSideTouch) != 0;
    Combine(p.difCore, blocks, Mask({kInner, kSecondCols}));
    uint32_t all = 0x7f;
    if (wide) all = (s.afeStatus & kAfeRawUnstable) != 0 ? Mask({kInner, kSecondCols, kBottomCorners, kBottomRow}) : 0x7f & ~Mask({kOuterCols});
    Combine(p.dif, blocks, all);
    Combine(p.difInner, blocks, Mask({kInner}));
}

}  // namespace

// TSAPrpt_Malloc @ 0x6baa076c
bool Malloc() {
    BlockIdx() = static_cast<uint8_t*>(Mem::Allocate(GridBufSize()));
    return BlockIdx() != nullptr;
}

// TSAPrpt_Free @ 0x6baa07ad
void Free() { Mem::Release(BlockIdx()); }

// TSAPrpt_Init @ 0x6baa07c8 → TSA_InitBlockIdx @ 0x6baa0b46
void Init() {
    const FlashPrmt& fl = PrmtFlash();
    const uint32_t cols = fl.dim1Len;
    const uint32_t rows = fl.dim2Len;
    uint8_t* idx = BlockIdx();
    std::memset(idx, kInner, fl.sensorNum);
    const auto at = [&](uint32_t i) -> uint8_t& { return idx[i & 0xffff]; };
    for (uint32_t r = 0; r < rows; ++r) {
        at(cols * r) = kOuterCols;
        at(cols * r + cols - 1) = kOuterCols;
    }
    for (uint32_t r = 1; static_cast<int>(r) < static_cast<int>(rows - 1); ++r) {
        at(cols * r + 1) = kSecondCols;
        at(cols * r + cols - 2) = kSecondCols;
    }
    at(1) = kTopCorners;
    at(cols - 2) = kTopCorners;
    at(cols * (rows - 1) + 1) = kBottomCorners;
    at(cols * (rows - 1) + cols - 2) = kBottomCorners;
    for (uint32_t c = 2; static_cast<int>(c) < static_cast<int>(cols - 2); ++c) {
        at(c) = kTopRow;
        at(c + (rows - 1) * cols) = kBottomRow;
    }
    g_runs.clear();
    for (uint32_t r = 0; r < rows; ++r) {
        for (uint32_t c = 0; c < cols;) {
            const uint32_t begin = r * cols + c;
            const uint8_t block = idx[begin];
            while (c < cols && idx[r * cols + c] == block) ++c;
            g_runs.push_back({static_cast<uint16_t>(begin), static_cast<uint16_t>(r * cols + c), static_cast<uint8_t>(r), block});
        }
    }
}

// TSA_GetPrpt @ 0x6baa0db6
void Get(const uint16_t* raw, const uint16_t* bl) {
    if ((Static().systemFlags & kSysDifBufPath) == 0) GetRawPrpt(raw, bl);
    GetDifPrpt();
}

// TSAPrpt_GetDifPreCMFPrpt @ 0x6baa13be
// 超门限按 |Dif| ≥ noiseThold 计。原代码的极值初值都是 0，「先比上界、不是新高才比下界」：上界不低于 0、
// 下界不高于 0，两者不会同时成立，所以就是 max(0, 最大值) 与 min(0, 最小值)。极值本身与所在格没有模块读，
// 只留最大绝对值。
void GetDifPreCmf() {
    const FlashPrmt& fl = PrmtFlash();
    const int thold = Prpt().noiseThold;
    const int16_t* dif = Buffers().dif;
    const int n = fl.dim1Len * fl.dim2Len;
    int over = 0;
    int hi = 0, lo = 0;
    for (int i = 0; i < n; ++i) {
        const int v = dif[i];
        over += thold <= v || v <= -thold ? 1 : 0;
        hi = std::max(hi, v);
        lo = std::min(lo, v);
    }
    TsaPrpt& p = Prpt();
    p.preCmfOverCount = static_cast<uint16_t>(over);
    p.preCmfMaxAbsDif = static_cast<int16_t>(std::max(hi, -lo));
}

// TSA_GetMinSigPrpt @ 0x6baa1f03
int16_t MinSig() {
    return (Static().featureFlags & kFeatSideTouch) != 0 ? Prpt().difInner.minSig : Prpt().dif.minSig;
}

// TSA_GetMaxCMFNoiseValue @ 0x6baa16c0
int MaxCmfNoise() { return std::max(std::abs(Prpt().cmfNoiseMin), std::abs(Prpt().cmfNoiseMax)); }

}  // namespace Solvers::Tsa::Core::Property
