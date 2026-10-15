#pragma once

#include <cstdint>

namespace Config {

// ConfigKeyId: 紧凑的 16-bit 配置键 ID。
// 布局: high byte = 模块分区, low byte = 模块内序号
//
//   Service:    0x00xx
//   Touch:      0x01xx
//   Stylus:     0x02xx
//   Reserved:   0x03xx

enum class ConfigKeyId : uint16_t {
    // ── Service (0x0000-0x00FF) ──
    SvcMode              = 0x0000,
    SvcAutoMode          = 0x0001,
    SvcStylusVhfEnabled  = 0x0002,
    SvcPenButtonMode     = 0x0003,
    SvcPenButtonRoute    = 0x0004,

    // ── Touch (0x0100-0x01FF)：整个分区已退役 ──
    // 手指管线的每一个参数都取自 TSACore 按面板机型装入的 Flash 整定表，不接受上位机
    // 覆盖，因此这里不再定义任何键。该 ID 段保留不复用，避免与旧版本 IPC 快照撞号。

    // ── Stylus: reserved legacy HPP2 config key range (0x0200-0x022F) ──

    // ── Stylus: SP (0x0230-0x027F) ──
    StylusSpFrameParserEnabled            = 0x0230,
    StylusSpPeakDetectorEnabled           = 0x0231,
    StylusSpCoordinateSolverEnabled       = 0x0232,
    StylusSpTiltProcessEnabled            = 0x0233,
    StylusSpPressureSolverEnabled         = 0x0234,
    StylusSpPostPressureEnabled           = 0x0235,
    StylusSpFakePressureDecreaseEnabled   = 0x0236,
    StylusSpBtFreqShiftDebounceFrames     = 0x0237,
    StylusSpPressureEdgeEnterThreshold    = 0x0238,
    StylusSpPressureEdgeExitThreshold     = 0x0239,
    StylusSpTipDownPressureThreshold      = 0x023A,
    StylusSpBtPressSuppressEnterThreshold = 0x023B,
    StylusSpBtPressSuppressExitThreshold  = 0x023C,
    StylusSpSignalFloor                   = 0x023D,
    StylusSpEdgeCoorEnabled               = 0x023E,
    StylusSpEdgeCoorPostEnabled           = 0x023F,
    StylusSpNoisePostEnabled              = 0x0240,
    StylusSpNoiseSignalRatioThold         = 0x0241,
    StylusSpNoiseSignalDropRatio          = 0x0242,
    StylusSpLinearFilterEnabled           = 0x0243,
    StylusSpCoorReviseEnabled             = 0x0244,
    StylusSpCoorReviseFactorDim1          = 0x0245,
    StylusSpCoorReviseFactorDim2          = 0x0246,
    StylusSpCoorSpeedEnabled              = 0x0247,
    StylusSpIirFilterEnabled              = 0x0248,
    // Historical TLV IDs retained; names corrected from InBand/Edge to Hover/Writing.
    StylusSpIirCoefLowHover               = 0x0249,
    StylusSpIirCoefHighHover              = 0x024A,
    StylusSpIirSpeedTholdHover            = 0x024B,
    StylusSpIirCoefLowWriting             = 0x024C,
    StylusSpIirCoefHighWriting            = 0x024D,
    StylusSpIirSpeedTholdWriting          = 0x024E,
    StylusSpIirSpeedMax                   = 0x024F,
    StylusSpIirMaxCoef                    = 0x0250,
    StylusSpAftCoorEnabled                = 0x0251,
    StylusSpLockFlashInBandX              = 0x0252,
    StylusSpLockFlashInBandY              = 0x0253,
    StylusSpLockFlashEdgeX                = 0x0254,
    StylusSpLockFlashEdgeY                = 0x0255,
    StylusSpLockSensorTxCount             = 0x0256,
    StylusSpLockSensorRxCount             = 0x0257,
    StylusSpLockBypass                    = 0x0258,

    // ── Sentinel ──
    MaxKeyId = 0x0300,
};

} // namespace Config
