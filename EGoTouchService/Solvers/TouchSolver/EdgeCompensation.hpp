#pragma once
// ── TouchPipeline Module: EdgeCompensation (pipeline module) ──
// Header-only. Converted from TouchSolver/EdgeCompensation.cpp.
// The data types (EdgeBounds, ZoneEdgeInfo, etc.) remain in EdgeCompensation.h.
// This file contains the EdgeCompensator class with inlined processing logic.

#include "EdgeCompensation.h"
#include <algorithm>
#include <cstdint>

namespace Solvers { namespace Touch {

// ── g_ctd256Ln[256]: official LUT from firmware ──
static const uint16_t g_ctd256Ln[256] = {
    0x0000, 0x0000, 0x00B1, 0x0119, 0x0162, 0x019C, 0x01CA, 0x01F2,
    0x0214, 0x0232, 0x024D, 0x0265, 0x027C, 0x0290, 0x02A3, 0x02B5,
    0x02C5, 0x02D5, 0x02E3, 0x02F1, 0x02FE, 0x030B, 0x0317, 0x0322,
    0x032D, 0x0338, 0x0342, 0x034B, 0x0355, 0x035E, 0x0366, 0x036F,
    0x0377, 0x037F, 0x0386, 0x038E, 0x0395, 0x039C, 0x03A3, 0x03A9,
    0x03B0, 0x03B6, 0x03BC, 0x03C2, 0x03C8, 0x03CE, 0x03D4, 0x03D9,
    0x03DF, 0x03E4, 0x03E9, 0x03EE, 0x03F3, 0x03F8, 0x03FD, 0x0401,
    0x0406, 0x040B, 0x040F, 0x0413, 0x0418, 0x041C, 0x0420, 0x0424,
    0x0428, 0x042C, 0x0430, 0x0434, 0x0438, 0x043B, 0x043F, 0x0443,
    0x0446, 0x044A, 0x044D, 0x0451, 0x0454, 0x0458, 0x045B, 0x045E,
    0x0461, 0x0464, 0x0468, 0x046B, 0x046E, 0x0471, 0x0474, 0x0477,
    0x047A, 0x047D, 0x047F, 0x0482, 0x0485, 0x0488, 0x048B, 0x048D,
    0x0490, 0x0493, 0x0495, 0x0498, 0x049A, 0x049D, 0x049F, 0x04A2,
    0x04A4, 0x04A7, 0x04A9, 0x04AC, 0x04AE, 0x04B0, 0x04B3, 0x04B5,
    0x04B7, 0x04BA, 0x04BC, 0x04BE, 0x04C0, 0x04C3, 0x04C5, 0x04C7,
    0x04C9, 0x04CB, 0x04CD, 0x04CF, 0x04D1, 0x04D4, 0x04D6, 0x04D8,
    0x04DA, 0x04DC, 0x04DE, 0x04E0, 0x04E1, 0x04E3, 0x04E5, 0x04E7,
    0x04E9, 0x04EB, 0x04ED, 0x04EF, 0x04F1, 0x04F2, 0x04F4, 0x04F6,
    0x04F8, 0x04FA, 0x04FB, 0x04FD, 0x04FF, 0x0501, 0x0502, 0x0504,
    0x0506, 0x0507, 0x0509, 0x050B, 0x050C, 0x050E, 0x0510, 0x0511,
    0x0513, 0x0514, 0x0516, 0x0518, 0x0519, 0x051B, 0x051C, 0x051E,
    0x051F, 0x0521, 0x0522, 0x0524, 0x0525, 0x0527, 0x0528, 0x052A,
    0x052B, 0x052D, 0x052E, 0x052F, 0x0531, 0x0532, 0x0534, 0x0535,
    0x0537, 0x0538, 0x0539, 0x053B, 0x053C, 0x053D, 0x053F, 0x0540,
    0x0541, 0x0543, 0x0544, 0x0545, 0x0547, 0x0548, 0x0549, 0x054B,
    0x054C, 0x054D, 0x054E, 0x0550, 0x0551, 0x0552, 0x0553, 0x0555,
    0x0556, 0x0557, 0x0558, 0x055A, 0x055B, 0x055C, 0x055D, 0x055E,
    0x0560, 0x0561, 0x0562, 0x0563, 0x0564, 0x0565, 0x0567, 0x0568,
    0x0569, 0x056A, 0x056B, 0x056C, 0x056D, 0x056F, 0x0570, 0x0571,
    0x0572, 0x0573, 0x0574, 0x0575, 0x0576, 0x0577, 0x0578, 0x0579,
    0x057B, 0x057C, 0x057D, 0x057E, 0x057F, 0x0580, 0x0581, 0x0582,
    0x0583, 0x0584, 0x0585, 0x0586, 0x0587, 0x0588, 0x0589, 0x058A
};

// ── Default EC profiles (per edge direction) ──
static const ECProfile g_defaultECProfiles[4] = {
    { 3, { {64, 2, 32}, {128, 32, 96}, {255, 96, 192}, {0,0,0} } },
    { 3, { {64, 2, 32}, {128, 32, 96}, {255, 96, 192}, {0,0,0} } },
    { 3, { {64, 2, 32}, {128, 32, 96}, {255, 96, 192}, {0,0,0} } },
    { 3, { {64, 2, 32}, {128, 32, 96}, {255, 96, 192}, {0,0,0} } },
};

static inline int ECGetOffset(uint8_t subIdx, uint8_t edgeW,
                               const ECProfile& prof) {
    int si = 0;
    while (si < prof.numSegments - 1 &&
           prof.segments[si].edgeWidthThreshold < edgeW)
        si++;
    auto& s = prof.segments[si];
    uint16_t hi = g_ctd256Ln[s.lutIdxHigh];
    uint16_t lo = g_ctd256Ln[s.lutIdxLow];
    if (hi == lo) return 0;
    int r = (int(g_ctd256Ln[subIdx] - lo) * 0x100) / int(hi - lo);
    return std::min(r, 0xFF);
}

static inline int ECGetFinalOffset(int rawDist, int compOff, int blendRange) {
    if (rawDist <= 0) return 0;
    if (compOff >= rawDist) return rawDist;
    if (blendRange <= 0 || rawDist >= blendRange) return rawDist;
    return (rawDist * rawDist + compOff * (blendRange - rawDist)) / blendRange;
}

class EdgeCompensator {
public:
    bool m_enabled = true;
    EdgeBounds m_bounds;
    float m_ecBlendRange = 2.0f;

