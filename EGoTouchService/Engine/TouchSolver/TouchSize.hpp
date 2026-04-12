#pragma once
// ── TouchPipeline Module: TouchSize ──
// Header-only. Converted from TouchSolver/TouchSize.{h,cpp}.
// Converts zone signalSum → approximate radius in mm.

#include "EngineTypes.h"
#include <vector>
#include <cstdint>

namespace Engine { namespace Touch {

class TouchSizeCalculator {
public:
    float m_pixelPitchMm = 4.5f;
    int   m_unitPerSigMm2 = 128;
    uint8_t m_fallbackSizeMm = 5;

    inline void Process(std::vector<TouchContact>& contacts) {
        for (auto& tc : contacts) {
            uint8_t sizeMm = GetSizeInMM(tc.signalSum, m_unitPerSigMm2);
            if (sizeMm == 0) sizeMm = m_fallbackSizeMm;
            tc.sizeMm = static_cast<float>(sizeMm);
        }
    }

private:
    static inline uint8_t GetSizeInMM(int sigSum, int scale) {
        if (sigSum >= 0x200000) return 0xFF;
        uint8_t r = 1;
        int shifted = sigSum << 10;
        while (r < 15) {
            int threshold = scale * r * (r + r * r);
            if (threshold > shifted) break;
            r++;
        }
        return r;
    }
};

}} // namespace Engine::Touch
