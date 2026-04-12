#pragma once
// ── TouchPipeline Module: BaselineSubtraction ──
// Header-only. Converted from Preprocessing/BaselineSubtraction.{h,cpp}.

#include "SolverTypes.h"
#include "NeonCompat.h"
#include <cstddef>
#include <cstdint>

#if defined(_M_X64) || defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#endif

namespace Solvers { namespace Touch {

class BaselineSubtraction {
public:
    bool m_enabled = true;
    int  m_baseline = 0x7FFE; // Default baseline (32766)

    inline bool Process(HeatmapFrame& frame) {
        if (!m_enabled) return true;

        int16_t* ptr = reinterpret_cast<int16_t*>(frame.heatmapMatrix);
        const int16_t base = static_cast<int16_t>(m_baseline);
        constexpr size_t kCellCount = 40u * 60u;
        size_t i = 0;

#if defined(_M_ARM64) || defined(__ARM_NEON)
        const int16x8_t baseVec = vdupq_n_s16(base);
        for (; i + 8u <= kCellCount; i += 8u) {
            int16x8_t values = vld1q_s16(ptr + i);
            vst1q_s16(ptr + i, vsubq_s16(values, baseVec));
        }
#elif defined(_M_X64) || defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
        const __m128i baseVec = _mm_set1_epi16(base);
        for (; i + 8u <= kCellCount; i += 8u) {
            __m128i values = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr + i));
            _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr + i),
                             _mm_sub_epi16(values, baseVec));
        }
#endif

        for (; i < kCellCount; ++i) {
            ptr[i] = static_cast<int16_t>(ptr[i] - base);
        }
        return true;
    }
};

}} // namespace Solvers::Touch