    inline void Process(std::vector<TouchContact>& contacts,
                        const std::vector<ZoneEdgeInfo>& edgeInfos,
                        const EdgeBounds& bounds) {
        if (!m_enabled) return;
        for (int i = 0; i < (int)contacts.size(); ++i) {
            if (i >= (int)edgeInfos.size()) break;
            auto& tc = contacts[i];
            auto& ei = edgeInfos[i];
            if (!(ei.edgeFlags & 0x20)) continue;
            tc.isEdge = true;
            ProcessDim(tc.x, float(bounds.colMin), float(bounds.colMax),
                       ei.edgeFlags, ei, true);
            ProcessDim(tc.y, float(bounds.rowMin), float(bounds.rowMax),
                       ei.edgeFlags, ei, false);
        }
    }

private:
    inline void ProcessDim(float& coord, float boundNear, float boundFar,
                           uint32_t edgeFlags, const ZoneEdgeInfo& ei,
                           bool isDimX) {
        bool nearEdge = isDimX ? (ei.minCol <= kGridColMin)
                               : (ei.minRow <= kGridRowMin);
        bool farEdge  = isDimX ? (ei.maxCol >= kGridColMax)
                               : (ei.maxRow >= kGridRowMax);
        if (!nearEdge && !farEdge) return;

        int outer = isDimX ? ei.outerColSigSum : ei.outerRowSigSum;
        int inner = isDimX ? ei.innerColSigSum : ei.innerRowSigSum;
        uint8_t edgeWidth = (inner > 0)
            ? uint8_t(std::min(outer * 255 / inner, 255))
            : 0;

        int q8coord = int(coord * 256.0f);
        int rawDist;
        if (farEdge)
            rawDist = int(boundFar * 256.0f) - q8coord;
        else
            rawDist = q8coord - int(boundNear * 256.0f);
        if (rawDist < 0) rawDist = 0;

        uint8_t subIdx = uint8_t(std::min(rawDist, 255));
        int profIdx = isDimX ? (farEdge ? 1 : 0) : (farEdge ? 3 : 2);
        int offset = ECGetOffset(subIdx, edgeWidth,
                                 g_defaultECProfiles[profIdx]);
        int compOff = 256 - offset;

        int blendRange = int(m_ecBlendRange * 256.0f);
        int finalOff = ECGetFinalOffset(rawDist, compOff, blendRange);

        if (farEdge)
            coord = boundFar - float(finalOff) / 256.0f;
        else
            coord = boundNear + float(finalOff) / 256.0f;
    }
};

}} // namespace Solvers::Touch
