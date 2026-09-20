#pragma once
// ── TouchPipeline Stage 0: MasterFrameParser ──────────────────────────
// Header-only. Converted from Preprocessing/MasterFrameParser.{h,cpp}.
// Fast zero-copy parser for SPI raw frame:
//   7B header + 4800B mutual matrix + 256B master suffix + 332B slave suffix
// Decodes raw LE bytes -> frame.heatmapMatrix and (if mounted) bigFrame.raw,
// and extracts hardware status & finger-presence flags from MasterSuffix.
//
// 线上格式：互容矩阵的每个 16 位字是**偏移二进制 (offset binary)**——0x8000 表示
// 零，而不是二进制补码。对应 TSACore 的 Rawdata_NormalizeGridUnit：
// `bRawdataForceNormalize == 0` 时它把线上字当作 **unsigned** 取用
// (`local_c = (uint)param_1`)，只有该标志置位才按 `(short)param_1` 解读。
// 偏移二进制与补码之间正好差一个符号位，因此转换就是 `val ^ 0x8000`。
// 若照补码直接解读，每个格子的符号位都会被翻转：真实的 +2862 会变成 −32762、
// 真实的 −102 会变成 +32666，整屏退化成 ±32700 的随机噪声，后级基线/共模/峰值
// 全部失效。
// ─────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <cstring>

#include "SolverTypes.h"
#include "FrameLayout.h"
#include "TouchCoreTypes.hpp"

namespace Solvers::Touch {

class MasterFrameParser {
public:
    bool m_enabled = true;

    /// 线上互容字是否为偏移二进制 (0x8000 = 零)。W273AS2700 的 Himax MCU 是，
    /// 对应 TSACore 的 `bRawdataForceNormalize == 0` 分支（按 unsigned 取用）。
    /// 置 false 则按二进制补码直接解读，对应 `bRawdataForceNormalize != 0`。
    bool m_offsetBinaryInput = true;

    /// 偏移二进制 → 二进制补码：两者只差一个符号位
    [[nodiscard]] inline int16_t DecodeCell(uint16_t wire) const noexcept {
        return static_cast<int16_t>(m_offsetBinaryInput ? (wire ^ 0x8000u) : wire);
    }

    // ── Parameter & Control Interface ─────────────────────────────────
    void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }

    /// Primary entry point: Process HeatmapFrame
    /// Decodes SPI raw bytes into frame.heatmapMatrix and touch.bigFrame.raw
    inline bool Process(HeatmapFrame& frame) {
        frame.masterSuffixValid = false;
        frame.slaveSuffixValid = false;
        if (frame.touch.bigFrame != nullptr) {
            frame.touch.bigFrame->masterSuffixValid = false;
            frame.touch.bigFrame->slaveSuffixValid = false;
            frame.touch.bigFrame->hasFinger = false;
        }

        if (!m_enabled) {
            // Parser disabled: pass-through / debug bypass
            return true;
        }

        // Master 帧: 7B header + 4800B matrix + 256B suffix = 5063B
        if (frame.rawLen < Frame::kMasterFrameSize || frame.rawPtr == nullptr) {
            // Short/incomplete frame: zero out matrix buffers
            std::memset(frame.heatmapMatrix, 0, sizeof(frame.heatmapMatrix));
            if (frame.touch.bigFrame != nullptr) {
                std::memset(frame.touch.bigFrame->raw, 0, sizeof(frame.touch.bigFrame->raw));
            }
            return true;
        }

        const uint8_t* raw_ptr = frame.rawPtr + Frame::kMatrixOffset;
        auto* bigFrame = frame.touch.bigFrame;

        // 使用标准 C++ 循环并确保小端非对齐内存加载安全 (ARM64 / x64 NEON/AVX 友好)
        for (int r = 0; r < Frame::kTxCount; ++r) {
            for (int c = 0; c < Frame::kRxCount; ++c) {
                const int i = r * Frame::kRxCount + c;
                const uint16_t val = static_cast<uint16_t>(raw_ptr[i * 2]) |
                                     (static_cast<uint16_t>(raw_ptr[i * 2 + 1]) << 8);
                const int16_t sval = DecodeCell(val);

                frame.heatmapMatrix[r][c] = sval;
                if (bigFrame != nullptr) {
                    bigFrame->raw[r][c] = sval;
                }
            }
        }

        // 解析 Master Suffix (256 字节状态表)
        frame.masterSuffix.LoadFromBytes(frame.rawPtr + Frame::kMasterSuffixOffset);
        frame.masterSuffixValid = true;

        if (bigFrame != nullptr) {
            bigFrame->masterSuffixValid = true;
            bigFrame->hasFinger = frame.masterSuffix.hasFinger();
            bigFrame->timestampUs = frame.timestamp;
        }

        // 若包含 Slave 帧 (手写笔数据)，解析 Slave Suffix
        if (frame.rawLen >= Frame::kTotalFrameSize) {
            frame.slaveSuffix.LoadFromBytes(frame.rawPtr + Frame::kSlaveSuffixOffset);
            frame.slaveSuffixValid = true;
            if (bigFrame != nullptr) {
                bigFrame->slaveSuffixValid = true;
            }
        }

        return true;
    }

    /// Standalone entry point: Process directly onto TouchBigFrame
    inline bool Process(TouchBigFrame& bigFrame, const uint8_t* rawPtr, size_t rawLen, uint64_t timestamp = 0) {
        bigFrame.masterSuffixValid = false;
        bigFrame.slaveSuffixValid = false;
        bigFrame.hasFinger = false;

        if (!m_enabled) {
            return true;
        }

        if (rawLen < Frame::kMasterFrameSize || rawPtr == nullptr) {
            std::memset(bigFrame.raw, 0, sizeof(bigFrame.raw));
            return true;
        }

        const uint8_t* matrix_ptr = rawPtr + Frame::kMatrixOffset;
        for (int r = 0; r < Frame::kTxCount; ++r) {
            for (int c = 0; c < Frame::kRxCount; ++c) {
                const int i = r * Frame::kRxCount + c;
                const uint16_t val = static_cast<uint16_t>(matrix_ptr[i * 2]) |
                                     (static_cast<uint16_t>(matrix_ptr[i * 2 + 1]) << 8);
                bigFrame.raw[r][c] = DecodeCell(val);
            }
        }

        Frame::MasterSuffixView masterView{};
        masterView.LoadFromBytes(rawPtr + Frame::kMasterSuffixOffset);
        bigFrame.masterSuffixValid = true;
        bigFrame.hasFinger = masterView.hasFinger();
        bigFrame.timestampUs = timestamp;

        if (rawLen >= Frame::kTotalFrameSize) {
            bigFrame.slaveSuffixValid = true;
        }

        return true;
    }
};

} // namespace Solvers::Touch
