#include "App/Property.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>

#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Image.h"
#include "Common/Memory.h"
#include "Legacy/RawApi.h"
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

uint8_t*& BlockIdx() { return Image::At<uint8_t*>(0x6bb81a00); }  // g_tsaBlockIdx

// TSA_GetDifPrptCommon @ 0x6baa1035
// |Dif| 不低于噪声门限的格按正负累计和与个数。
void Accumulate(Blocks& blocks) {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t thold = Prpt().noiseThold;
    const int16_t* dif = Buffers().dif;
    const uint8_t* idx = BlockIdx();
    for (uint8_t r = 0; r < fl.dim2Len; ++r) {
        for (uint8_t c = 0; c < fl.dim1Len; ++c, ++dif, ++idx) {
            const int16_t v = *dif;
            BlockAcc& b = blocks[*idx];
            if (b.maxSig < v) {
                b.maxSig = v;
                b.maxDim1 = c;
                b.maxDim2 = r;
            }
            if (v < b.minSig) {
                b.minSig = v;
                b.minDim1 = c;
                b.minDim2 = r;
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

// TSA_GetRawPrpt @ 0x6baa0839
// 整屏 raw 的极值与所在格（同一格只算一次，先比上界）；内圈（去掉最外一圈）的极值与 raw / 基线平均。
void GetRawPrpt(const uint16_t* raw, const uint16_t* bl) {
    const FlashPrmt& fl = PrmtFlash();
    const uint8_t cols = fl.dim1Len;
    const uint8_t rows = fl.dim2Len;
    uint16_t hi = 0, lo = 0xffff, innerHi = 0, innerLo = 0xffff;
    uint8_t hiDim1 = 0, hiDim2 = 0, loDim1 = 0, loDim2 = 0;
    uint32_t rawSum = 0, blSum = 0;
    int i = 0;
    for (uint8_t r = 0; r < rows; ++r) {
        const bool edgeRow = r == 0 || r == rows - 1u;
        for (uint8_t c = 0; c < cols; ++c, ++i) {
            const uint16_t v = raw[i];
            if (hi < v) {
                hi = v;
                hiDim1 = c;
                hiDim2 = r;
            } else if (v < lo) {
                lo = v;
                loDim1 = c;
                loDim2 = r;
            }
            if (!edgeRow && c != 0 && c != cols - 1u) {
                rawSum += v;
                blSum += bl[i];
                innerHi = std::max(innerHi, v);
                innerLo = std::min(innerLo, v);
            }
        }
    }
    const uint32_t n = (fl.dim1Len - 2u) * (fl.dim2Len - 2u);
    TsaPrpt& p = Prpt();
    p.blAvg = static_cast<uint16_t>(blSum / n);
    p.boundaryPositive = hi;
    p.boundaryNegative = lo;
    p.rawMax = innerHi;
    p.rawMin = innerLo;
    p.rawAvg = static_cast<uint16_t>(rawSum / n);
    p.boundaryPositiveDim1 = hiDim1;
    p.boundaryPositiveDim2 = hiDim2;
    p.boundaryNegativeDim1 = loDim1;
    p.boundaryNegativeDim2 = loDim2;
}

// TSA_GetDifPrpt @ 0x6baa0f9a
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
    std::memcpy(&p.difSnapshot, &p.dif, 0x20);
    Combine(p.difEdge, blocks, wide ? Mask({kOuterCols, kSecondCols, kTopCorners, kBottomCorners}) : Mask({kOuterCols}));
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
}

// TSA_GetPrpt @ 0x6baa0db6
void Get(const uint16_t* raw, const uint16_t* bl) {
    if ((Static().systemFlags & kSysDifBufPath) == 0) GetRawPrpt(raw, bl);
    GetDifPrpt();
}

// TSAPrpt_GetDifPreCMFPrpt @ 0x6baa13be
// 极值只在正向极值未更新时才比负向；超门限按 |Dif| ≥ noiseThold 计。
void GetDifPreCmf() {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t thold = Prpt().noiseThold;
    const int16_t* dif = Buffers().dif;
    int16_t over = 0;
    int16_t hi = 0, lo = 0;
    uint8_t hiDim1 = 0, hiDim2 = 0, loDim1 = 0, loDim2 = 0;
    for (uint8_t r = 0; r < fl.dim2Len; ++r) {
        for (uint8_t c = 0; c < fl.dim1Len; ++c, ++dif) {
            const int16_t v = *dif;
            if (thold <= v || v <= -thold) ++over;
            if (hi < v) {
                hi = v;
                hiDim1 = c;
                hiDim2 = r;
            } else if (v < lo) {
                lo = v;
                loDim1 = c;
                loDim2 = r;
            }
        }
    }
    TsaPrpt& p = Prpt();
    p.preCmfOverCount = static_cast<uint16_t>(over);
    p.preCmfMaxAbsDif = static_cast<int16_t>(std::max<int>(hi, -lo));
    p.preCmfMaxDif = hi;
    p.preCmfMaxDim1 = hiDim1;
    p.preCmfMaxDim2 = hiDim2;
    p.preCmfMinDif = lo;
    p.preCmfMinDim1 = loDim1;
    p.preCmfMinDim2 = loDim2;
}

// TSA_GetMinSigPrpt @ 0x6baa1f03
int16_t MinSig() {
    return (Static().featureFlags & kFeatSideTouch) != 0 ? Prpt().difInner.minSig : Prpt().dif.minSig;
}

// TSA_GetMaxCMFNoiseValue @ 0x6baa16c0
int MaxCmfNoise() { return std::max(std::abs(Prpt().cmfNoiseMin), std::abs(Prpt().cmfNoiseMax)); }

}  // namespace Solvers::Tsa::Core::Property

// raw 的调用方
extern "C" {
namespace P = Solvers::Tsa::Core::Property;
void TSAPrpt_GetDifPreCMFPrpt(void) { P::GetDifPreCmf(); }
void TSA_GetPrpt(int16_t* raw, int16_t* bl) { P::Get(reinterpret_cast<const uint16_t*>(raw), reinterpret_cast<const uint16_t*>(bl)); }
}  // extern "C"
