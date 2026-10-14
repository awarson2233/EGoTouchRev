#include "HimaxAfeAdapter.h"

#include <algorithm>
#include <cstring>

namespace Solvers::Tsa {

namespace {

#include "HimaxPixelGainTables.inc"

constexpr int kCells = Frame::kMatrixCells;
constexpr int kRx = Frame::kRxCount;
constexpr int kTx = Frame::kTxCount;

// thp_afe_open_project 为 W273AS2700 写入的常量（两个频点取值相同）。
constexpr int kNoDampPositiveThreshold = 300;  // g_MasterNoDampPositiveThreshold
constexpr int kDampPositiveThreshold = 400;    // g_MasterDampPositiveThreshold
constexpr int kDampScalePercent = 110;         // g_MasterDampScalePercent

constexpr uint8_t kDampMaskFresh = 10;

inline uint16_t LoadWord(const uint8_t* p) noexcept {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

}  // namespace

void HimaxAfeAdapter::Reset() noexcept {
    std::fill(std::begin(m_out), std::end(m_out), kZero);
    std::memset(m_prev, 0, sizeof(m_prev));
    std::memset(m_dampMask, 0, sizeof(m_dampMask));
    m_freqShiftInFlight = false;
    m_freqShiftTarget = 0;
    m_lastScanFreq = 0;
    m_checksumErrors = 0;
    m_suffix = {};
    std::memset(m_stylusGrid, 0, sizeof(m_stylusGrid));
}

void HimaxAfeAdapter::SetChargerPlugged(bool plugged) noexcept {
    if (plugged && !m_dampingEnabled) {
        std::memset(m_dampMask, 0, sizeof(m_dampMask));
    }
    m_dampingEnabled = plugged;
}

// thp_compute_checksum16(master + 5, (5063 - 5) / 2)：从第 5 字节起的全部 16 位字求和，
// 低 16 位为 0 且总和不为 0（排除全零帧）才算有效。
bool HimaxAfeAdapter::ValidateMasterChecksum(const uint8_t* rawPtr) noexcept {
    constexpr int kChecksumStart = 5;
    constexpr int kChecksumWords = (Frame::kMasterFrameSize - kChecksumStart) / 2;
    uint32_t sum = 0;
    for (int i = 0; i < kChecksumWords; ++i) {
        sum += LoadWord(rawPtr + kChecksumStart + i * 2);
    }
    return (sum & 0xffffu) == 0 && sum != 0;
}

// thp_afe_damp_repeated_master_frame：相对上一帧跳变过大的格子及其邻域打上标记，
// 标记当帧原样放行，其余格子向上一帧回拉一半。邻域下标不对称（上一行取 c..c+2，
// 下一行取 c-2..c），与原驱动一致。
void HimaxAfeAdapter::DampRepeatedFrame(uint16_t* cells) noexcept {
    for (int i = 0; i < kCells; ++i) {
        const uint16_t delta = static_cast<uint16_t>(cells[i] - m_prev[i] + 300);
        if (delta <= 500) continue;

        const int row = i / kRx;
        m_dampMask[i] = kDampMaskFresh;
        for (int k = i - kRx; k <= i - kRx + 2; ++k) {
            if (k >= 0 && k / kRx == row - 1) m_dampMask[k] = kDampMaskFresh;
        }
        if (i - 1 >= 0 && (i - 1) / kRx == row) m_dampMask[i - 1] = kDampMaskFresh;
        if (i + 1 < kCells && (i + 1) / kRx == row) m_dampMask[i + 1] = kDampMaskFresh;
        for (int k = i + kRx - 2; k <= i + kRx; ++k) {
            if (k < kCells && k / kRx == row + 1) m_dampMask[k] = kDampMaskFresh;
        }
    }

    for (int i = 0; i < kCells; ++i) {
        if (m_dampMask[i] == kDampMaskFresh) {
            m_dampMask[i] = kDampMaskFresh - 1;
            continue;
        }
        const int16_t delta = static_cast<int16_t>(cells[i] - m_prev[i]);
        const int half = (delta < 0 ? -delta : delta) / 2;
        cells[i] = static_cast<uint16_t>(delta > 0 ? cells[i] - half : cells[i] + half);
        if (m_dampMask[i] != 0) --m_dampMask[i];
    }
}

// thp_afe_preprocess_frame_data：每条 TX 以本行最大值的 15% 为死区收缩。
void HimaxAfeAdapter::PreprocessRows(uint16_t* cells) noexcept {
    for (int r = 0; r < kTx; ++r) {
        uint16_t* row = cells + r * kRx;
        int rowMax = -0x8000;
        for (int c = 0; c < kRx; ++c) {
            rowMax = std::max(rowMax, static_cast<int>(row[c]) - kZero);
        }
        const int deadZone = rowMax * 15 / 100;
        for (int c = 0; c < kRx; ++c) {
            int v = static_cast<int>(row[c]) - kZero;
            if (v < 0) {
                v = (v < -deadZone) ? v + deadZone : 0;
            } else {
                v = (deadZone <= v) ? v - deadZone : 0;
            }
            row[c] = static_cast<uint16_t>(v + kZero);
        }
    }
}

// thp_afe_get_frame 的增益 / 压缩循环。注意这里的零点取 0x8000，比死区阶段的 0x7ffe 高 2，
// 所以死区归零的格子在这里是 -2，最终输出仍是 kZero。
void HimaxAfeAdapter::ApplyGainAndCompress(const uint16_t* cells, bool slot1) noexcept {
    const uint8_t* gainTable = slot1 ? kPixelGainTable2 : kPixelGainTable1;
    const int positiveThreshold = m_dampingEnabled ? kDampPositiveThreshold : kNoDampPositiveThreshold;
    const int scalePercent = m_dampingEnabled ? kDampScalePercent : 100;

    bool saturated = false;
    for (int i = 0; i < kCells; ++i) {
        const uint32_t word = cells[i];
        const int outIndex = kCells - 1 - i;
        int v = static_cast<int>(word) - 0x8000;

        if (word - 0x7149u < 10999u) {
            if (!saturated && v > 0) {
                int gain = gainTable[outIndex];
                if (v < positiveThreshold) {
                    if (v < (positiveThreshold * 3 >> 2)) {
                        gain = (v < (positiveThreshold >> 1)) ? gain : gain * 2;
                    } else {
                        gain = gain * 3;
                    }
                    gain /= 4;
                }
                v = ((gain * v) / 100) * scalePercent / 100;
            }
        } else {
            // 出现越界样本后，本帧余下的格子都不再做增益
            saturated = true;
        }

        if (v >= 1) {
            v -= (v < 51) ? 20 : 80;
        } else if (v < 0) {
            v = (v + 50 < 0) ? v + 80 : 0;
        }
        m_out[outIndex] = static_cast<uint16_t>(static_cast<int16_t>(v) + kZero);
    }
}

AfeFrameInfo HimaxAfeAdapter::Build(const uint8_t* rawPtr, size_t rawLen) noexcept {
    AfeFrameInfo info;
    info.scanFreq = m_lastScanFreq;
    info.scanRate = kScanRate;

    if (rawPtr == nullptr || rawLen < static_cast<size_t>(Frame::kMasterFrameSize) ||
        !ValidateMasterChecksum(rawPtr)) {
        ++m_checksumErrors;
        if (m_checksumErrors >= 5) info.status = AfeStatus::kSos;
        return info;
    }
    m_checksumErrors = 0;

    m_suffix.LoadFromBytes(rawPtr + Frame::kMasterSuffixOffset);
    const uint16_t freqSlot = m_suffix.words[10];

    uint16_t cells[kCells];
    for (int i = 0; i < kCells; ++i) {
        cells[i] = LoadWord(rawPtr + Frame::kMatrixOffset + i * 2);
    }
    if (m_dampingEnabled) DampRepeatedFrame(cells);
    std::memcpy(m_prev, cells, sizeof(m_prev));
    PreprocessRows(cells);
    ApplyGainAndCompress(cells, freqSlot != 0);

    const uint32_t status = ComposeStatus();
    m_lastScanFreq = kScanFreqTable[freqSlot != 0 ? 1 : 0];

    info.valid = true;
    info.status = status;
    info.scanFreq = m_lastScanFreq;
    return info;
}

AfeFrameInfo HimaxAfeAdapter::BuildWithoutMaster() const noexcept {
    AfeFrameInfo info;
    info.valid = true;
    info.status = ComposeStatus();
    info.scanFreq = m_lastScanFreq;
    info.scanRate = kScanRate;
    return info;
}

// thp_afe_process_stylus_status 的 AFE status：本帧有效记 2，其余各位取自缓存的 master suffix。
uint32_t HimaxAfeAdapter::ComposeStatus() const noexcept {
    uint32_t status = AfeStatus::kFrameValid;
    if (m_suffix.words[Frame::MasterWord::kIdleFlag] == 1) status |= AfeStatus::kIdle;
    if (m_freqShiftInFlight && m_freqShiftTarget == m_lastScanFreq &&
        m_suffix.words[Frame::MasterWord::kFreqShiftDone] != 0) {
        status |= AfeStatus::kFreqShiftDone;
    }
    if (m_suffix.words[Frame::MasterWord::kDiagStatus] == 0xbb) status |= AfeStatus::kCalibDone;
    if (m_suffix.words[Frame::MasterWord::kPenF0NoiseCount] > 0x5db ||
        m_suffix.words[Frame::MasterWord::kPenF1NoiseCount] > 0x5db) {
        status |= AfeStatus::kChargerNoise;
    }
    if (m_suffix.words[Frame::MasterWord::kFwResetFlag] != 0) status |= AfeStatus::kSos;
    return status;
}

// thp_compute_checksum16(slave + 5, (339 - 5) / 2)：与 master 同一个判据。
bool HimaxAfeAdapter::ValidateSlaveChecksum(const uint8_t* slave) noexcept {
    constexpr int kChecksumStart = 5;
    constexpr int kChecksumWords = (Frame::kSlaveFrameSize - kChecksumStart) / 2;
    uint32_t sum = 0;
    for (int i = 0; i < kChecksumWords; ++i) {
        sum += LoadWord(slave + kChecksumStart + i * 2);
    }
    return (sum & 0xffffu) == 0 && sum != 0;
}

// thp_unpack_9x9_block_to_matrix：块头两个字是 9×9 窗口中心所在的行、列（各取低字节），
// 窗口里落在面板内的格子写进整屏矩阵，下标与 master 一样整帧倒序。
void HimaxAfeAdapter::UnpackStylusBlock(const uint8_t* block, uint16_t* matrix) noexcept {
    const uint16_t rowWord = LoadWord(block);
    const uint16_t colWord = LoadWord(block + 2);
    if (rowWord == 0xff && colWord == 0xff) return;

    const int centerRow = static_cast<int8_t>(rowWord & 0xff);
    const int centerCol = static_cast<int8_t>(colWord & 0xff);
    const uint8_t* cell = block + 4;
    for (int r = 0; r < Frame::kGridDim; ++r) {
        const int row = centerRow - 4 + r;
        for (int c = 0; c < Frame::kGridDim; ++c, cell += 2) {
            const int col = centerCol - 4 + c;
            const int index = row * kRx + col;
            if (index < 0 || index >= kCells || col > kRx || row > kTx) continue;
            matrix[kCells - 1 - index] = LoadWord(cell);
        }
    }
}

AfeStylusInfo HimaxAfeAdapter::BuildStylus(const uint8_t* rawPtr, size_t rawLen) noexcept {
    // slave 帧里两个块的位置：matrix_header = 7，g_SlaveStylusF1Offset = 173
    constexpr int kBlock0Offset = Frame::kHeaderBytes;
    constexpr int kBlock1Offset = Frame::kHeaderBytes + Frame::kBlockWords * 2;
    // master suffix word24 达到这个值时，驱动把本帧的笔数据换成默认矩阵并按无笔数据上报
    constexpr int kStylusNoiseWord = 24;
    constexpr uint16_t kStylusNoiseLimit = 3000;

    AfeStylusInfo info;
    if (rawPtr == nullptr || rawLen < static_cast<size_t>(Frame::kTotalFrameSize)) return info;
    const uint8_t* slave = rawPtr + Frame::kSlaveHeaderOffset;
    if (!ValidateSlaveChecksum(slave)) return info;

    const bool hasSignal =
        LoadWord(slave + kBlock0Offset) != 0xff || LoadWord(slave + kBlock0Offset + 2) != 0xff;
    std::memset(m_stylusGrid, 0, sizeof(m_stylusGrid));
    UnpackStylusBlock(slave + kBlock0Offset, m_stylusGrid);
    UnpackStylusBlock(slave + kBlock1Offset, m_stylusGrid + kCells);

    if (!hasSignal || !m_stylusConnected || m_suffix.words[kStylusNoiseWord] >= kStylusNoiseLimit) {
        return info;
    }
    info.status = StylusStatus::kActive;
    info.tx1 = m_stylusGrid;
    info.tx2 = m_stylusGrid + kCells;
    return info;
}

}  // namespace Solvers::Tsa
