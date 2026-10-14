#include "Alg/IDTracking/IdTracking.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <span>

#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchSize/TouchSize.h"
#include "App/TsaStatic.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::IdTracking {

namespace {

constexpr uint8_t kNoMatch = 0xff;
constexpr int32_t kAnchoredCost = -0x3fffffff;            // 锚定配对的代价（0xc0000001）
constexpr uint32_t kPrevExceptionUseTimestamps = 0x100;   // PrevTouchTable::exceptionFlags 恰为此值时帧间隔按时间戳算
constexpr uint32_t kProcNoSnapMinAge = 0x10;              // TsaStatic::processingState：锚定不要求轨迹帧龄
constexpr uint16_t kSnapMinAge = 5;
constexpr uint32_t kFeatureWideDim1Edge = 0x8000;         // TsaStatic::featureFlags：Dim1 两端各两列算边缘
constexpr uint32_t kPersistNoIdMapping = 0x200000;        // PrevTouchUnit::persistFlags：本帧不允许配对
constexpr uint32_t kPersist2NoIncAccThold = 0x3000000;    // PrevTouchUnit::persistFlags2：不放大加速度门限
constexpr uint16_t kIncAccTholdSqDistMm = 0x19;           // 帧间位移平方（mm²）达到此值时放大加速度门限
constexpr uint32_t kGripIdMappingCancelled = 0x200;       // TouchUnit::gripFlags：GripFilter 取消了配对
constexpr uint32_t kSpecialAccTooLarge = 0x4;             // TsaStatic::specialControlFlags

/// 历史下标 → 当前下标（kNoMatch 为没配上）。
using Matching = std::array<uint8_t, kMaxTouchNum>;
/// 匈牙利的结果：多的一方的下标 → 少的一方的下标。
using Assignment = std::array<uint8_t, kMaxTouchNum>;
/// 代价矩阵按 [多的一方][少的一方] 排布，一行 kMaxTouchNum 格。
using CostMatrix = std::array<int32_t, kMaxTouchNum * kMaxTouchNum>;

struct Candidate {
    uint8_t curIdx;
    uint8_t prevIdx;
};

TouchUnit& Cur(uint8_t idx) { return Touches().unit[idx]; }
PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }

// 原代码填矩阵时下标累加器是 uint8，整体对 256 取模；匈牙利读矩阵时只有行偏移是 uint8，加上列号不取模。
// 两者只在行偏移为 240（第 12 行）且列号 ≥ 16 时不同，此时读到的是没写过的格子（初值 0）。
std::size_t WriteCell(uint8_t row, uint8_t col) { return static_cast<uint8_t>(row * kMaxTouchNum + col); }
std::size_t ReadCell(uint8_t row, uint8_t col) { return static_cast<std::size_t>(static_cast<uint8_t>(row * kMaxTouchNum)) + col; }

/// 按位取绝对值（INT_MIN 仍为 INT_MIN），与原代码的 (x ^ s) - s 一致。
int32_t WrapAbs(int32_t v) {
    const auto sign = static_cast<uint32_t>(v >> 31);
    return static_cast<int32_t>((static_cast<uint32_t>(v) ^ sign) - sign);
}

bool UsesTimestamps() { return PrevTouches().exceptionFlags == kPrevExceptionUseTimestamps; }

/// 当前触点（没有时间戳时取本帧时间）与历史触点的时间差（ms）。
uint64_t ElapsedMs(const TouchUnit& cur, const PrevTouchUnit& prev) {
    return (cur.timestamp == 0 ? Static().curTimeMs : cur.timestamp) - prev.timestamp;
}

/// 加速度门限：accThold（mm，取低字节）换成像素后 /16 再平方。
int64_t AccThold() {
    const auto px = static_cast<uint16_t>(TouchSize::MmToPixel(static_cast<uint8_t>(PrmtFlash().accThold)));
    const uint32_t q = px >> 4;
    return static_cast<int32_t>(q * q);
}

