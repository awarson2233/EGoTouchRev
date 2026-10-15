#pragma once
// TouchZone 各源文件之间共用的状态访问与函数（不对外）。

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Alg/Peak/PeakData.h"
#include "Alg/SelfData/SelfData.h"
#include "Alg/TouchZone/TouchZoneData.h"
#include "App/TsaBuffer.h"
#include "Common/Assert.h"
#include "Common/Image.h"

namespace Solvers::Tsa::Core::TouchZone {

/// FlashPrmt::gripFlags
inline constexpr uint32_t kGripForceFinger = 0x001;        // 区域形状一律按手指
inline constexpr uint32_t kGripNoVirtualPeak = 0x002;
inline constexpr uint32_t kGripFixedSigThold = 0x004;      // 区域生长门限取 gripModeSigThold
inline constexpr uint32_t kGripRegisterDirect = 0x008;     // 区域不判形状、不拆分，直接登记
inline constexpr uint32_t kGripLoByZoneAxis = 0x010;       // 大物体面积按区域自身的椭圆算（否则按最大子区域）
inline constexpr uint32_t kGripNoZoneException = 0x020;
inline constexpr uint32_t kGripSignedSig = 0x040;          // 生长按带符号的信号比门限（互容主流程中只在贴边两列这样做）
inline constexpr uint32_t kGripCentroidOnly = 0x080;       // 不生长，只在峰周围 3×3 求质心
inline constexpr uint32_t kGripSimpleTraversal = 0x100;
inline constexpr uint32_t kGripAge = 0x200;                // 维护握持区域年龄，年龄悬殊的贴边格不并入
inline constexpr uint32_t kGripCentroidSigOffset = 0x400;  // 质心权重先减去 centroidSigOffset

/// PeakEntry::flags
inline constexpr uint32_t kPeakVirtual = 0x100;
inline constexpr uint32_t kPeakInGrip = 0x400;
inline constexpr uint32_t kPeakNotInGrip = 0x800;

inline constexpr uint8_t kNoPeak = kMaxPeakNum;

/// g_pkAccessedFlag / g_pkBeforeSDCAccessedFlag：已归入某个区域的峰（峰表下标的位图）。
[[nodiscard]] inline uint32_t& PeakAccessed() noexcept { return Image::At<uint32_t>(0x6bc139d4); }
[[nodiscard]] inline uint32_t& SdcPeakAccessed() noexcept { return Image::At<uint32_t>(0x6bc139d0); }
/// g_tzBuffer：生长时遍历到的格子的 Dif，轮廓插值的输入。
[[nodiscard]] inline int16_t*& ZoneSig() noexcept { return Image::At<int16_t*>(0x6bb80fc0); }
/// g_peaksBeforeSwitchDifBuffer：切换 Dif 前的峰表。
[[nodiscard]] inline PeakTable& PeaksBeforeSwitchDif() noexcept { return Image::At<PeakTable>(0x6bbc0b40); }
/// g_tsaBufTimeHand：每格自手掌按下以来的时间。
[[nodiscard]] inline int16_t* TimeHand() noexcept { return Image::At<int16_t*>(0x6bba3518); }
/// g_tsaBufSelf 里 TZ 借用的两段（每个区域重新清零），下标 dim1，或 dim1Len + dim2：
/// 每列 / 行遍历到的信号和，与未达门限的格数。
[[nodiscard]] inline int32_t* LineSigSum() noexcept { return SelfBuf().field1400; }
[[nodiscard]] inline int32_t* LineWeakCount() noexcept { return SelfBuf().field1900; }
inline constexpr size_t kLineBufBytes = 0x280;

[[nodiscard]] inline int16_t Abs16(int16_t v) noexcept { return static_cast<int16_t>(v < 0 ? -v : v); }
/// 区域图里的编码：当前句柄的 tag + kind，带 Dif 的符号。
[[nodiscard]] inline int8_t MapCode(int kind, int16_t dif) noexcept {
    return static_cast<int8_t>((CurrentAccess()->tag + kind) * (dif < 0 ? -1 : 1));
}
/// 区域图里该格的编码去掉 tag 后的格子类别（带 Dif 的符号还原）。
[[nodiscard]] inline int CellKind(uint16_t cell) noexcept {
    const int sign = Buffers().dif[cell] < 0 ? -1 : 1;
    return TzAccess().map[cell] * sign - CurrentAccess()->tag;
}
// StateMachine.cpp
void UnitInit(TouchZoneUnit& z);
void VirtualPeakInfoRetrieval();

// Traversal.cpp
void PeakBasedTraversal(TouchZoneUnit& z, int16_t thold, uint8_t dim1, uint8_t dim2, bool centroidSigOffset);
void PeakBasedTraversalSimple(TouchZoneUnit& z, int16_t thold, uint8_t dim1, uint8_t dim2);
void PeakFilter(TouchZoneUnit& z);

// Common.cpp
void CentroidCore(TouchZoneUnit& z);
void GetAxis(TouchZoneUnit& z);
void GetEdgeTouchedFlag(TouchZoneUnit& z);
void GetHtiValidFlag(TouchZoneUnit& z);
void GetGripRatio(TouchZoneUnit& z);
void GetShapeRatio(TouchZoneUnit& z);
void GetShapeFlag(TouchZoneUnit& z);
void GetSurroundingStatusFlag(TouchZoneUnit& z);
void GetCmfFlag(TouchZoneUnit& z);
void PeakRegister(TouchZoneUnit& z, uint8_t peak);
void GetMaxMinPkIdx(TouchZoneUnit& z);
void Get2ClosestPeaks(const TouchZoneUnit& z, uint8_t& a, uint8_t& b);
void UnitToTouch(TouchZoneUnit& z);

// Shape.cpp
void GetType(TouchZoneUnit& z);
void GetDfsPrpt(TouchZoneUnit& z);
[[nodiscard]] uint32_t GetDbShape(const TouchZoneUnit& z);

// GripMerge.cpp
void CheckToSeparateZone(TouchZoneUnit& z);
void GetPeakGripFlag(TouchZoneUnit& z);

// VirtualPeak.cpp
void VpInit();
void VpProcess(TouchZoneUnit& z);
bool InsertVirtualPeak(TouchZoneUnit& z, uint8_t dim1, uint8_t dim2);

// Contour.cpp
void ContourInit();
void GetContour(TouchZoneUnit& z);

}  // namespace Solvers::Tsa::Core::TouchZone
