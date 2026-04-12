#pragma once
// ── TouchPipeline Module: MasterFrameParser ──
// Header-only. Converted from Preprocessing/MasterFrameParser.{h,cpp}.
// Parses raw frame bytes → heatmapMatrix + structured suffix views.

#include "EngineTypes.h"
#include "FrameLayout.h"
#include <cstring>

namespace Engine { namespace Touch {

class MasterFrameParser {
public:
    bool m_enabled = true;

    inline bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;

        // Master 帧: 7B header + 4800B matrix + 256B suffix = 5063B
        if (frame.rawData.size() < Frame::kMasterFrameSize) {
            return true;
        }

        const uint8_t* raw_ptr = frame.rawData.data() + Frame::kHeaderBytes;
        int16_t* heat_ptr = reinterpret_cast<int16_t*>(frame.heatmapMatrix);

        // 使用标准 C++ 循环处理，避免在 ARM64 和未对齐地址导致的硬件异常
        // MSVC (O2) 能够很好地将此自动向量化 (NEON)
        for (int i = 0; i < Frame::kMatrixCells; ++i) {
            // 处理无对齐的小端内存加载
            uint16_t val = static_cast<uint16_t>(raw_ptr[i * 2]) |
                           (static_cast<uint16_t>(raw_ptr[i * 2 + 1]) << 8);
            heat_ptr[i] = static_cast<int16_t>(val);
        }

        // Populate structured suffix views from rawData
        frame.masterSuffix.LoadFromBytes(
            frame.rawData.data() + Frame::kMasterSuffixOffset);
        frame.masterSuffixValid = true;

        if (frame.rawData.size() >= Frame::kTotalFrameSize) {
            frame.slaveSuffix.LoadFromBytes(
                frame.rawData.data() + Frame::kSlaveSuffixOffset);
            frame.slaveSuffixValid = true;
        }

        return true;
    }
};

}} // namespace Engine::Touch
