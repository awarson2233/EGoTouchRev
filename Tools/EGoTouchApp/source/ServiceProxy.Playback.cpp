#include "ServiceProxyInternal.h"
#include "FrameLayout.h"
#include <algorithm>
#include <chrono>

namespace App {

namespace {

constexpr uint64_t kSyntheticPlaybackStepUs = 10000ull;

bool IsStrictlyIncreasingHostTimeline(const std::vector<Solvers::HeatmapFrame>& frames) {
    if (frames.empty()) return false;
    uint64_t previous = 0;
    bool first = true;
    for (const auto& frame : frames) {
        if (frame.receiveSystemEpochUs == 0) return false;
        if (!first && frame.receiveSystemEpochUs <= previous) return false;
        previous = frame.receiveSystemEpochUs;
        first = false;
    }
    return true;
}

bool IsMonotonicServiceTimeline(const std::vector<Solvers::HeatmapFrame>& frames) {
    if (frames.empty()) return false;
    uint64_t previous = 0;
    bool first = true;
    for (const auto& frame : frames) {
        if (!first && frame.timestamp < previous) return false;
        previous = frame.timestamp;
        first = false;
    }
    return true;
}

void PopulateRawHeatmapFromFrameData(Solvers::HeatmapFrame& frame) {
    if (!frame.rawPtr || frame.rawLen < Frame::kMasterFrameSize) return;
    const uint8_t* raw = frame.rawPtr + Frame::kMatrixOffset;
    int16_t* heat = reinterpret_cast<int16_t*>(frame.heatmapMatrix);
    for (int i = 0; i < Frame::kMatrixCells; ++i) {
        const uint16_t value = static_cast<uint16_t>(raw[i * 2]) |
            (static_cast<uint16_t>(raw[i * 2 + 1]) << 8);
        heat[i] = static_cast<int16_t>(value);
    }
}

const char* PlaybackTimingModeLabel(PlaybackTimingMode mode) {
    switch (mode) {
    case PlaybackTimingMode::HostReceiveEpochUs:
        return "Imported DVR dataset (host high-resolution timing)";
    case PlaybackTimingMode::LegacyServiceTimestamp:
        return "Imported DVR dataset (legacy service timestamp timing)";
    case PlaybackTimingMode::SyntheticFrameIndex:
    default:
        return "Imported DVR dataset (synthetic frame timing fallback)";
    }
}

} // namespace

bool ServiceProxy::GetLatestFrame(Solvers::HeatmapFrame& out) {
    if (!m_hasNewFrame.load()) return false;
    std::lock_guard<std::mutex> lk(m_frameMutex);
    out = m_latestFrame;
    m_hasNewFrame.store(false);
    return true;
}

bool ServiceProxy::GetCurrentFrame(Solvers::HeatmapFrame& out) {
    if (m_frameSourceMode.load() == FrameSourceMode::Playback) {
        std::lock_guard<std::mutex> lk(m_playbackMutex);
        if (m_playbackDataset.frames.empty()) return false;
        const size_t index = std::min(m_playbackFrameIndex.load(), m_playbackDataset.frames.size() - 1);
        out = m_playbackDataset.frames[index].frame;
        if (m_masterParserOnly.load(std::memory_order_relaxed)) {
            PopulateRawHeatmapFromFrameData(out);
        }
        return true;
    }
    return GetLatestFrame(out);
}

void ServiceProxy::UpdatePlayback() {
    if (m_frameSourceMode.load() != FrameSourceMode::Playback || !m_playbackPlaying.load()) {
        m_lastPlaybackAdvance = std::chrono::steady_clock::time_point{};
        return;
    }

    std::lock_guard<std::mutex> lk(m_playbackMutex);
    if (m_playbackDataset.frames.empty()) {
        m_playbackPlaying.store(false);
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (m_lastPlaybackAdvance.time_since_epoch().count() == 0) {
        m_lastPlaybackAdvance = now;
        if (m_playbackCurrentTimeUs.load() == 0) {
            m_playbackCurrentTimeUs.store(m_playbackDataset.frames.front().recordingTimeUs);
        }
        return;
    }

    const auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastPlaybackAdvance).count();
    if (elapsedUs <= 0) {
        return;
    }
    m_lastPlaybackAdvance = now;

    const uint64_t startTimeUs = m_playbackDataset.frames.front().recordingTimeUs;
    const uint64_t endTimeUs = m_playbackDataset.frames.back().recordingTimeUs;
    uint64_t nextTimeUs = m_playbackCurrentTimeUs.load();
    if (nextTimeUs == 0) {
        nextTimeUs = startTimeUs;
    }
    nextTimeUs += static_cast<uint64_t>(elapsedUs);
    if (nextTimeUs >= endTimeUs) {
        nextTimeUs = endTimeUs;
        m_playbackPlaying.store(false);
    }

    size_t nextIndex = m_playbackFrameIndex.load();
    while (nextIndex + 1 < m_playbackDataset.frames.size() &&
           m_playbackDataset.frames[nextIndex + 1].recordingTimeUs <= nextTimeUs) {
        ++nextIndex;
    }

    m_playbackFrameIndex.store(nextIndex);
    m_playbackCurrentTimeUs.store(nextTimeUs);
}

bool ServiceProxy::LoadDvrDataset(const std::filesystem::path& inputPath) {
    std::vector<Solvers::HeatmapFrame> importedFrames;
    DvrDynamicDebugSchema importedDynamicSchema;
    std::vector<DvrDynamicDebugFrame> importedDynamicFrames;
    int version = 0;
    uint32_t flags = 0;
    std::string error;
    const auto binPath = ResolveReplayBinaryPath(inputPath);
    if (!ReadDvrBinaryFile(binPath, importedFrames, version, &flags, &error, &importedDynamicSchema, &importedDynamicFrames)) {
        std::lock_guard<std::mutex> lk(m_playbackMutex);
        m_playbackStatusMessage = error.empty() ? "Import failed: invalid or unsupported DVR2 .dvrbin" : ("Import failed: " + error);
        return false;
    }

    DvrPlaybackDataset dataset{};
    dataset.formatVersion = version;
    dataset.flags = flags;
    if (IsStrictlyIncreasingHostTimeline(importedFrames)) {
        dataset.timingMode = PlaybackTimingMode::HostReceiveEpochUs;
    } else if (IsMonotonicServiceTimeline(importedFrames)) {
        dataset.timingMode = PlaybackTimingMode::LegacyServiceTimestamp;
    } else {
        dataset.timingMode = PlaybackTimingMode::SyntheticFrameIndex;
    }

    dataset.dynamicDebugSchema = std::move(importedDynamicSchema);
    dataset.frames.reserve(importedFrames.size());
    for (size_t i = 0; i < importedFrames.size(); ++i) {
        DvrPlaybackFrame playbackFrame{};
        playbackFrame.frame = std::move(importedFrames[i]);
        playbackFrame.sourceTimeUs = playbackFrame.frame.timestamp;
        playbackFrame.hostReceiveUnixTimeUs = playbackFrame.frame.receiveSystemEpochUs;
        playbackFrame.sequence = static_cast<uint64_t>(i);
        if (i < importedDynamicFrames.size()) {
            playbackFrame.dynamicDebug = std::move(importedDynamicFrames[i]);
        }
        switch (dataset.timingMode) {
        case PlaybackTimingMode::HostReceiveEpochUs:
            playbackFrame.recordingTimeUs = playbackFrame.hostReceiveUnixTimeUs;
            break;
        case PlaybackTimingMode::LegacyServiceTimestamp:
            playbackFrame.recordingTimeUs = playbackFrame.sourceTimeUs;
            break;
        case PlaybackTimingMode::SyntheticFrameIndex:
        default:
            playbackFrame.recordingTimeUs = static_cast<uint64_t>(i) * kSyntheticPlaybackStepUs;
            break;
        }
        dataset.frames.push_back(std::move(playbackFrame));
    }

    {
        std::lock_guard<std::mutex> lk(m_playbackMutex);
        m_playbackDataset = std::move(dataset);
        m_playbackStatusMessage = std::string(PlaybackTimingModeLabel(m_playbackDataset.timingMode)) +
            " (format v" + std::to_string(version) + ", flags=" + std::to_string(flags) + ")";
    }
    m_playbackFrameIndex.store(0);
    m_playbackCurrentTimeUs.store(m_playbackDataset.Empty() ? 0 : m_playbackDataset.frames.front().recordingTimeUs);
    m_playbackPlaying.store(false);
    m_playbackFormatVersion.store(version);
    m_playbackFlags.store(flags);
    m_frameSourceMode.store(m_playbackDataset.Empty() ? FrameSourceMode::Live : FrameSourceMode::Playback);
    m_lastPlaybackAdvance = std::chrono::steady_clock::time_point{};
    return !m_playbackDataset.Empty();
}

void ServiceProxy::UnloadDvrDataset() {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    m_playbackDataset.frames.clear();
    m_playbackStatusMessage.clear();
    m_playbackFrameIndex.store(0);
    m_playbackCurrentTimeUs.store(0);
    m_playbackPlaying.store(false);
    m_playbackFormatVersion.store(0);
    m_playbackFlags.store(0);
    m_frameSourceMode.store(FrameSourceMode::Live);
    m_lastPlaybackAdvance = std::chrono::steady_clock::time_point{};
}

bool ServiceProxy::HasPlaybackDataset() const {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    return !m_playbackDataset.frames.empty();
}

void ServiceProxy::SetFrameSourceMode(FrameSourceMode mode) {
    if (mode == FrameSourceMode::Playback && !HasPlaybackDataset()) {
        return;
    }
    m_frameSourceMode.store(mode);
    if (mode == FrameSourceMode::Live) {
        m_playbackPlaying.store(false);
        m_lastPlaybackAdvance = std::chrono::steady_clock::time_point{};
    } else {
        std::lock_guard<std::mutex> lk(m_playbackMutex);
        if (!m_playbackDataset.frames.empty()) {
            const size_t index = std::min(m_playbackFrameIndex.load(), m_playbackDataset.frames.size() - 1);
            m_playbackCurrentTimeUs.store(m_playbackDataset.frames[index].recordingTimeUs);
        }
    }
}

void ServiceProxy::PlayPlayback() {
    if (!HasPlaybackDataset()) return;
    {
        std::lock_guard<std::mutex> lk(m_playbackMutex);
        if (!m_playbackDataset.frames.empty()) {
            const size_t index = std::min(m_playbackFrameIndex.load(), m_playbackDataset.frames.size() - 1);
            m_playbackCurrentTimeUs.store(m_playbackDataset.frames[index].recordingTimeUs);
        }
    }
    m_lastPlaybackAdvance = std::chrono::steady_clock::time_point{};
    m_frameSourceMode.store(FrameSourceMode::Playback);
    m_playbackPlaying.store(true);
}

void ServiceProxy::PausePlayback() {
    m_playbackPlaying.store(false);
    m_lastPlaybackAdvance = std::chrono::steady_clock::time_point{};
}

void ServiceProxy::StepPlaybackForward() {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    if (m_playbackDataset.frames.empty()) return;
    const size_t current = m_playbackFrameIndex.load();
    size_t next = current;
    if (next + 1 < m_playbackDataset.frames.size()) {
        ++next;
    }
    m_playbackFrameIndex.store(next);
    m_playbackCurrentTimeUs.store(m_playbackDataset.frames[next].recordingTimeUs);
    m_frameSourceMode.store(FrameSourceMode::Playback);
    m_playbackPlaying.store(false);
    m_lastPlaybackAdvance = std::chrono::steady_clock::time_point{};
}

void ServiceProxy::StepPlaybackBackward() {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    if (m_playbackDataset.frames.empty()) return;
    size_t current = m_playbackFrameIndex.load();
    if (current > 0) {
        --current;
    }
    m_playbackFrameIndex.store(current);
    m_playbackCurrentTimeUs.store(m_playbackDataset.frames[current].recordingTimeUs);
    m_frameSourceMode.store(FrameSourceMode::Playback);
    m_playbackPlaying.store(false);
    m_lastPlaybackAdvance = std::chrono::steady_clock::time_point{};
}

void ServiceProxy::SeekPlaybackFrame(size_t index) {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    if (m_playbackDataset.frames.empty()) return;
    const size_t resolved = std::min(index, m_playbackDataset.frames.size() - 1);
    m_playbackFrameIndex.store(resolved);
    m_playbackCurrentTimeUs.store(m_playbackDataset.frames[resolved].recordingTimeUs);
    m_frameSourceMode.store(FrameSourceMode::Playback);
    m_playbackPlaying.store(false);
    m_lastPlaybackAdvance = std::chrono::steady_clock::time_point{};
}

void ServiceProxy::SeekPlaybackTimeUs(uint64_t timeUs) {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    if (m_playbackDataset.frames.empty()) return;

    const uint64_t startTimeUs = m_playbackDataset.frames.front().recordingTimeUs;
    const uint64_t endTimeUs = m_playbackDataset.frames.back().recordingTimeUs;
    const uint64_t clampedTimeUs = std::clamp(timeUs, startTimeUs, endTimeUs);

    size_t resolved = 0;
    while (resolved + 1 < m_playbackDataset.frames.size() &&
           m_playbackDataset.frames[resolved + 1].recordingTimeUs <= clampedTimeUs) {
        ++resolved;
    }

    m_playbackFrameIndex.store(resolved);
    m_playbackCurrentTimeUs.store(clampedTimeUs);
    m_frameSourceMode.store(FrameSourceMode::Playback);
    m_playbackPlaying.store(false);
    m_lastPlaybackAdvance = std::chrono::steady_clock::time_point{};
}

size_t ServiceProxy::GetPlaybackFrameCount() const {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    return m_playbackDataset.frames.size();
}

uint64_t ServiceProxy::GetPlaybackStartTimeUs() const {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    return m_playbackDataset.frames.empty() ? 0 : m_playbackDataset.frames.front().recordingTimeUs;
}

uint64_t ServiceProxy::GetPlaybackEndTimeUs() const {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    return m_playbackDataset.frames.empty() ? 0 : m_playbackDataset.frames.back().recordingTimeUs;
}

uint64_t ServiceProxy::GetPlaybackCurrentSourceTimeUs() const {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    if (m_playbackDataset.frames.empty()) return 0;
    const size_t index = std::min(m_playbackFrameIndex.load(), m_playbackDataset.frames.size() - 1);
    return m_playbackDataset.frames[index].sourceTimeUs;
}

uint64_t ServiceProxy::GetPlaybackCurrentHostReceiveTimeUs() const {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    if (m_playbackDataset.frames.empty()) return 0;
    const size_t index = std::min(m_playbackFrameIndex.load(), m_playbackDataset.frames.size() - 1);
    return m_playbackDataset.frames[index].hostReceiveUnixTimeUs;
}

PlaybackTimingMode ServiceProxy::GetPlaybackTimingMode() const {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    return m_playbackDataset.timingMode;
}

std::string ServiceProxy::GetPlaybackStatusMessage() const {
    std::lock_guard<std::mutex> lk(m_playbackMutex);
    return m_playbackStatusMessage;
}

} // namespace App
