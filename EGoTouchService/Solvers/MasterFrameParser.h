#pragma once
// MasterFrameParser：把 SPI 裸帧（7B 头 + 4800B 互容矩阵 + 256B master suffix + 332B slave suffix）
// 解成 HeatmapFrame 的 heatmapMatrix 与两段 suffix，供诊断、IPC、DVR 与 VHF 使用。TSACore 自己从
// rawPtr 取帧，不读这里的结果。
//
// 线上互容字是偏移二进制（0x8000 为零），对应 TSACore Rawdata_NormalizeGridUnit 在
// bRawdataForceNormalize == 0 时按 unsigned 取用；转成补码只需翻转符号位（val ^ 0x8000）。

#include <cstdint>
#include <cstring>

#include "FrameLayout.h"
#include "SolverTypes.h"

namespace Solvers {

class MasterFrameParser {
public:
    /// 只在读到 master 的帧上调用。
    void Process(HeatmapFrame& frame) const noexcept {
        frame.masterSuffixValid = false;
        frame.slaveSuffixValid = false;
        if (frame.rawLen < Frame::kMasterFrameSize || frame.rawPtr == nullptr) {
            std::memset(frame.heatmapMatrix, 0, sizeof(frame.heatmapMatrix));
            return;
        }

        const uint8_t* matrix = frame.rawPtr + Frame::kMatrixOffset;
        for (int r = 0; r < Frame::kTxCount; ++r) {
            for (int c = 0; c < Frame::kRxCount; ++c) {
                const int i = r * Frame::kRxCount + c;
                const auto wire = static_cast<uint16_t>(matrix[i * 2] | (matrix[i * 2 + 1] << 8));
                frame.heatmapMatrix[r][c] = static_cast<int16_t>(wire ^ 0x8000u);
            }
        }

        frame.masterSuffix.LoadFromBytes(frame.rawPtr + Frame::kMasterSuffixOffset);
        frame.masterSuffixValid = true;
        if (frame.rawLen >= Frame::kTotalFrameSize) {
            frame.slaveSuffix.LoadFromBytes(frame.rawPtr + Frame::kSlaveSuffixOffset);
            frame.slaveSuffixValid = true;
        }
    }
};

}  // namespace Solvers
