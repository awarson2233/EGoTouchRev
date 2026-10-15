#include "Alg/Exception/Exception.h"

#include <cstddef>
#include <cstring>

#include "Alg/Baseline/Baseline.h"
#include "Alg/Exception/FiException.h"
#include "Alg/Touch/TouchData.h"
#include "Alg/TouchZone/TouchZoneData.h"
#include "App/Afe.h"
#include "App/Report.h"
#include "App/Sys.h"
#include "App/TsaBuffer.h"
#include "App/TsaStatic.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Exception {

namespace {

constexpr uint64_t kNoiseSettleLimit = 0x52;      // noiseSettleCount 超过它时噪声等级清零，计数封顶于它加一
constexpr uint16_t kDebounceNoException = 300;    // 回到无异常要连续检出的帧数
constexpr uint16_t kDebounceException = 10;       // 进入其它异常要连续检出的帧数
constexpr uint32_t kChangedControlFlag = 0x80;    // 全局异常字变化时置于 TsaStatic::specialControlFlags
constexpr uint8_t kZTholdGlove = 0x04;            // DynamicPrmt::zTholdFlags 的手套位

bool IsFullIncell() { return PrmtFlash().sensorArchitecture == kFullIncell; }

// Exception_InitVariable @ 0x6ba58aaa
ExceptionState& State() { return IsFullIncell() ? Static().fiException : Static().hiException; }

// Exception_GetWaterPrpt @ 0x6ba5882e
// 先求差分（fromBaseline 时为 Bl 相对 BlNormal，否则为 Raw 相对 Bl）写进 Dif，再数 Dif、-Dif
// 各有多少格落在 [underWaterTholdLow, underWaterThold] 内。
void CountWaterCells(bool fromBaseline) {
    TsaBufferTable& buf = Buffers();
    if (fromBaseline) Baseline::CalcDiffCommon(buf.dif, buf.bl, buf.blNormal);
    else Baseline::CalcDiffCommon(buf.dif, buf.raw, buf.bl);
    const FlashPrmt& fl = PrmtFlash();
    const DynamicPrmt& dyn = PrmtDynamic();
    int16_t pos = 0;
    int16_t neg = 0;
    uint16_t cell = 0;
    for (uint8_t r = 0; r < fl.dim2Len; ++r) {
        for (uint8_t c = 0; c < fl.dim1Len; ++c, ++cell) {
            const int v = buf.dif[cell];
            if (dyn.underWaterTholdLow <= v && v <= dyn.underWaterThold) ++pos;
            if (dyn.underWaterTholdLow <= -v && -v <= dyn.underWaterThold) ++neg;
        }
    }
    Prpt().underWaterCells = pos;
    Prpt().underWaterNegCells = neg;
}

// Exception_DebounceProcess @ 0x6ba58b9e
// 检出的异常与上一帧相同时计数：回到无异常要连续 300 帧，其它异常 10 帧；未达到前沿用上一帧的全局异常。
void Debounce(ExceptionState& st, uint32_t detected) {
    TouchTable& cur = Touches();
    const PrevTouchTable& prev = PrevTouches();
    if (st.lastDetected != detected) {
        cur.exceptionFlags = prev.exceptionFlags;
        st.debounceFrames = 0;
        return;
    }
    ++st.debounceFrames;
    if (st.debounceFrames >= (detected == 0 ? kDebounceNoException : kDebounceException)) {
        cur.exceptionFlags = detected;
        st.debounceFrames = 0;
    } else {
        cur.exceptionFlags = prev.exceptionFlags;
    }
}

// Exception_UpdatePrevException @ 0x6ba58c5c
// 本帧的全局异常：噪声、水下（以及 Full Incell 下的基线重置）立即生效并保持；刚退出特殊控制模式时
// 加上 kExitedSpecialCtrl；上一帧带 0x200 / 0x1000 时清零；其余按检出结果消抖。
void UpdateGlobal(ExceptionState& st) {
    TouchTable& cur = Touches();
    const PrevTouchTable& prev = PrevTouches();
    const uint32_t detected = st.detected;
    const auto force = [&](uint32_t exception) {
        cur.exceptionFlags = exception;
        st.lastDetected = exception;
    };
    if (detected == kNoiseLevel2 || prev.exceptionFlags == kNoiseLevel2) return force(kNoiseLevel2);
    if (detected == kNoiseLevel3) return force(kNoiseLevel3);
    if (detected == kUnderWater || prev.exceptionFlags == kUnderWater) return force(kUnderWater);
    if (detected == kUnderWaterReverted) return force(kUnderWaterReverted);
    if (Baseline::ResetStatus() == 1 && IsFullIncell()) return force(kAbnormalBaseline);
    if (Static().justExitedSpecialCtrlMode != 0) {
        cur.exceptionFlags |= kExitedSpecialCtrl;
        st.lastDetected = kExitedSpecialCtrl;
        return;
    }
    if ((prev.exceptionFlags & (kNoiseLevel2 | kAbnormalBaseline | kExitedSpecialCtrl)) != 0) {
        cur.exceptionFlags = 0;
        st.debounceFrames = 0;
    } else if (detected == prev.exceptionFlags) {
        cur.exceptionFlags = prev.exceptionFlags;
        st.debounceFrames = 0;
    } else if (prev.exceptionFlags != 0 && prev.count != 0) {
        // 有触点时异常不变，只允许 0x40 升级为湿手指
        cur.exceptionFlags = detected == kWetFinger && prev.exceptionFlags == 0x40 ? kWetFinger : prev.exceptionFlags;
        st.debounceFrames = 0;
    } else {
        Debounce(st, detected);
    }
    st.lastDetected = detected;
}

/// 原代码按字节块拷贝：from 处 n 字节写到 to 处。
template <class To, class From>
void CopyBytes(To& to, size_t toOffset, const From& from, size_t fromOffset, size_t n) {
    std::memcpy(reinterpret_cast<uint8_t*>(&to) + toOffset, reinterpret_cast<const uint8_t*>(&from) + fromOffset, n);
}

// 无异常（或只有 0x400）时从机型参数恢复手套相关的动态参数（zTholdFlags、gloveFilterSides、
// distToEnterGloveMode 两块），否则关掉 zTholdFlags 的手套位。
void UpdateGloveParams(uint32_t exception) {
    DynamicPrmt& dyn = PrmtDynamic();
    if (exception == 0 || exception == 0x400) {
        const FlashPrmt& fl = PrmtFlash();
        static_assert(offsetof(DynamicPrmt, gloveFilterSides) + 4 - offsetof(DynamicPrmt, zTholdFlags) == 8);
        static_assert(offsetof(FlashPrmt, gloveFilterSides) + 4 - offsetof(FlashPrmt, zTholdFlags) == 8);
        CopyBytes(dyn, offsetof(DynamicPrmt, zTholdFlags), fl, offsetof(FlashPrmt, zTholdFlags), 8);
        CopyBytes(dyn, offsetof(DynamicPrmt, distToEnterGloveMode), fl, offsetof(FlashPrmt, distToEnterGloveMode), 4);
    } else {
        dyn.zTholdFlags &= static_cast<uint8_t>(~kZTholdGlove);
    }
}

}  // namespace

