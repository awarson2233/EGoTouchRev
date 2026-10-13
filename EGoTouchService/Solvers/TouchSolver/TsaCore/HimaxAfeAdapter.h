#pragma once
// ══════════════════════════════════════════════════════════════════════
// HimaxAfeAdapter — 复刻 himax_thp_drv!thp_afe_get_frame 对 master 帧的预处理。
//
// TSACore 的参数表是在驱动处理后的数据上整定的：0x7ffe 零点、整帧倒序、
// 逐行 15% 死区、逐像素增益、偏移压缩。SPI 原始字直接喂进去会让阈值、
// 基线、峰值全部失配。
//
// 对应的原驱动函数：
//   thp_compute_checksum16                 master 帧校验
//   thp_afe_damp_repeated_master_frame     充电器插入时的帧间阻尼
//   thp_afe_preprocess_frame_data          逐行死区
//   thp_afe_get_frame (0x180035444..)      增益 + 压缩 + 倒序
//   thp_afe_process_stylus_status          status 位 / 当前扫描频率
// ══════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <cstdint>

#include "FrameLayout.h"

namespace Solvers::Tsa {

/// AFE_Frame.status（+0x34）的各个位。
namespace AfeStatus {
inline constexpr uint32_t kIdle          = 0x01;  // suffix word0 == 1
inline constexpr uint32_t kFrameValid    = 0x02;  // master 校验通过
inline constexpr uint32_t kFreqShiftDone = 0x04;  // 跳频完成
inline constexpr uint32_t kCalibDone     = 0x08;  // suffix word3 == 0xbb
inline constexpr uint32_t kChargerNoise  = 0x20;  // suffix word14 / word16 > 1499
inline constexpr uint32_t kSos           = 0x40;  // suffix word6 != 0
}  // namespace AfeStatus

struct AfeFrameInfo {
    bool     valid    = false;  // false：校验失败或帧太短，RawData() 保持上一帧内容
    uint32_t status   = 0;
    uint16_t scanFreq = 0;      // {75, 113}[suffix word10]
    uint16_t scanRate = 0;
};

class HimaxAfeAdapter {
public:
    /// TSACore 输入域的零点。
    static constexpr uint16_t kZero = 0x7ffe;
    static constexpr uint16_t kScanFreqTable[2] = {75, 113};
    static constexpr uint16_t kScanRate = 120;

    HimaxAfeAdapter() noexcept { Reset(); }

    void Reset() noexcept;

    /// thp_afe_set_charger：充电器插入时启用帧间阻尼，并换用另一组正向阈值 / 缩放。
    void SetChargerPlugged(bool plugged) noexcept;

    /// 宿主下发跳频命令后置位；suffix 报告完成且当前频率等于目标时 status |= kFreqShiftDone。
    void SetFreqShiftInFlight(bool inFlight, uint16_t targetFreq) noexcept {
        m_freqShiftInFlight = inFlight;
        m_freqShiftTarget = targetFreq;
    }

    /// 处理一帧 SPI master 数据（7B 头 + 4800B 矩阵 + 256B suffix）。
    AfeFrameInfo Build(const uint8_t* rawPtr, size_t rawLen) noexcept;

    /// AFE_Frame.pRawData：2400 个 uint16，kZero 为零点，下标已倒序。
    [[nodiscard]] const uint16_t* RawData() const noexcept { return m_out; }

    [[nodiscard]] uint32_t ConsecutiveChecksumErrors() const noexcept { return m_checksumErrors; }

private:
    static bool ValidateMasterChecksum(const uint8_t* rawPtr) noexcept;
    void DampRepeatedFrame(uint16_t* cells) noexcept;
    static void PreprocessRows(uint16_t* cells) noexcept;
    void ApplyGainAndCompress(const uint16_t* cells, bool slot1) noexcept;

    uint16_t m_out[Frame::kMatrixCells];       // g_RealTimeFrameBuffer
    uint16_t m_prev[Frame::kMatrixCells];      // g_PreviousMasterFrameCache
    uint8_t  m_dampMask[Frame::kMatrixCells];  // g_MasterDampingMaskBuffer
    bool     m_dampingEnabled = false;         // g_MasterFrameDampingEnable
    bool     m_freqShiftInFlight = false;      // freqShiftStatus
    uint16_t m_freqShiftTarget = 0;            // g_AfeCurrentFreqCommand
    uint16_t m_lastScanFreq = 0;               // g_FrameOut +0x30
    uint32_t m_checksumErrors = 0;             // g_HandChecksumErrorCount
};

}  // namespace Solvers::Tsa
