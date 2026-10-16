#include "HimaxAfeAdapter.h"

#include <algorithm>
#include <bit>
#include <cstring>

namespace Solvers::Tsa {

namespace {

#include "HimaxPixelGainTables.inc"

// SPI 帧是小端字节流，直接按 uint16 读
static_assert(std::endian::native == std::endian::little);

constexpr int kCells = Frame::kMatrixCells;
constexpr int kRx = Frame::kRxCount;
constexpr int kTx = Frame::kTxCount;

// thp_afe_open_project 写入的常量（各面板、两个频点取值都相同）。
constexpr int kNoDampPositiveThreshold = 300;  // g_MasterNoDampPositiveThreshold
constexpr int kDampPositiveThreshold = 400;    // g_MasterDampPositiveThreshold
constexpr int kDampScalePercent = 110;         // g_MasterDampScalePercent

constexpr uint8_t kDampMaskFresh = 10;

// 增益只作用在 [kGainWindowLow, kGainWindowLow + kGainWindowLen) 内的样本
constexpr uint32_t kGainWindowLow = 0x7149;
constexpr uint32_t kGainWindowLen = 10999;

inline uint16_t LoadWord(const uint8_t* p) noexcept {
    uint16_t w;
    std::memcpy(&w, p, sizeof(w));
    return w;
}

// thp_compute_checksum16(p, words)：words 个 16 位字求和，低 16 位为 0 且总和不为 0（排除全零帧）才算有效。
bool Checksum16Ok(const uint8_t* p, int words) noexcept {
    uint32_t sum = 0;
    for (int i = 0; i < words; ++i) sum += LoadWord(p + i * 2);
    return (sum & 0xffffu) == 0 && sum != 0;
}

bool OutsideGainWindow(uint32_t word) noexcept { return word - kGainWindowLow >= kGainWindowLen; }

/// 第一个落在增益窗口外的格（没有时为 kCells）。按 64 格一块先粗筛（无分支、可向量化）。
int FirstOutsideGainWindow(const uint16_t* cells) noexcept {
    constexpr int kChunk = 64;
    for (int base = 0; base < kCells; base += kChunk) {
        const int end = std::min(base + kChunk, kCells);
        uint8_t any = 0;
        for (int i = base; i < end; ++i) any |= OutsideGainWindow(cells[i]) ? 1 : 0;
        if (any == 0) continue;
        for (int i = base; i < end; ++i) {
            if (OutsideGainWindow(cells[i])) return i;
        }
    }
    return kCells;
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
    m_stylusGridDirty = false;
}

void HimaxAfeAdapter::SetPanelGain(PanelGain gain) noexcept {
    const bool boe = gain == PanelGain::Boe;
    m_gainTable[0] = boe ? kBoePixelGainTable1 : kCsotPixelGainTable1;
    m_gainTable[1] = boe ? kBoePixelGainTable2 : kCsotPixelGainTable2;
}

void HimaxAfeAdapter::SetChargerPlugged(bool plugged) noexcept {
    if (plugged && !m_dampingEnabled) {
        std::memset(m_dampMask, 0, sizeof(m_dampMask));
    }
    m_dampingEnabled = plugged;
}

// thp_compute_checksum16(master + 5, (5063 - 5) / 2)：从第 5 字节起的全部 16 位字。
bool HimaxAfeAdapter::ValidateMasterChecksum(const uint8_t* rawPtr) noexcept {
    constexpr int kChecksumStart = 5;
    return Checksum16Ok(rawPtr + kChecksumStart, (Frame::kMasterFrameSize - kChecksumStart) / 2);
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
// 原代码按 v = 样本 - kZero 的 int 运算；这里留在 uint16 域里算（可按 8 个 lane 向量化），结果相同：
//   - 行最大值：60 个样本减零点后不会低于原初值 -0x8000，所以就是最大样本减零点；
//   - v < 0 ⇔ 样本 < kZero；v < -deadZone ⇔ 样本 < kZero - deadZone，v ≥ deadZone ⇔ 样本 ≥ kZero + deadZone，
//     两个阈值夹到 [0, 0x10000] 不改变比较结果；
//   - v ± deadZone + kZero 就是样本 ± deadZone，原代码同样按 2^16 取模写回。
void HimaxAfeAdapter::PreprocessRows(uint16_t* cells) noexcept {
    for (int r = 0; r < kTx; ++r) {
        uint16_t* row = cells + r * kRx;
        uint16_t rowMax = 0;
        for (int c = 0; c < kRx; ++c) rowMax = std::max(rowMax, row[c]);
        const int deadZone = (static_cast<int>(rowMax) - kZero) * 15 / 100;
        const auto lowBound = static_cast<uint32_t>(std::clamp(kZero - deadZone, 0, 0x10000));
        const auto highBound = static_cast<uint32_t>(std::clamp(kZero + deadZone, 0, 0x10000));
        const auto shift = static_cast<uint16_t>(deadZone);
        for (int c = 0; c < kRx; ++c) {
            const uint32_t w = row[c];
            const uint16_t below = w < lowBound ? static_cast<uint16_t>(w + shift) : kZero;
            const uint16_t above = w >= highBound ? static_cast<uint16_t>(w - shift) : kZero;
            row[c] = w < kZero ? below : above;
        }
    }
}

namespace {

/// 增益阶段的有符号样本（零点 0x8000），正好落在 int16 内。
inline int16_t Signed(uint16_t word) noexcept { return static_cast<int16_t>(word - 0x8000); }

// 偏移压缩：v ≥ 1 时减 20（v < 51）或 80，v < 0 时加 80（加完仍为负）或归零，再加回零点。
// 原代码按 int 算再截成 int16；v 是 int16 时各分支都不溢出，直接在 int16 里算结果相同（可按 8 个 lane 向量化）。
inline uint16_t Compress(int16_t v) noexcept {
    const auto positive = static_cast<int16_t>(v - (v < 51 ? 20 : 80));
    const auto negative = v < -50 ? static_cast<int16_t>(v + 80) : int16_t{0};
    const int16_t w = v >= 1 ? positive : v < 0 ? negative : v;
    return static_cast<uint16_t>(w + HimaxAfeAdapter::kZero);
}

// [0, end) 的格子做增益再压缩，写到倒序的位置（这些格都在增益窗口内，v ∈ [-3767, 7232)）。小信号削弱增益：
// v 在 [0, T/2)、[T/2, 3T/4)、[3T/4, T) 时取 1/4、2/4、3/4，其余不变（4/4）。只对 v > 0 生效，增益与 v 都非负，
// 按无符号算与原来的有符号除法相同；增益至多 350，结果不超过 350 × 7231 / 100 × 110 / 100，落在 int16 内。
// 不阻尼时缩放是 100%，(x / 100) × 100 / 100 就是 x / 100，少一次除法。
template <bool kDamped>
void GainAndCompress(const uint16_t* cells, int end, const uint16_t* gainTable, uint16_t* out) noexcept {
    constexpr int kThreshold = kDamped ? kDampPositiveThreshold : kNoDampPositiveThreshold;
    for (int i = 0; i < end; ++i) {
        const int outIndex = kCells - 1 - i;
        const int16_t v = Signed(cells[i]);
        const uint32_t quarters = v < (kThreshold >> 1) ? 1u : v < (kThreshold * 3 >> 2) ? 2u : v < kThreshold ? 3u : 4u;
        const uint32_t gain = gainTable[outIndex] * quarters >> 2;
        uint32_t gained = gain * static_cast<uint16_t>(v) / 100u;
        if constexpr (kDamped) gained = gained * kDampScalePercent / 100u;
        out[outIndex] = Compress(v > 0 ? static_cast<int16_t>(gained) : v);
    }
}

// [begin, kCells) 的格子不做增益，只压缩。
void CompressOnly(const uint16_t* cells, int begin, uint16_t* out) noexcept {
    for (int i = begin; i < kCells; ++i) out[kCells - 1 - i] = Compress(Signed(cells[i]));
}

}  // namespace

// thp_afe_get_frame 的增益 / 压缩循环。注意这里的零点取 0x8000，比死区阶段的 0x7ffe 高 2，
// 所以死区归零的格子在这里是 -2，最终输出仍是 kZero。
// 原循环带一个「已出现越界样本」的标志：从第一个落在增益窗口外的格起，本帧余下的格子（含它自己）都不做
// 增益。这里先找到那一格，前后两段各自没有跨格依赖，写成无分支的选择后可以向量化。
void HimaxAfeAdapter::ApplyGainAndCompress(const uint16_t* cells, bool slot1) noexcept {
    const uint16_t* gainTable = m_gainTable[slot1 ? 1 : 0];
    const int gainEnd = FirstOutsideGainWindow(cells);
    if (m_dampingEnabled) {
        GainAndCompress<true>(cells, gainEnd, gainTable, m_out);
    } else {
        GainAndCompress<false>(cells, gainEnd, gainTable, m_out);
    }
    CompressOnly(cells, gainEnd, m_out);
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
    std::memcpy(cells, rawPtr + Frame::kMatrixOffset, sizeof(cells));
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
    return Checksum16Ok(slave + kChecksumStart, (Frame::kSlaveFrameSize - kChecksumStart) / 2);
}

// thp_unpack_9x9_block_to_matrix：块头两个字是 9×9 窗口中心所在的行、列（各取低字节），
// 窗口里落在面板内的格子写进整屏矩阵，下标与 master 一样整帧倒序。返回是否可能写过矩阵。
bool HimaxAfeAdapter::UnpackStylusBlock(const uint8_t* block, uint16_t* matrix) noexcept {
    const uint16_t rowWord = LoadWord(block);
    const uint16_t colWord = LoadWord(block + 2);
    if (rowWord == 0xff && colWord == 0xff) return false;

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
    return true;
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
    // 驱动每帧先把整屏矩阵清零再展开；没有笔时两个块都是空的，矩阵上一帧也没写过，就不必再清
    if (m_stylusGridDirty) std::memset(m_stylusGrid, 0, sizeof(m_stylusGrid));
    m_stylusGridDirty = UnpackStylusBlock(slave + kBlock0Offset, m_stylusGrid);
    m_stylusGridDirty |= UnpackStylusBlock(slave + kBlock1Offset, m_stylusGrid + kCells);

    if (!hasSignal || !m_stylusConnected || m_suffix.words[kStylusNoiseWord] >= kStylusNoiseLimit) {
        return info;
    }
    info.status = StylusStatus::kActive;
    info.tx1 = m_stylusGrid;
    info.tx2 = m_stylusGrid + kCells;
    return info;
}

}  // namespace Solvers::Tsa
