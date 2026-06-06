#include "ServiceProxy.h"
#include "DvrBinaryIO.h"
#include "DvrCsvExport.h"
#include "Logger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <vector>

namespace App {

namespace {

constexpr const char* kExportRootDir = "C:/ProgramData/EGoTouchRev/exports";

std::string MakeDatasetTimestampString() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &t);
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y%m%d_%H%M%S")
       << '_' << std::setfill('0') << std::setw(6) << us.count();
    return ts.str();
}

std::filesystem::path MakeDvrExportRoot() {
    namespace fs = std::filesystem;
    fs::path dir(kExportRootDir);
    dir /= "dvr";
    return dir;
}

std::string MakeDvrDatasetName() {
    return "dvr" + MakeDatasetTimestampString();
}

} // namespace

bool ServiceProxy::ExportLoadedDvrDatasetToCsv(const std::filesystem::path& outputDirectory,
                                               std::string* outError) const {
    DvrPlaybackDataset dataset;
    {
        std::lock_guard<std::mutex> lk(m_playbackMutex);
        if (m_playbackDataset.Empty()) {
            if (outError) *outError = "No playback dataset loaded.";
            return false;
        }
        dataset = m_playbackDataset;
    }

    std::error_code ec;
    std::filesystem::create_directories(outputDirectory, ec);
    if (ec) {
        if (outError) *outError = "Failed to create CSV directory.";
        return false;
    }

    for (size_t i = 0; i < dataset.frames.size(); ++i) {
        std::ostringstream name;
        name << "frame_" << std::setfill('0') << std::setw(4) << i << ".csv";
        const std::filesystem::path path = outputDirectory / name.str();
        const bool ok = WriteFrameCsvFile(path,
                                          dataset.frames[i].frame,
                                          nullptr,
                                          true,
                                          true,
                                          true,
                                          "PlaybackDataset",
                                          true,
                                          dataset.formatVersion,
                                          outputDirectory.filename().string(),
                                          &dataset.dynamicDebugSchema,
                                          &dataset.frames[i].dynamicDebug,
                                          &dataset.runtimeConfig);
        if (!ok) {
            if (outError) *outError = "Failed to write CSV frame file.";
            return false;
        }
    }

    std::ofstream manifest((outputDirectory / "dataset_manifest.csv").string());
    if (!manifest.is_open()) {
        if (outError) *outError = "Failed to write dataset manifest.";
        return false;
    }
    const char* timingModeText = "SyntheticFrameIndex";
    switch (dataset.timingMode) {
    case PlaybackTimingMode::HostReceiveEpochUs:
        timingModeText = "HostReceiveEpochUs";
        break;
    case PlaybackTimingMode::LegacyServiceTimestamp:
        timingModeText = "LegacyServiceTimestamp";
        break;
    case PlaybackTimingMode::SyntheticFrameIndex:
    default:
        timingModeText = "SyntheticFrameIndex";
        break;
    }
    manifest << "FrameCount," << dataset.frames.size() << "\n";
    manifest << "DvrFormatVersion," << dataset.formatVersion << "\n";
    manifest << "DvrFlags," << dataset.flags << "\n";
    manifest << "ConfigSnapshotPresent," << (dataset.runtimeConfig.Empty() ? 0 : 1) << "\n";
    manifest << "ConfigFieldCount," << (dataset.runtimeConfig.Empty() ? 0 : dataset.runtimeConfig.fields.size()) << "\n";
    manifest << "ConfigSchemaHash," << (dataset.runtimeConfig.Empty() ? 0 : dataset.runtimeConfig.schemaHash) << "\n";
    manifest << "DatasetKind,DVR2\n";
    manifest << "PlaybackTimingMode," << timingModeText << "\n";
    manifest << "PlaybackFirstTimeUs," << dataset.frames.front().recordingTimeUs << "\n";
    manifest << "PlaybackLastTimeUs," << dataset.frames.back().recordingTimeUs << "\n";
    manifest << "ServiceFirstTimestampRaw," << dataset.frames.front().sourceTimeUs << "\n";
    manifest << "ServiceLastTimestampRaw," << dataset.frames.back().sourceTimeUs << "\n";
    manifest << "HostReceiveFirstEpochUs," << dataset.frames.front().hostReceiveUnixTimeUs << "\n";
    manifest << "HostReceiveLastEpochUs," << dataset.frames.back().hostReceiveUnixTimeUs << "\n";
    return true;
}

void ServiceProxy::TriggerDvrBinaryExport() {
    if (!m_dvrBuffer) return;
    if (m_dvrExporting.load()) return;

    if (m_dvrThread.joinable()) m_dvrThread.join();

    const uint64_t triggerSeq = m_dvrSeqCounter.load(std::memory_order_relaxed);
    m_dvrExporting.store(true);
    m_dvrThread = std::thread([this, triggerSeq]() {
        auto frames = m_dvrBuffer->GetSnapshot();
        if (frames.empty()) {
            m_dvrExporting.store(false);
            return;
        }
        namespace fs = std::filesystem;
        const fs::path dir = MakeDvrExportRoot();
        const std::string datasetName = MakeDvrDatasetName();
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) {
            LOG_ERROR("App", "TriggerDvrBinaryExport", "IPC", "Failed to create export directory: {}", dir.string());
            m_dvrExporting.store(false);
            return;
        }

        std::vector<Dvr::DvrFrameSlot> preTriggerFrames;
        preTriggerFrames.reserve(frames.size());
        for (const auto& fr : frames) {
            if (fr.dvrSeq <= triggerSeq) {
                preTriggerFrames.push_back(fr);
            }
        }
        if (preTriggerFrames.empty()) {
            LOG_WARN("App", "TriggerDvrBinaryExport", "IPC", "No pre-trigger frames available for export (triggerSeq={}).", triggerSeq);
            m_dvrExporting.store(false);
            return;
        }
        if (preTriggerFrames.size() > kDvrPreTriggerFrames) {
            const size_t dropCount = preTriggerFrames.size() - kDvrPreTriggerFrames;
            preTriggerFrames.erase(preTriggerFrames.begin(), preTriggerFrames.begin() + static_cast<long long>(dropCount));
        }

        const fs::path replayBinPath = dir / (datasetName + ".dvrbin");
        const auto dynamicSchema = GetCurrentDvrDynamicDebugSchema();
        const auto runtimeConfig = CaptureRuntimeConfigSnapshot();
        if (!WriteDvrBinaryFile(replayBinPath, preTriggerFrames, &dynamicSchema, nullptr, &runtimeConfig, nullptr)) {
            LOG_ERROR("App", "TriggerDvrBinaryExport", "IPC", "Failed to write DVR2 dataset: {}", replayBinPath.string());
            m_dvrExporting.store(false);
            return;
        }

        LOG_INFO("App", "TriggerDvrBinaryExport", "IPC",
                 "Exported {} pre-trigger frames to {} (triggerSeq={})",
                 preTriggerFrames.size(), replayBinPath.string(), triggerSeq);
        m_dvrExporting.store(false);
    });
}

} // namespace App