// IDT_GetAcc @ 0x6ba61704
// 当前触点相对历史触点的速度（位移 / 帧间隔），第二帧取速度平方的 1/4，之后取速度平方与加速度平方的较小者。
// 帧间隔默认取 |TsaStatic::frameIntervalMs|（原代码如此）。
int32_t GetAcc(uint8_t curIdx, uint8_t prevIdx) {
    const TouchUnit& cur = Cur(curIdx);
    const PrevTouchUnit& prev = Prev(prevIdx);
    int32_t dt = WrapAbs(Static().frameIntervalMs);
    if (UsesTimestamps()) dt = static_cast<int32_t>(ElapsedMs(cur, prev));
    if (dt == 0) dt = 1;
    const int32_t vx = (cur.rawPos.dim1 - prev.rawPos.dim1) / dt;
    const int32_t vy = (cur.rawPos.dim2 - prev.rawPos.dim2) / dt;
    const int32_t ax = vx - prev.rawVelocity.dim1;
    const int32_t ay = vy - prev.rawVelocity.dim2;
    const int32_t speedSq = vy * vy + vx * vx;
    if (prev.age == 1) return speedSq / 4;
    return std::min(ay * ay + ax * ax, speedSq);
}

// IDT_IsToIncAccThold @ 0x6ba615c7
// 放大加速度门限（×4）：不是握持中的普通触点，历史没有 persistFlags2 的禁止位，且处于异常、
// 噪声等级 ≥ 2 或上一帧位移已经较大。
bool IsToIncAccThold(uint8_t curIdx, uint8_t prevIdx) {
    const PrevTouchUnit& prev = Prev(prevIdx);
    if (!GripFilter_IsPeakAbsGrip(Cur(curIdx).peakIdx) && prev.gripStage == 1) return false;
    if ((prev.persistFlags2 & kPersist2NoIncAccThold) != 0) return false;
    if (Touch::IsException(curIdx, prevIdx) || Static().noiseLevel >= 2) return true;
    return prev.maxSqDistMmFromPrev >= kIncAccTholdSqDistMm;
}

/// 这对触点的加速度在门限内。
bool AccFits(uint8_t curIdx, uint8_t prevIdx, int64_t accThold) {
    const int32_t acc = GetAcc(curIdx, prevIdx);
    return acc <= (IsToIncAccThold(curIdx, prevIdx) ? accThold << 2 : accThold);
}

// IDT_IsEdgeTouch @ 0x6ba622e9
// 质心落在最外一圈传感器上（featureFlags 0x8000 时 Dim1 两端各两列）。
bool IsEdgeTouch(uint8_t curIdx) {
    const Point pos = Cur(curIdx).rawPos;
    const unsigned col = pos.dim1 >> 8;
    const unsigned row = pos.dim2 >> 8;
    const uint8_t margin = (Static().featureFlags & kFeatureWideDim1Edge) != 0 ? 2 : 1;
    const bool inner = col >= margin && col < static_cast<uint8_t>(PrmtFlash().dim1Len - margin) && row != 0 &&
                       row < static_cast<uint8_t>(PrmtFlash().dim2Len - 1);
    return !inner;
}

// IDT_AlwaysMatchProcess @ 0x6ba620ac
// 候选配对在加速度门限下一对一相容（当前触点只与这个历史触点相容，这个历史触点也只与这个当前触点相容）时锚定。
void AnchorExclusivePairs(CostMatrix& cost, bool prevMajor, std::span<const Candidate> candidates) {
    const int64_t accThold = AccThold();
    for (const Candidate& cand : candidates) {
        bool exclusive = true;
        for (uint8_t p = 0; p < PrevTouches().count && exclusive; ++p) {
            exclusive = AccFits(cand.curIdx, p, accThold) == (p == cand.prevIdx);
        }
        for (uint8_t c = 0; c < Touches().count && exclusive; ++c) {
            if (c != cand.curIdx) exclusive = !AccFits(c, cand.prevIdx, accThold);
        }
        if (!exclusive) continue;
        cost[prevMajor ? WriteCell(cand.prevIdx, cand.curIdx) : WriteCell(cand.curIdx, cand.prevIdx)] = kAnchoredCost;
    }
}

