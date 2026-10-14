// 虚拟峰（TZ_VirtualPeak.c）：峰比上一帧触点少、而形状消抖认为是多指时，在峰外一圈找一个够强的格子补成峰。

#include "Alg/Peak/Peak.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchZone/TouchZoneInternal.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::TouchZone {

namespace {

constexpr uint8_t kVpMaxPrevMajorMm = 10;  // 历史触点长轴不到它才补
constexpr uint8_t kVpDenyFrames = 5;       // 连续这么多帧不满足就不再允许

struct VpCandidate {
    int16_t sig;
    uint8_t dim1;
    uint8_t dim2;
};

// TZ_IsVirtualPeakAllowed @ 0x6ba8d230
// 区域长轴明显长于（峰数 + 0.5）个短轴时允许；否则连续 5 帧不满足后不再允许。
bool IsVirtualPeakAllowed(const TouchZoneUnit& z) {
    TsaStatic& s = Static();
    const auto scale = static_cast<uint8_t>((z.peakCount + 1) * 8 - 4);
    if (z.minorMm * static_cast<uint32_t>(scale) < static_cast<uint32_t>(z.majorMm) << 3) {
        s.vpAllowed = 1;
        s.vpDenyFrames = 0;
    } else if (kVpDenyFrames - 1 < ++s.vpDenyFrames) {
        s.vpAllowed = 0;
        s.vpDenyFrames = kVpDenyFrames;
    }
    return s.vpAllowed != 0;
}

// TZ_SearchNxN @ 0x6ba8d53e
// 峰周围 5×5 的外圈里，Dif 达到门限、比已选的更强、且 3×3 内没有峰的格子。
bool SearchNxN(uint8_t dim1, uint8_t dim2, int16_t thold, VpCandidate& best) {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    bool found = false;
    for (uint8_t a = 0; a < 5; ++a) {
        for (uint8_t b = 0; b < 5; ++b) {
            if (a != 0 && a != 4 && b != 0 && b != 4) continue;
            const auto d1 = static_cast<uint8_t>(dim1 + a - 2);
            const auto d2 = static_cast<uint8_t>(dim2 + b - 2);
            if (d1 >= fl.dim1Len || d2 >= fl.dim2Len) continue;
            const int16_t sig = dif[GridIndex(d1, d2)];
            if (sig < thold || sig <= best.sig || Peak::IsInNxNBox(d1, d2, 3)) continue;
            found = true;
            best = {sig, d1, d2};
        }
    }
    return found;
}

// TZ_TryInsertVPA @ 0x6ba8d6c4：在所有峰的外圈里取最强的候选格补成峰。
bool TryInsertVpa(TouchZoneUnit& z) {
    VpCandidate best{};
    // TZ_GetSigTholdForVPA @ 0x6ba8d519
    const int16_t thold = PrmtDynamic().sigThold;
    bool found = false;
    for (uint8_t k = 0; k < z.peakCount; ++k) {
        const PeakEntry& p = Peaks().entry[z.peakIdx[k]];
        if (SearchNxN(p.dim1, p.dim2, thold, best)) found = true;
    }
    return found && InsertVirtualPeak(z, best.dim1, best.dim2);
}

}  // namespace

// TZ_VPInit @ 0x6ba8d320
void VpInit() {
    Static().vpAllowed = 0;
    Static().vpDenyFrames = 0;
}

// TZ_VPProcess @ 0x6ba8d348
// 峰比上一帧触点少、形状消抖为多指、区域无异常不贴边、对应的历史触点不大时尝试补虚拟峰。
void VpProcess(TouchZoneUnit& z) {
    const uint8_t prevMajorMm = z.prevIdx != kNoTouch ? static_cast<uint8_t>(PrevTouches().unit[z.prevIdx].majorMm) : 0;
    if (PrevTouches().count <= Peaks().count) return;
    if (GetDbShape(z) != kShapeMultiFinger || Prpt().touchMode != Prpt().prevTouchMode || z.exceptionType != 0) return;
    if (PrevTouches().exceptionFlags != 0 || (z.flags & kZoneOnBorder) != 0 || kVpMaxPrevMajorMm <= prevMajorMm) return;
    if ((PrmtFlash().gripFlags & kGripNoVirtualPeak) != 0) return;
    if (IsVirtualPeakAllowed(z)) TryInsertVpa(z);
}

// TZ_InsertVirtualPeak @ 0x6ba8d460
bool InsertVirtualPeak(TouchZoneUnit& z, uint8_t dim1, uint8_t dim2) {
    const auto idx = static_cast<uint8_t>(Peak::Insert(dim1, dim2));
    if (idx == kNoPeak) return false;
    z.flags |= kZoneVirtualPeak;
    Peaks().entry[idx].flags |= kPeakVirtual;
    PeakRegister(z, idx);
    return true;
}

}  // namespace Solvers::Tsa::Core::TouchZone
