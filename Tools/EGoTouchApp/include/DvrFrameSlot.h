#pragma once
// DvrFrameSlot.h — Fixed-size, pure-POD frame slot for zero-copy DVR recording.
//
// Replaces the heap-allocating HeatmapFrame in the DVR ring buffer.
// All fields are fixed-size arrays — PushOverwriting becomes a single memcpy
// with zero heap allocation per frame.
//
// sizeof(DvrFrameSlot) ≈ 10.3 KB per slot
// Total DVR budget: 480 × 10.3 KB ≈ 4.8 MB (vs previous unbounded vector allocs)

#include <cstdint>
#include <cstring>
#include "FrameLayout.h"
#include "SolverTypes.h"

namespace Dvr {

// Maximum counts for fixed-size arrays (chosen from observed maximums)
constexpr int kMaxContacts = 10;
constexpr int kMaxPeaks    = 30;

// ── DvrContact — flat POD mirror of Solvers::TouchContact ──
struct DvrContact {
    int id = 0;
    float x = 0.0f;
    float y = 0.0f;
    int state = 0;
    int area = 0;
    int signalSum = 0;
};
static_assert(sizeof(DvrContact) == 24, "DvrContact must be exactly 24 bytes");

// ── DvrPeak — flat POD mirror of Solvers::TouchPeak ──
struct DvrPeak {
    int r = 0;
    int c = 0;
    int16_t z = 0;
    uint8_t id = 0;
    uint8_t _pad = 0;  // explicit padding
};
static_assert(sizeof(DvrPeak) == 12, "DvrPeak must be exactly 12 bytes");

// ── DvrFrameSlot — the complete POD frame for DVR storage ──
struct DvrFrameSlot {
    // Timestamp and metadata
    uint64_t timestamp = 0;
    bool     masterWasRead = true;

    // Heatmap matrix (4800 bytes)
    int16_t heatmapMatrix[40][60]{};

    // Structured suffix views (588 bytes total)
    Frame::MasterSuffixView masterSuffix{};
    Frame::SlaveSuffixView  slaveSuffix{};
    bool masterSuffixValid = false;
    bool slaveSuffixValid  = false;

    // Fixed-size contact/peak arrays (no heap allocation)
    DvrContact contacts[kMaxContacts]{};
    uint8_t    contactCount = 0;

    DvrPeak    peaks[kMaxPeaks]{};
    uint8_t    peakCount = 0;

    // ── Populate from HeatmapFrame ──
    void CopyFrom(const Solvers::HeatmapFrame& src) {
        timestamp = src.timestamp;
        masterWasRead = src.masterWasRead;

        std::memcpy(heatmapMatrix, src.heatmapMatrix, sizeof(heatmapMatrix));

        masterSuffix = src.masterSuffix;
        slaveSuffix  = src.slaveSuffix;
        masterSuffixValid = src.masterSuffixValid;
        slaveSuffixValid  = src.slaveSuffixValid;

        // Copy contacts (clamped to fixed capacity)
        contactCount = static_cast<uint8_t>(
            std::min(static_cast<int>(src.contacts.size()), kMaxContacts));
        for (int i = 0; i < contactCount; ++i) {
            const auto& sc = src.contacts[i];
            contacts[i] = { sc.id, sc.x, sc.y, sc.state, sc.area, sc.signalSum };
        }

        // Copy peaks (clamped to fixed capacity)
        peakCount = static_cast<uint8_t>(
            std::min(static_cast<int>(src.peaks.size()), kMaxPeaks));
        for (int i = 0; i < peakCount; ++i) {
            const auto& sp = src.peaks[i];
            peaks[i] = { sp.r, sp.c, sp.z, sp.id, 0 };
        }
    }
};

} // namespace Dvr