// IDT_PopulateDistanceMap @ 0x6ba623e4
// 代价为位置差的平方。位移与按速度外推的位移都不超过 idtSnapDistMm、且轨迹帧龄超过 5 的配对直接锚定；
// 其余配对里外推位置（含半个加速度）落在该距离内、不在边缘的记为候选，交给 AnchorExclusivePairs。
// 有触点消失（历史多于当前）时矩阵以历史为行，每格再加上 2 × (位移 − 速度)²。
void PopulateDistanceMap(CostMatrix& cost, bool prevMajor) {
    const TouchTable& touches = Touches();
    const PrevTouchTable& prevTable = PrevTouches();
    const auto snapDist = static_cast<int16_t>(TouchSize::MmToPixel(PrmtFlash().idtSnapDistMm));
    const uint16_t snapMinAge = (Static().processingState & kProcNoSnapMinAge) != 0 ? 0 : kSnapMinAge;
    std::array<Candidate, kMaxTouchNum> candidates{};
    uint8_t candidateCount = 0;

    for (uint8_t c = 0; c < touches.count; ++c) {
        const TouchUnit& cur = touches.unit[c];
        for (uint8_t p = 0; p < prevTable.count; ++p) {
            const PrevTouchUnit& prev = prevTable.unit[p];
            const int32_t dx = cur.rawPos.dim1 - prev.rawPos.dim1;
            const int32_t dy = cur.rawPos.dim2 - prev.rawPos.dim2;
            int32_t cellCost = dy * dy + dx * dx;

            int16_t dt = prev.dt;
            if (UsesTimestamps()) dt = static_cast<int16_t>(ElapsedMs(cur, prev));
            const SPoint vel = prev.rawVelocity;
            const int32_t moveX = dt * vel.dim1;
            const int32_t moveY = dt * vel.dim2;
            const int32_t predX = dt * (prev.rawAccel.dim1 / 2) + moveX + prev.rawPos.dim1;
            const int32_t predY = dt * (prev.rawAccel.dim2 / 2) + moveY + prev.rawPos.dim2;
            const int32_t predErr = WrapAbs(predX - cur.rawPos.dim1) + WrapAbs(predY - cur.rawPos.dim2);

            const bool snap = WrapAbs(dx) + WrapAbs(dy) <= snapDist && prev.age > snapMinAge &&
                              WrapAbs(moveX) + WrapAbs(moveY) <= snapDist;
            if (snap) {
                cellCost = kAnchoredCost;
            } else if (predErr <= snapDist && !IsEdgeTouch(c) && candidateCount < kMaxTouchNum) {
                candidates[candidateCount++] = {c, p};
            }
            if (prevMajor) {
                const int32_t ex = dx - vel.dim1;
                const int32_t ey = dy - vel.dim2;
                cellCost += (ey * ey + ex * ex) * 2;
            }
            cost[prevMajor ? WriteCell(p, c) : WriteCell(c, p)] = cellCost;
        }
    }
    AnchorExclusivePairs(cost, prevMajor, std::span(candidates.data(), candidateCount));
}

