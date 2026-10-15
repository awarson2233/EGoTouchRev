#include "Alg/CTD/Ctd.h"

#include <array>

#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "App/TsaBuffer.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Ctd {

namespace {

/// TouchUnit::edgeFlags 低 4 位：触点挨着的阵列边缘。
constexpr uint32_t kEdgeDim1Near = 0x1;
constexpr uint32_t kEdgeDim1Far = 0x2;
constexpr uint32_t kEdgeDim2Near = 0x4;
constexpr uint32_t kEdgeDim2Far = 0x8;
/// TouchUnit::volatileFlags：本帧的坐标被边缘修正改动过。
constexpr uint32_t kVolatileDim1EdgeCorrected = 0x100;
constexpr uint32_t kVolatileDim2EdgeCorrected = 0x200;

/// FlashPrmt::ctdEdge 的下标。
enum EdgeTable : uint8_t { kDim1NearTable, kDim1FarTable, kDim2NearTable, kDim2FarTable };

/// g_ctd256Ln：256 项的 ln 表。
const std::array<uint16_t, 256>& Ln() { return Image::At<std::array<uint16_t, 256>>(0x6bb51d80); }

// 原代码不检查段数：segmentNum 超过 4 时按同样的步长读到分段表后面的 RamPrmt 字节。
const CtdEdgeSegment& SegmentAt(const CtdEdgePrmt& table, uint8_t idx) {
    return reinterpret_cast<const CtdEdgeSegment*>(reinterpret_cast<const uint8_t*>(&table) + offsetof(CtdEdgePrmt, segment))[idx];
}

// CTD_ECGetOffset @ 0x6ba553a6
// 按触点尺寸在当前分段表（RamPrmt::ctdEdge）里选段，返回 ratio 在该段 ln 区间里的位置 ×256，封顶 255。
int16_t GetOffset(uint8_t ratio, uint8_t sizeMm) {
    const CtdEdgePrmt& table = PrmtRam().ctdEdge;
    uint8_t seg = 0;
    while (seg < table.segmentNum - 1 && SegmentAt(table, seg).maxSizeMm < sizeMm) ++seg;
    const auto& ln = Ln();
    const uint32_t low = ln[SegmentAt(table, seg).lnLowIdx];
    const uint32_t span = ln[SegmentAt(table, seg).lnHighIdx] - low;
    int32_t offset = 0;
    if (span != 0) offset = static_cast<int32_t>((ln[ratio] - low) * 0x100) / static_cast<int32_t>(span);
    if (offset > 0xff) offset = 0xff;
    return static_cast<int16_t>(offset);
}

// CTD_ECGetFinalOffset @ 0x6ba554f7
// 质心到边缘的距离 dist（Q8.8）不超过一格时取修正值；一格到一格加 1/4 之间向原距离线性过渡；更远不修正。
int32_t GetFinalOffset(int32_t dist, int32_t corrected) {
    const int32_t beyond = dist - 0x100;
    if (beyond < 1) return corrected;
    if (beyond < 0x40) return (dist * beyond * 4 + (0x100 - beyond * 4) * corrected) / 256;
    return dist;
}

/// 一个方向上的边缘修正：先看末端再看首端。修正时把机型参数里对应的分段表复制到 RamPrmt 供 GetOffset 使用。
void CorrectAxis(TouchUnit& unit, uint16_t rawPos, uint16_t& pos, uint8_t ratio, uint32_t nearFlag, uint32_t farFlag,
                 EdgeTable nearTable, EdgeTable farTable, uint16_t nearEdge, uint16_t farEdge, uint32_t correctedFlag) {
    const FlashPrmt& flash = PrmtFlash();
    unit.volatileFlags &= ~correctedFlag;
    int32_t dist;
    int32_t final;
    if ((unit.edgeFlags & farFlag) != 0) {
        PrmtRam().ctdEdge = flash.ctdEdge[farTable];
        const int16_t offset = GetOffset(ratio, unit.sizeMm);
        dist = static_cast<int32_t>(static_cast<uint32_t>(farEdge) - rawPos);
        final = GetFinalOffset(dist, 0x100 - offset);
        pos = static_cast<uint16_t>(farEdge - static_cast<int16_t>(final));
    } else if ((unit.edgeFlags & nearFlag) != 0) {
        PrmtRam().ctdEdge = flash.ctdEdge[nearTable];
        const int16_t offset = GetOffset(ratio, unit.sizeMm);
        dist = static_cast<int32_t>(static_cast<uint32_t>(rawPos) - nearEdge);
        final = GetFinalOffset(dist, 0x100 - offset);
        pos = static_cast<uint16_t>(static_cast<int16_t>(final) + nearEdge);
    } else {
        return;
    }
    if (final != dist) unit.volatileFlags |= correctedFlag;
}

// CTD_ECProcessUnit @ 0x6ba55c6b（CTD_ECUpdatePosToEC 只是 Touch_SetEdgeDist）
// 两个方向的修正比例分别取 TouchUnit::gripRatioDim1 / gripRatioDim2（TZ 写入）在 ln 表里的值。
void EcProcessUnit(uint8_t idx) {
    Touch::SetEdgeDist(idx);
    TouchUnit& unit = Touches().unit[idx];
    const FlashConstPrmt& c = PrmtFlashConst();
    CorrectAxis(unit, unit.rawPos.dim1, unit.pos.dim1, unit.gripRatioDim1, kEdgeDim1Near, kEdgeDim1Far, kDim1NearTable,
                kDim1FarTable, c.dim1NearEdge, c.dim1FarEdge, kVolatileDim1EdgeCorrected);
    CorrectAxis(unit, unit.rawPos.dim2, unit.pos.dim2, unit.gripRatioDim2, kEdgeDim2Near, kEdgeDim2Far, kDim2NearTable,
                kDim2FarTable, c.dim2NearEdge, c.dim2FarEdge, kVolatileDim2EdgeCorrected);
}

}  // namespace

