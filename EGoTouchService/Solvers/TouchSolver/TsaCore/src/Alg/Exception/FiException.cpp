#include "Alg/Exception/FiException.h"

#include <cstddef>
#include <cstring>

#include "Alg/Exception/Exception.h"
#include "Alg/Peak/PeakData.h"
#include "Alg/Touch/PrevTouch.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchZone/TouchZoneData.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Common/Image.h"
#include "Legacy/RawApi.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::FiException {

namespace {

constexpr uint16_t kAbnormalBaselineRatio = 200;  // exceptionRatio[0] 超过它（且区域吃掉全帧的峰）判基线异常
constexpr uint32_t kMinAxisRatioX10 = 20;         // 湿手指要求长轴 / 短轴 ≥ 2
constexpr uint32_t kNoWetFingerFlag = 0x20;       // TouchZoneUnit::flags 带这一位的区域不判湿手指
constexpr int kQueueSize = 16;

ExceptionState& State() { return Static().fiException; }

/// g_reached：本帧数水膜格时已经数过的格。
uint8_t*& Reached() { return Image::At<uint8_t*>(0x6bb806d0); }

// 原代码不检查历史下标：为 kNoTouch 以外的越界值时照样按下标读，落在历史表之后的内存上。
const PrevTouchUnit& Prev(uint8_t idx) { return PrevTouches().unit[idx]; }

/// 历史轨迹队列第 slot 项。原代码按字节下标直接取，slot 超出 16 时读到队列之后的字段。
uint8_t QueueAt(const PrevTouchUnit& unit, size_t field, uint8_t slot) {
    return reinterpret_cast<const uint8_t*>(&unit)[field + slot];
}

// FIException_Recording @ 0x6ba5943a
// 本帧检出的异常取最大值；2 不记。
void Record(uint32_t exception) {
    ExceptionState& st = State();
    if (st.detected < exception && exception != 2) st.detected = exception;
}

// FIException_CheckAbnormalTouchZoneBaseline @ 0x6ba595ee
bool IsAbnormalBaseline(const TouchZoneUnit& zone) {
    return zone.peakCount == Peaks().count && zone.exceptionRatio[0] > kAbnormalBaselineRatio;
}

// FIException_IsAroundSmall @ 0x6ba59639
// (dim1, dim2) 周围 (2·radius+1)² 里落在图内的格，Dif 都不超过 high。
bool IsAroundSmall(int dim1, int dim2, int radius, int16_t high) {
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const int c1 = dim1 + dx;
            const int c2 = dim2 + dy;
            if (static_cast<uint8_t>(c1) < fl.dim1Len && static_cast<uint8_t>(c2) < fl.dim2Len &&
                high < dif[static_cast<uint16_t>(c1 + fl.dim1Len * c2)]) {
                return false;
            }
        }
    }
    return true;
}

// FIException_GetWaterSize @ 0x6ba59733
// 轨迹第 slot 项那一格的 3×3 邻域里，找第一个没数过、Dif 在 [low, high] 内、周围也都不超过 high 的格：
// 标为已数，计数加一。行列先按 int16 相加，再按 8 位判断是否在图内。
int16_t CountWaterCell(uint8_t prevIdx, uint8_t slot, int16_t count, int16_t low, int16_t high) {
    const PrevTouchUnit& unit = Prev(prevIdx);
    const uint8_t base1 = QueueAt(unit, offsetof(PrevTouchUnit, queueDim1), slot);
    const uint8_t base2 = QueueAt(unit, offsetof(PrevTouchUnit, queueDim2), slot);
    const FlashPrmt& fl = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    uint8_t* reached = Reached();
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const auto c1 = static_cast<int16_t>(dx + base1);
            const auto c2 = static_cast<int16_t>(dy + base2);
            if (static_cast<uint8_t>(c1) >= fl.dim1Len || static_cast<uint8_t>(c2) >= fl.dim2Len) continue;
            const auto cell = static_cast<int16_t>(c1 + fl.dim1Len * c2);
            if (reached[cell] != 0) continue;
            const int16_t v = dif[cell];
            if (v > high || v < low || !IsAroundSmall(c1, c2, 1, high)) continue;
            reached[cell] = 1;
            return static_cast<int16_t>(count + 1);
        }
    }
    return count;
}

