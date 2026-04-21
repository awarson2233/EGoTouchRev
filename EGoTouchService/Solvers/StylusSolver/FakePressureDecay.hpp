#pragma once
#include <cstdint>

namespace Asa {

class FakePressureDecay {
public:
    inline void Init() {
        if (!m_initialized) {
            m_addNum = kDefaultAddNum;
            m_initialized = true;
        }
    }

    inline uint16_t Step(uint16_t prevPressure) {
        if (m_addNum <= 0) return 0;
        const uint16_t fakePress = static_cast<uint16_t>(
            (static_cast<uint32_t>(m_addNum) * static_cast<uint32_t>(prevPressure)) /
            static_cast<uint32_t>(m_addNum + 1));
        --m_addNum;
        return fakePress;
    }

    inline bool IsActive() const { return m_addNum > 0; }

    inline void Reset() {
        m_addNum = 0;
        m_initialized = false;
    }

    static constexpr int kDefaultAddNum = 2;

    int triggerMinPressure = 500;
    int triggerMaxPressure = 10;

private:
    int m_addNum = 0;
    bool m_initialized = false;
};

} // namespace Asa
