#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "SolverBuildConfig.h"

namespace Solvers {

enum TouchContactState : int {
    TouchStateDown = 0,
    TouchStateMove = 1,
    TouchStateUp = 2,
};

enum TouchLifeFlagBits : uint32_t {
    TouchLifeMapped = 1u << 0,
    TouchLifeNew = 1u << 1,
    TouchLifeLiftOff = 1u << 2,
    TouchLifeEdge = 1u << 3,
    TouchLifeDebounced = 1u << 4,
    TouchLifeAlwaysMatch = 1u << 5,
    TouchLifeSilentGap = 1u << 6,
};

enum TouchReportEventCode : int {
    TouchReportIdle = 1,
    TouchReportDown = 2,
    TouchReportMove = 4,
    TouchReportUp = 0x20,
};

// 触摸点结构体 (用于 Stage 2 连通域计算)
struct TouchContact {
    int id = 0;
    float x = 0.0f;
    float y = 0.0f;
    int state = 0; // 0=Down, 1=Update, 2=Up
    int area = 0;  // 连通域大小或强度
    int signalSum = 0; // 区域信号总和(对齐 TS 的 SigSum 语义)

    // Extended fields for TS/TE/TouchReport-aligned processing.
    float sizeMm = 0.0f;
    bool isEdge = false;
    bool isReported = true;
    int prevIndex = -1;
    int debugFlags = 0;
    uint32_t edgeFlags = 0;
    uint8_t centroidEdgeFlags = 0;
    uint32_t ecFlags = 0;
    float edgeDistX = 0.0f;
    float edgeDistY = 0.0f;
    float rawXBeforeEC = 0.0f;
    float rawYBeforeEC = 0.0f;
    uint8_t ecWidthX = 0;
    uint8_t ecWidthY = 0;

    // TS/TE/TouchReport-aligned state mirrors
    uint32_t lifeFlags = 0;
    uint32_t reportFlags = 0;
    int reportEvent = 0;

    // Upstream peak identity, used only as a weak tracking hint.
    uint8_t sourcePeakId = 0;
    uint8_t sourcePeakAge = 0;
};

template <typename T, size_t Capacity>
class FixedVector {
public:
    using value_type = T;
    using iterator = typename std::array<T, Capacity>::iterator;
    using const_iterator = typename std::array<T, Capacity>::const_iterator;

    [[nodiscard]] constexpr size_t size() const noexcept { return m_size; }
    [[nodiscard]] constexpr size_t capacity() const noexcept { return Capacity; }
    [[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }

    constexpr void clear() noexcept { m_size = 0; }
    constexpr void reserve(size_t) const noexcept {}

    constexpr void resize(size_t count) noexcept {
        const size_t newSize = std::min(count, Capacity);
        for (size_t i = m_size; i < newSize; ++i) {
            m_data[i] = T{};
        }
        m_size = newSize;
    }

    constexpr bool try_push_back(const T& value) noexcept {
        if (m_size >= Capacity) return false;
        m_data[m_size++] = value;
        return true;
    }

    constexpr bool try_push_back(T&& value) noexcept {
        if (m_size >= Capacity) return false;
        m_data[m_size++] = std::move(value);
        return true;
    }

    constexpr void push_back(const T& value) noexcept { (void)try_push_back(value); }
    constexpr void push_back(T&& value) noexcept { (void)try_push_back(std::move(value)); }

    constexpr void assign(const T* first, const T* last) noexcept {
        clear();
        for (const T* it = first; it != last && m_size < Capacity; ++it) {
            m_data[m_size++] = *it;
        }
    }

    template <typename InputIt>
    constexpr void assign(InputIt first, InputIt last) noexcept {
        clear();
        for (auto it = first; it != last && m_size < Capacity; ++it) {
            m_data[m_size++] = *it;
        }
    }

    constexpr iterator erase(const_iterator first, const_iterator last) noexcept {
        const size_t beginIndex = static_cast<size_t>(first - m_data.cbegin());
        const size_t endIndex = static_cast<size_t>(last - m_data.cbegin());
        if (beginIndex > m_size || endIndex <= beginIndex) {
            return begin() + static_cast<std::ptrdiff_t>(std::min(beginIndex, m_size));
        }
        const size_t clampedEnd = std::min(endIndex, m_size);
        const size_t removed = clampedEnd - beginIndex;
        std::move(m_data.begin() + static_cast<std::ptrdiff_t>(clampedEnd),
                  m_data.begin() + static_cast<std::ptrdiff_t>(m_size),
                  m_data.begin() + static_cast<std::ptrdiff_t>(beginIndex));
        m_size -= removed;
        return begin() + static_cast<std::ptrdiff_t>(beginIndex);
    }

    constexpr T& operator[](size_t index) noexcept { return m_data[index]; }
    constexpr const T& operator[](size_t index) const noexcept { return m_data[index]; }

    constexpr iterator begin() noexcept { return m_data.begin(); }
    constexpr iterator end() noexcept { return m_data.begin() + static_cast<std::ptrdiff_t>(m_size); }
    constexpr const_iterator begin() const noexcept { return m_data.begin(); }
    constexpr const_iterator end() const noexcept { return m_data.begin() + static_cast<std::ptrdiff_t>(m_size); }
    constexpr const_iterator cbegin() const noexcept { return begin(); }
    constexpr const_iterator cend() const noexcept { return end(); }

    constexpr std::span<T> span() noexcept { return {m_data.data(), m_size}; }
    constexpr std::span<const T> span() const noexcept { return {m_data.data(), m_size}; }

private:
    std::array<T, Capacity> m_data{};
    size_t m_size = 0;
};

inline constexpr size_t kMaxTouchContacts = 20;

struct TouchPacket {
    bool valid = false;
    uint8_t reportId = 0x01;
    uint8_t length = 0x20;
    std::array<uint8_t, 32> bytes{};
};

struct TouchPeak {
    int r = 0;
    int c = 0;
    int16_t z = 0;
    uint8_t id = 0;
};

// Represents a connected component in the heatmap greater than a global threshold
struct MacroZone {
    std::span<const int> pixels{}; // 1D indices (r * cols + c), owned by MacroZoneDetector arena
    int area = 0;
    int signalSum = 0;
    int minR = 39;
    int maxR = 0;
    int minC = 59;
    int maxC = 0;
};

struct TouchOutputState {
    FixedVector<TouchContact, kMaxTouchContacts> contacts;
    std::array<TouchPacket, 2> touchPackets{};
};

#if EGOTOUCH_DIAG
struct TouchDebugFrame {
    std::vector<TouchPeak> peaks;
    std::array<uint8_t, 2400> touchZones{};
    std::array<uint8_t, 2400> peakZones{};
};
#endif

struct TouchFrameData {
    TouchOutputState output{};
#if EGOTOUCH_DIAG
    TouchDebugFrame debug{};
#endif

    inline void ResetRuntime() {}
};

} // namespace Solvers