// FIException_CheckWetFinger @ 0x6ba59919
// 区域对应的历史触点先把当前位置推进轨迹队列，再沿整条轨迹数水膜格（Dif 在 sigPeakPositive 的
// 两个百分比之间、周围没有更大信号的格）。水膜格够多、形状够长、且区域没有 kNoWetFingerFlag 时判湿手指。
bool IsWetFinger(TouchZoneUnit& zone) {
    const uint8_t prevIdx = zone.prevIdx;
    if (prevIdx >= kNoTouch) return false;
    if (static_cast<uint8_t>(TSAStatic_IsProximityEnabled()) != 0) return false;
    const PrevTouchUnit& unit = Prev(prevIdx);
    PrevTouch::HistoryQueuePush(prevIdx, unit.iirPos.dim1, unit.iirPos.dim2);

    const FlashPrmt& fl = PrmtFlash();
    const int16_t sig = Prpt().sigPeakPositive;
    if (sig < fl.fingerThold) return false;
    std::memset(Reached(), 0, GridBufSize());
    const auto low = static_cast<int16_t>(sig * fl.waterSigLowPercent / 100);
    const auto high = static_cast<int16_t>(sig * fl.waterSigHighPercent / 100);
    uint16_t water = 0;
    for (int slot = unit.queueHead; unit.queueTail != slot; slot = (slot + 1) % kQueueSize) {
        water = static_cast<uint16_t>(CountWaterCell(prevIdx, static_cast<uint8_t>(slot), static_cast<int16_t>(water), low, high));
    }
    zone.waterCells = water;

    const uint32_t axisRatioX10 = zone.minorMm != 0 ? zone.majorMm * 10u / zone.minorMm : 0;
    return water >= fl.wetFingerMinWaterCells && axisRatioX10 >= kMinAxisRatioX10 && (zone.flags & kNoWetFingerFlag) == 0;
}

// FIException_DebounceTouchZone @ 0x6ba59471
// 区域异常参照上一帧的全局异常与对应历史触点的区域异常收敛：
// - 全局为 8：区域或历史触点有异常时，本帧没有湿手指或 field005 > 2 则区域取 8，否则反过来把全局改成区域异常；
// - 全局无 0x40 / 0x80 位时，区域异常 2 跟随历史触点的 0x40 / 0x80（取湿手指）或 8；
// - 全局带 0x40 / 0x80 位时，区域或历史触点有异常就取全局异常。
// 最后 field005 > 4 时有异常的区域一律取 8。
void Debounce(TouchZoneUnit& zone) {
    PrevTouchTable& prev = PrevTouches();
    uint32_t exception = zone.exceptionType;
    const auto prevException = zone.prevIdx != kNoTouch ? static_cast<uint32_t>(Prev(zone.prevIdx).exceptionType) : 0u;
    const uint32_t global = prev.exceptionFlags;
    const bool any = exception != 0 || prevException != 0;
    if ((global & 0xc0) != 0) {
        if (any) exception = global;
    } else if (global == 8) {
        if (any) {
            if (State().wetFinger == 0 || prev.field005 > 2) exception = 8;
            else prev.exceptionFlags = exception;
        }
    } else if (exception == 2) {
        if (prevException == 0x40 || prevException == Exception::kWetFinger) exception = Exception::kWetFinger;
        else if (prevException == 8) exception = 8;
    }
    if (prev.field005 > 4 && exception != 0) exception = 8;
    zone.exceptionType = exception;
}

}  // namespace

// FIException_Malloc @ 0x6ba59360
bool Malloc() {
    Reached() = static_cast<uint8_t*>(TSA_MallocAndInit(GridBufSize()));
    return Reached() != nullptr;
}

// FIException_Free @ 0x6ba593a1
void Free() { TSA_Free(reinterpret_cast<int64_t*>(&Reached())); }

// FIException_Init @ 0x6ba593c2：逐个字段清零，填充字节不动。
void Init() {
    ExceptionState& st = State();
    st.debounceFrames = 0;
    st.noiseSettleCount = 0;
    st.lastDetected = 0;
    st.wetFinger = 0;
    st.detected = 0;
}

// FIException_ProcessTouchZone @ 0x6ba59bdb
// 全局已检出噪声时区域直接记为噪声；否则依次判基线异常、湿手指，再消抖。区域异常并入本帧检出的异常。
void ProcessTouchZone(TouchZoneUnit& zone) {
    State().wetFinger = 0;
    TZ_GetExceptionRatio(&zone);
    if (State().detected == Exception::kNoiseLevel2) {
        zone.exceptionType |= Exception::kNoiseLevel2;
        zone.detectedException = Exception::kNoiseLevel2;
        zone.noiseException = 1;
    } else {
        if (IsAbnormalBaseline(zone)) {
            zone.exceptionType |= Exception::kAbnormalBaseline;
            zone.detectedException = Exception::kAbnormalBaseline;
            zone.abnormalBaseline = 1;
        } else if (IsWetFinger(zone)) {
            State().wetFinger = 1;
            zone.exceptionType |= Exception::kWetFinger;
            zone.detectedException = Exception::kWetFinger;
        }
        Debounce(zone);
    }
    Record(zone.exceptionType);
}

}  // namespace Solvers::Tsa::Core::FiException
