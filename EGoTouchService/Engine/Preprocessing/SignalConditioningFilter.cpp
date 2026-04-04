#include "SignalConditioningFilter.h"
#include <algorithm>
#include <cstring>

#if defined(_M_ARM64)
#include <arm_neon.h>
#endif

namespace Engine {

SignalConditioningFilter::SignalConditioningFilter() : m_hasHistory(false) {
    std::memset(m_historyData, 0, sizeof(m_historyData));
}

SignalConditioningFilter::~SignalConditioningFilter() {}

bool SignalConditioningFilter::Process(HeatmapFrame& frame) {
    if (!m_enabled) {
        m_hasHistory = false;
        return true;
    }

    const int numPixels = 40 * 60;
    int16_t* frameData16 = &frame.heatmapMatrix[0][0];

    // 1. IIR 时域滤波 (Exponential Moving Average)
    const int32_t alpha = m_alpha;       // Current frame weight (0-1000)
    const int32_t beta = 1000 - alpha;

    if (!m_hasHistory) {
        std::memcpy(m_historyData, frameData16, numPixels * sizeof(int16_t));
        m_hasHistory = true;
    }

    // If alpha is 1000, IIR is effectively disabled.
    if (alpha < 1000) {
        for (int i = 0; i < numPixels; ++i) {
            // Apply IIR only to positive signals to keep noise floor stable
            if (frameData16[i] > 0) {
                int32_t val = frameData16[i];
                int32_t hist = m_historyData[i];
                val = (val * alpha + hist * beta) / 1000;
                frameData16[i] = static_cast<int16_t>(val);
            }
            m_historyData[i] = frameData16[i];
        }
    } else {
        std::memcpy(m_historyData, frameData16, numPixels * sizeof(int16_t));
    }

    // 2. 线性底噪切除水平面 (Water-level Clipping)
    // 算法：对于 > noiseFloor 的信号，减去 noiseFloor。对于 =< noiseFloor 的信号，归零。
    // 特性：保证抛物面的连续性，不会出现阶梯掉崖式反相撕裂！

#if defined(_M_ARM64)
    int i = 0;
    int16x8_t vZero = vdupq_n_s16(0);
    int16x8_t vFloor = vdupq_n_s16((int16_t)m_noiseFloor);
    
    for (; i <= numPixels - 8; i += 8) {
        int16x8_t vData = vld1q_s16(&frameData16[i]);
        
        // 核心公式: result = max(0, Data - Floor)
        int16x8_t vSubbed = vsubq_s16(vData, vFloor);
        int16x8_t result = vmaxq_s16(vZero, vSubbed);
        
        vst1q_s16(&frameData16[i], result);
    }
    
    // 剩余尾部处理
    for (; i < numPixels; ++i) {
        int16_t v = frameData16[i];
        frameData16[i] = std::max<int16_t>(0, v - m_noiseFloor);
    }
#else
    for (int i = 0; i < numPixels; ++i) {
        int16_t v = frameData16[i];
        frameData16[i] = std::max<int16_t>(0, v - m_noiseFloor);
    }
#endif

    return true;
}

std::vector<ConfigParam> SignalConditioningFilter::GetConfigSchema() const {
    std::vector<ConfigParam> schema = IFrameProcessor::GetConfigSchema();
    schema.push_back(ConfigParam("IIRAlpha", "IIR Alpha",
        ConfigParam::Int, const_cast<int*>(&m_alpha), 100, 1000));
    schema.push_back(ConfigParam("NoiseFloor", "Noise Cut-off Floor",
        ConfigParam::Int, const_cast<int*>(&m_noiseFloor), 0, 500));
    return schema;
}

} // namespace Engine
