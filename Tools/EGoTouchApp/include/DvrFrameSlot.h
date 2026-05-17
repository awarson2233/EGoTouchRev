#pragma once
// DvrFrameSlot.h — Fixed-size, pure-POD frame slot for zero-copy DVR recording.
//
// Replaces the heap-allocating HeatmapFrame in the DVR ring buffer.
// All fields are fixed-size arrays — PushOverwriting becomes a single memcpy
// with zero heap allocation per frame.
//
// sizeof(DvrFrameSlot) ≈ 10.3 KB per slot
// Total DVR budget: 480 × 10.3 KB ≈ 4.8 MB (vs previous unbounded vector allocs)

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include "FrameLayout.h"
#include "SolverTypes.h"
#include "ServiceProxyTypes.h"

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

// ── DVR stylus snapshot ──
struct DvrStylusPoint {
    bool valid = false;
    float x = 0.0f;
    float y = 0.0f;
    uint16_t reportX = 0;
    uint16_t reportY = 0;
    uint16_t pressure = 0;
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint16_t peakTx1 = 0;
    uint16_t peakTx2 = 0;
    float tx1X = 0.0f;
    float tx1Y = 0.0f;
    float tx2X = 0.0f;
    float tx2Y = 0.0f;
    float confidence = 0.0f;
};

struct DvrStylusData {
    bool slaveValid = false;
    bool checksumOk = false;
    bool tx1BlockValid = false;
    bool tx2BlockValid = false;
    uint32_t status = 0;
    uint16_t pressure = 0;
    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint16_t maxRawPeak = 0;
    uint8_t pipelineStage = 0;
    DvrStylusPoint point{};
};

// ── DvrFrameSlot — the complete POD frame for DVR storage ──
struct DvrFrameSlot {
    // Timestamp and metadata
    uint64_t timestamp = 0;
    uint64_t receiveSystemEpochUs = 0;
    uint64_t dvrSeq = 0;
    bool     masterWasRead = true;

    // Heatmap matrix (4800 bytes)
    int16_t heatmapMatrix[40][60]{};

    // Structured suffix views (588 bytes total)
    Frame::MasterSuffixView masterSuffix{};
    Frame::SlaveSuffixView  slaveSuffix{};
    bool masterSuffixValid = false;
    bool slaveSuffixValid  = false;

    // Stylus diagnostics snapshot
    DvrStylusData stylus{};

    // Fixed-size contact/peak arrays (no heap allocation)
    DvrContact contacts[kMaxContacts]{};
    uint8_t    contactCount = 0;

    DvrPeak    peaks[kMaxPeaks]{};
    uint8_t    peakCount = 0;

    App::DvrDynamicDebugFrame dynamicDebug{};

    // ── Populate from HeatmapFrame ──
    void CopyFrom(const Solvers::HeatmapFrame& src) {
        timestamp = src.timestamp;
        receiveSystemEpochUs = src.receiveSystemEpochUs;
        masterWasRead = src.masterWasRead;

        std::memcpy(heatmapMatrix, src.heatmapMatrix, sizeof(heatmapMatrix));

        masterSuffix = src.masterSuffix;
        slaveSuffix  = src.slaveSuffix;
        masterSuffixValid = src.masterSuffixValid;
        slaveSuffixValid  = src.slaveSuffixValid;

        stylus.slaveValid = src.stylus.input.slaveValid;
        stylus.checksumOk = src.stylus.input.checksumOk;
        stylus.tx1BlockValid = src.stylus.input.tx1BlockValid;
        stylus.tx2BlockValid = src.stylus.input.tx2BlockValid;
        stylus.status = src.stylus.input.status;
        stylus.pressure = src.stylus.output.pressure;
        stylus.signalX = src.stylus.interop.signalX;
        stylus.signalY = src.stylus.interop.signalY;
        stylus.maxRawPeak = src.stylus.interop.maxRawPeak;
        stylus.pipelineStage = src.stylus.output.pipelineStage;
        stylus.point.valid = src.stylus.output.point.valid;
        stylus.point.x = src.stylus.output.point.x;
        stylus.point.y = src.stylus.output.point.y;
        stylus.point.reportX = src.stylus.output.point.reportX;
        stylus.point.reportY = src.stylus.output.point.reportY;
        stylus.point.pressure = src.stylus.output.point.pressure;
        stylus.point.rawPressure = src.stylus.output.point.rawPressure;
        stylus.point.mappedPressure = src.stylus.output.point.mappedPressure;
        stylus.point.peakTx1 = src.stylus.output.point.peakTx1;
        stylus.point.peakTx2 = src.stylus.output.point.peakTx2;
        stylus.point.tx1X = src.stylus.output.point.tx1X;
        stylus.point.tx1Y = src.stylus.output.point.tx1Y;
        stylus.point.tx2X = src.stylus.output.point.tx2X;
        stylus.point.tx2Y = src.stylus.output.point.tx2Y;
        stylus.point.confidence = src.stylus.output.point.confidence;

        // Copy contacts (clamped to fixed capacity)
        contactCount = static_cast<uint8_t>(
            std::min(static_cast<int>(src.contacts.size()), kMaxContacts));
        for (int i = 0; i < contactCount; ++i) {
            const auto& sc = src.contacts[i];
            contacts[i] = { sc.id, sc.x, sc.y, sc.state, sc.area, sc.signalSum };
        }
        for (int i = contactCount; i < kMaxContacts; ++i) {
            contacts[i] = {};
        }

        // Copy peaks (clamped to fixed capacity)
#if EGOTOUCH_DIAG
        peakCount = static_cast<uint8_t>(
            std::min(static_cast<int>(src.peaks.size()), kMaxPeaks));
        for (int i = 0; i < peakCount; ++i) {
            const auto& sp = src.peaks[i];
            peaks[i] = { sp.r, sp.c, sp.z, sp.id, 0 };
        }
#else
        peakCount = 0;
#endif
        for (int i = peakCount; i < kMaxPeaks; ++i) {
            peaks[i] = {};
        }
    }
};

} // namespace Dvr