// CTD_ECProcess @ 0x6ba55ca0
void EcProcess() {
    for (uint8_t i = 0; i < Touches().count; ++i) {
        TouchUnit& unit = Touches().unit[i];
        unit.pos = unit.rawPos;
        EcProcessUnit(i);
        unit.rptPos = unit.pos;
        Touch::SetEdgeDist(i);
    }
}

// CTD_General @ 0x6ba5523c
// 窗口按 int 遍历，越界判断按 uint8 截断：负的行列号截断后超出阵列被跳过，超过 255 的会回绕到阵列内并按原值加权。
void General(uint8_t dim1, uint8_t dim2, uint8_t dim1Span, uint8_t dim2Span, uint16_t* centerDim1, uint16_t* centerDim2) {
    const FlashPrmt& flash = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    int32_t dim1Sum = 0;
    int32_t dim2Sum = 0;
    int32_t weight = 0;
    for (int d1 = dim1 - (dim1Span >> 1); d1 <= (dim1Span >> 1) + dim1; ++d1) {
        if (static_cast<uint8_t>(d1) >= flash.dim1Len) continue;
        for (int d2 = dim2 - (dim2Span >> 1); d2 <= (dim2Span >> 1) + dim2; ++d2) {
            if (static_cast<uint8_t>(d2) >= flash.dim2Len) continue;
            const int16_t sig = dif[static_cast<uint16_t>(flash.dim1Len * d2 + d1)];
            if (sig <= 0) continue;
            dim1Sum += sig * d1;
            dim2Sum += sig * d2;
            weight += sig;
        }
    }
    *centerDim1 = static_cast<uint16_t>(BasicCalculation(dim1Sum, weight));
    *centerDim2 = static_cast<uint16_t>(BasicCalculation(dim2Sum, weight));
}

// CTD_BasicCalculation @ 0x6ba551e0
int64_t BasicCalculation(int32_t weightedSum, int32_t weight) {
    if (weight == 0) return 0;
    const int64_t center = (static_cast<int64_t>(weightedSum) << 8) / weight + 0x80;
    return center < 0 ? 0 : center;
}

}  // namespace Solvers::Tsa::Core::Ctd

namespace C = Solvers::Tsa::Core::Ctd;

extern "C" {
void CTD_ECProcess(void) { C::EcProcess(); }
void CTD_General(uint8_t dim1, uint8_t dim2, uint8_t dim1Span, uint8_t dim2Span, uint16_t* centerDim1,
                 uint16_t* centerDim2) {
    C::General(dim1, dim2, dim1Span, dim2Span, centerDim1, centerDim2);
}
int64_t CTD_BasicCalculation(int32_t weightedSum, int32_t weight) { return C::BasicCalculation(weightedSum, weight); }
}  // extern "C"
