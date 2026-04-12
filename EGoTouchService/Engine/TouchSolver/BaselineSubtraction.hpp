#pragma once
// ── TouchPipeline Module: BaselineSubtraction ──
// Header-only. Converted from Preprocessing/BaselineSubtraction.{h,cpp}.

#include "EngineTypes.h"
#include <cstdint>

namespace Engine { namespace Touch {

class BaselineSubtraction {
public:
    bool m_enabled = true;
    int  m_baseline = 0x7FFE; // Default baseline (32766)

    inline bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;

        int16_t* ptr = reinterpret_cast<int16_t*>(frame.heatmapMatrix);
        int16_t base = static_cast<int16_t>(m_baseline);

        for (int i = 0; i < 2400; ++i) {
            ptr[i] = ptr[i] - base;
        }
        return true;
    }
};

}} // namespace Engine::Touch
