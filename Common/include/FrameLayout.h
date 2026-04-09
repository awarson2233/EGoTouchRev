#pragma once
// FrameLayout.h — Hardware frame constants and structured overlay types.
//
// Provides typed, zero-cost views over the raw SPI frame buffers (master suffix,
// slave suffix) so that consumers can use named accessors instead of magic byte
// offsets.  The View structs are pure POD and can be embedded directly in shared
// memory, DVR ring buffers, and IPC structures.
//
// All offsets validated against Ghidra reverse-engineering of himax_thp_drv.dll.
// See: docs/逆向/frame_memory_layout.md

#include <cstdint>
#include <cstring>

namespace Frame {

// ── Hardware Frame Constants ─────────────────────────────────────────────
constexpr int kHeaderBytes        = 7;
constexpr int kTxCount            = 40;
constexpr int kRxCount            = 60;
constexpr int kMatrixCells        = kTxCount * kRxCount;           // 2400
constexpr int kMatrixBytes        = kMatrixCells * 2;              // 4800

constexpr int kMasterSuffixWords  = 128;
constexpr int kMasterSuffixBytes  = kMasterSuffixWords * 2;       // 256
constexpr int kSlaveSuffixWords   = 166;
constexpr int kSlaveSuffixBytes   = kSlaveSuffixWords * 2;        // 332

constexpr int kMasterFrameSize    = kHeaderBytes + kMatrixBytes + kMasterSuffixBytes;  // 5063
constexpr int kSlaveFrameSize     = kHeaderBytes + kSlaveSuffixBytes;                  // 339
constexpr int kTotalFrameSize     = kMasterFrameSize + kSlaveFrameSize;                // 5402

// ── Offsets within back_data[0..5401] ────────────────────────────────────
constexpr int kMatrixOffset       = kHeaderBytes;                      // 7
constexpr int kMasterSuffixOffset = kHeaderBytes + kMatrixBytes;       // 4807
constexpr int kSlaveHeaderOffset  = kMasterFrameSize;                  // 5063
constexpr int kSlaveSuffixOffset  = kSlaveHeaderOffset + kHeaderBytes; // 5070

// ── Master Suffix Confirmed Word Indices (in 128-word status table) ─────
namespace MasterWord {
    constexpr int kRetryFlag         = 0;   // +0x00  FW requests host to retry this frame
    constexpr int kFreqShiftDone     = 2;   // +0x04  !=0 means chip completed freq switch
    constexpr int kDiagStatus        = 3;   // +0x06  0xBB = diagnostic marker
    constexpr int kPendingFreqSwitch = 6;   // +0x0C  !=0 means pending freq switch request
    constexpr int kTpFreq1           = 8;   // +0x10  current TPIC frequency 1
    constexpr int kTpFreq2           = 9;   // +0x12  current TPIC frequency 2
    constexpr int kPenF0NoiseCount   = 14;  // +0x1C  F0 noise count (>5000 triggers switch)
    constexpr int kPenF1NoiseCount   = 16;  // +0x20  F1 noise count (>5000 triggers switch)
    constexpr int kTouchX            = 54;  // +0x6C  touch X coordinate (0xFF = no touch)
    constexpr int kTouchY            = 55;  // +0x6E  touch Y coordinate (0xFF = no touch)
}

// ── Slave Suffix Constants ───────────────────────────────────────────────
constexpr int kBlockWords       = 83;    // 2 anchor words + 81 grid words
constexpr int kGridDim          = 9;
constexpr int kGridSize         = kGridDim * kGridDim;  // 81
constexpr uint16_t kAnchorInvalid = 0x00FF;

// ── MasterSuffixView ─────────────────────────────────────────────────────
// Overlays the raw 256-byte (128-word) master status table.
// On little-endian platforms (Windows x86/ARM64), the in-memory layout of
// uint16_t[] matches the SPI LE byte stream — so a direct memcpy from
// back_data[4807..5062] into `words[]` is valid without byte-swapping.
struct MasterSuffixView {
    uint16_t words[kMasterSuffixWords]{};

    // ── Named accessors for confirmed fields ──
    uint16_t retryFlag()         const { return words[MasterWord::kRetryFlag]; }
    uint16_t freqShiftDone()     const { return words[MasterWord::kFreqShiftDone]; }
    uint16_t diagStatus()        const { return words[MasterWord::kDiagStatus]; }
    uint16_t pendingFreqSwitch() const { return words[MasterWord::kPendingFreqSwitch]; }
    uint16_t tpFreq1()           const { return words[MasterWord::kTpFreq1]; }
    uint16_t tpFreq2()           const { return words[MasterWord::kTpFreq2]; }
    uint16_t penF0NoiseCount()   const { return words[MasterWord::kPenF0NoiseCount]; }
    uint16_t penF1NoiseCount()   const { return words[MasterWord::kPenF1NoiseCount]; }
    uint16_t touchX()            const { return words[MasterWord::kTouchX]; }
    uint16_t touchY()            const { return words[MasterWord::kTouchY]; }

    // ── Convenience predicates ──
    bool hasFinger() const {
        return !((touchX() & 0xFF) == 0xFF && (touchY() & 0xFF) == 0xFF);
    }

    // ── Populate from raw SPI byte stream (LE) ──
    // On little-endian: equivalent to memcpy(words, data, 256).
    // Explicit loop kept for clarity and portability.
    void LoadFromBytes(const uint8_t* data) {
        for (int i = 0; i < kMasterSuffixWords; ++i)
            words[i] = static_cast<uint16_t>(data[i * 2] | (data[i * 2 + 1] << 8));
    }
};
static_assert(sizeof(MasterSuffixView) == kMasterSuffixBytes,
              "MasterSuffixView must be exactly 256 bytes");

// ── SlaveSuffixView ──────────────────────────────────────────────────────
// Overlays the raw 332-byte (166-word) slave pen data.
// Layout: TX1 block (83 words) + TX2 block (83 words).
struct SlaveSuffixView {
    uint16_t words[kSlaveSuffixWords]{};

    // ── TX1 block [0..82] ──
    uint16_t tx1AnchorRow() const { return words[0]; }
    uint16_t tx1AnchorCol() const { return words[1]; }
    int16_t  tx1Grid(int r, int c) const {
        return static_cast<int16_t>(words[2 + r * kGridDim + c]);
    }
    bool tx1Valid() const {
        return (tx1AnchorRow() != kAnchorInvalid) || (tx1AnchorCol() != kAnchorInvalid);
    }

    // ── TX2 block [83..165] ──
    uint16_t tx2AnchorRow() const { return words[kBlockWords]; }
    uint16_t tx2AnchorCol() const { return words[kBlockWords + 1]; }
    int16_t  tx2Grid(int r, int c) const {
        return static_cast<int16_t>(words[kBlockWords + 2 + r * kGridDim + c]);
    }
    bool tx2Valid() const { return tx1Valid(); }  // TX2 is garbage when no pen

    // ── Pen presence detection ──
    bool hasStylus() const {
        return !((words[0] & 0xFF) == 0xFF && (words[1] & 0xFF) == 0xFF);
    }

    // ── Populate from raw SPI byte stream (LE) ──
    void LoadFromBytes(const uint8_t* data) {
        for (int i = 0; i < kSlaveSuffixWords; ++i)
            words[i] = static_cast<uint16_t>(data[i * 2] | (data[i * 2 + 1] << 8));
    }
};
static_assert(sizeof(SlaveSuffixView) == kSlaveSuffixBytes,
              "SlaveSuffixView must be exactly 332 bytes");

} // namespace Frame