// IDT_HM @ 0x6ba61ba2
// 匈牙利算法（势 + 最短增广路）：少的一方 smallCount 个，多的一方 bigCount 个，代价取 cost[ReadCell(big, small)]。
// 返回多的一方每项分到的少的一方下标。
Assignment Solve(uint8_t smallCount, uint8_t bigCount, const CostMatrix& cost) {
    Assignment owner;
    owner.fill(kNoMatch);
    std::array<int32_t, kMaxTouchNum> u{};  // 势：少的一方
    std::array<int32_t, kMaxTouchNum> v{};  // 势：多的一方
    for (uint8_t i = 0; i < smallCount; ++i) {
        std::array<int32_t, kMaxTouchNum> minSlack;
        minSlack.fill(INT32_MAX);
        std::array<bool, kMaxTouchNum> used{};
        std::array<uint8_t, kMaxTouchNum> way;  // 增广路上的前一项
        way.fill(kNoMatch);

        uint8_t prevCol = kNoMatch;
        uint8_t col = kNoMatch;
        for (uint8_t row = i; row != kNoMatch; row = owner[col]) {
            col = kNoMatch;
            for (uint8_t j = 0; j < bigCount; ++j) {
                if (used[j]) continue;
                const int32_t slack = cost[ReadCell(j, row)] - u[row] - v[j];
                if (slack < minSlack[j]) {
                    minSlack[j] = slack;
                    way[j] = prevCol;
                }
                if (col == kNoMatch || minSlack[j] < minSlack[col]) col = j;
            }
            const int32_t delta = minSlack[col];
            for (uint8_t j = 0; j < bigCount; ++j) {
                if (used[j]) {
                    u[owner[j]] += delta;
                    v[j] -= delta;
                } else {
                    minSlack[j] -= delta;
                }
            }
            u[i] += delta;
            used[col] = true;
            prevCol = col;
        }
        for (; way[col] != kNoMatch; col = way[col]) owner[col] = owner[way[col]];
        owner[col] = i;
    }
    return owner;
}

// IDT_HMProcess @ 0x6ba61323
Matching Match() {
    const uint8_t curCount = Touches().count;
    const uint8_t prevCount = PrevTouches().count;
    const bool prevMajor = curCount < prevCount;
    CostMatrix cost{};
    PopulateDistanceMap(cost, prevMajor);
    if (prevMajor) return Solve(curCount, prevCount, cost);
    const Assignment owner = Solve(prevCount, curCount, cost);
    Matching match;
    match.fill(kNoMatch);
    for (uint8_t c = 0; c < curCount; ++c) {
        if (owner[c] != kNoMatch) match[owner[c]] = c;
    }
    return match;
}

// IDT_AccCheck @ 0x6ba619a3
// 拆掉加速度超门限（strict 时门限一律 ×4）、GripFilter 要求取消、或历史触点禁止配对的配对。
void AccCheck(Matching& match, bool strict) {
    const int64_t accThold = AccThold();
    const uint8_t n = std::min<uint8_t>(std::max(PrevTouches().count, Touches().count), kMaxTouchNum);
    for (uint8_t p = 0; p < n; ++p) {
        const uint8_t c = match[p];
        if (c == kNoMatch) continue;
        const int32_t acc = GetAcc(c, p);
        const bool inc = IsToIncAccThold(c, p);
        if ((inc || strict ? accThold << 2 : accThold) < acc) {
            match[p] = kNoMatch;
            Static().specialControlFlags |= kSpecialAccTooLarge;
        } else if (GripFilter_IsToCancelIDMapping(c, p)) {
            match[p] = kNoMatch;
            Cur(c).gripFlags |= kGripIdMappingCancelled;
        }
        if ((Prev(p).persistFlags & kPersistNoIdMapping) != 0) match[p] = kNoMatch;
    }
}

// IDT_Map @ 0x6ba6122e
void ApplyIds(const Matching& match) {
    TouchTable& touches = Touches();
    for (uint8_t c = 0; c < touches.count; ++c) touches.unit[c].id = 0;
    const uint8_t n = std::min<uint8_t>(PrevTouches().count, kMaxTouchNum);
    for (uint8_t p = 0; p < n; ++p) {
        if (match[p] != kNoMatch) touches.unit[match[p]].id = Prev(p).id;
    }
}

}  // namespace

// IDT_Process @ 0x6ba611a6
void Process(bool strictAccCheck) {
    Matching match = Match();
    AccCheck(match, strictAccCheck);
    ApplyIds(match);
}

}  // namespace Solvers::Tsa::Core::IdTracking

extern "C" {
void IDT_Init(void) {}
void IDT_Process(uint8_t useStrictAccCheck) { Solvers::Tsa::Core::IdTracking::Process(useStrictAccCheck != 0); }
}  // extern "C"