// Exception_Malloc @ 0x6ba587c0
bool Malloc() { return FiException::Malloc(); }

// Exception_Free @ 0x6ba587e3
void Free() { FiException::Free(); }

// Exception_Init @ 0x6ba587f7（Hybrid Incell 的 HIException_Init 被裁掉，为空）
void Init() {
    if (IsFullIncell()) FiException::Init();
}

// Exception_CheckUnderWater @ 0x6ba589c1
// systemFlags bit0 清、bit2 置时按基线求差分，否则按原始数据，并且信号反向时记为 kUnderWaterReverted。
// 两组状态都记。
void CheckUnderWater() {
    const uint32_t flags = Static().systemFlags;
    uint32_t detected = 0;
    if ((flags & 1) == 0 && (flags & 4) != 0) {
        CountWaterCells(true);
        if (Report::IsPossibleUnderWater()) detected = kUnderWater;
    } else {
        CountWaterCells(false);
        if (Report::IsPossibleUnderWater()) detected = Sys::IsSignalReverted() != 0 ? kUnderWaterReverted : kUnderWater;
    }
    if (detected != 0) {
        Static().hiException.detected = detected;
        Static().fiException.detected = detected;
    }
}

// Exception_ProcessTouchZone @ 0x6ba591d1（Hybrid Incell 的 HIException_ProcessTouchZone 被裁掉，为空）
void ProcessTouchZone(TouchZoneUnit& zone) {
    if (IsFullIncell()) FiException::ProcessTouchZone(zone);
}

// Exception_Process @ 0x6ba58f25
// 上一帧的噪声异常在手动跳频且未完成时保留，否则清掉；上一帧水下而本帧检出信号反向的水下时也清掉。
// 然后更新本帧全局异常，据此设噪声等级、field110 与手套参数，清空本帧检出的异常；全局异常变化时
// 置 specialControlFlags 的 kChangedControlFlag。
void Process() {
    ExceptionState& st = State();
    PrevTouchTable& prev = PrevTouches();
    if (prev.exceptionFlags == kNoiseLevel2 &&
        !(Afe::IsManualFreqHopping() && !Sys::IsFreqShiftDone())) {
        prev.exceptionFlags = 0;
    }
    if (prev.exceptionFlags == kUnderWater && st.detected == kUnderWaterReverted) prev.exceptionFlags = 0;
    UpdateGlobal(st);

    const uint32_t exception = Touches().exceptionFlags;
    TsaStatic& stat = Static();
    stat.field110 = exception == 0x40 || exception == kWetFinger ? 1 : 0;
    if (exception == kNoiseLevel2) {
        stat.noiseLevel = 2;
        st.noiseSettleCount = 0;
    } else if (exception == kNoiseLevel3) {
        stat.noiseLevel = 3;
        st.noiseSettleCount = 0;
    } else {
        st.noiseSettleCount += static_cast<uint64_t>(static_cast<int64_t>(stat.frameIntervalMs));
        if (st.noiseSettleCount > kNoiseSettleLimit) {
            stat.noiseLevel = 0;
            st.noiseSettleCount = kNoiseSettleLimit + 1;
        }
    }
    UpdateGloveParams(exception);
    st.detected = 0;
    if (Touches().exceptionFlags != PrevTouches().exceptionFlags) Static().specialControlFlags |= kChangedControlFlag;
}

}  // namespace Solvers::Tsa::Core::Exception
