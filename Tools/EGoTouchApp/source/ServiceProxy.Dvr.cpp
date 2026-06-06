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
#include <unordered_map>
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

Dvr::DvrDynamicDebugFrameSlot MakeInvalidDynamicDebugFrameSlot(
    uint64_t dvrSeq,
    const DvrDynamicDebugSchema& schema) {
    Dvr::DvrDynamicDebugFrameSlot slot{};
    slot.dvrSeq = dvrSeq;
    const size_t sampleCount = std::min(schema.fields.size(), static_cast<size_t>(Dvr::kMaxDynamicDebugSamples));
    slot.sampleCount = static_cast<uint16_t>(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        slot.samples[i].fieldId = schema.fields[i].fieldId;
        slot.samples[i].valueType = static_cast<uint8_t>(schema.fields[i].valueType);
        slot.samples[i].valid = 0;
        slot.samples[i].rawValue = 0;
    }
    return slot;
}

bool DynamicDebugFrameSlotMatchesSchema(const Dvr::DvrDynamicDebugFrameSlot& slot,
                                        const DvrDynamicDebugSchema& schema) {
    if (slot.sampleCount != schema.fields.size()) return false;
    for (size_t i = 0; i < schema.fields.size(); ++i) {
        if (slot.samples[i].fieldId != schema.fields[i].fieldId) return false;
        if (static_cast<Ipc::DebugValueType>(slot.samples[i].valueType) != schema.fields[i].valueType) return false;
    }
    return true;
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

        std::vector<Dvr::DvrDynamicDebugFrameSlot> dynamicFramesForExport;
        const auto dynamicSchema = GetCurrentDvrDynamicDebugSchema();
        bool canExportDynamicFrames = m_dvrDynamicDebugBuffer &&
            !dynamicSchema.Empty() &&
            dynamicSchema.fields.size() <= static_cast<size_t>(Dvr::kMaxDynamicDebugSamples);
        if (canExportDynamicFrames) {
            const auto dynamicSnapshot = m_dvrDynamicDebugBuffer->GetSnapshot();
            std::unordered_map<uint64_t, const Dvr::DvrDynamicDebugFrameSlot*> dynamicBySeq;
            dynamicBySeq.reserve(dynamicSnapshot.size());
            for (const auto& frame : dynamicSnapshot) {
                dynamicBySeq[frame.dvrSeq] = &frame;
            }

            dynamicFramesForExport.reserve(preTriggerFrames.size());
            for (const auto& frame : preTriggerFrames) {
                auto it = dynamicBySeq.find(frame.dvrSeq);
                if (it == dynamicBySeq.end()) {
                    dynamicFramesForExport.push_back(MakeInvalidDynamicDebugFrameSlot(frame.dvrSeq, dynamicSchema));
                    continue;
                }
                if (!DynamicDebugFrameSlotMatchesSchema(*it->second, dynamicSchema)) {
                    dynamicFramesForExport.clear();
                    canExportDynamicFrames = false;
                    break;
                }
                dynamicFramesForExport.push_back(*it->second);
            }
        }
        const auto* dynamicFrames = (canExportDynamicFrames && dynamicFramesForExport.size() == preTriggerFrames.size())
            ? &dynamicFramesForExport
            : nullptr;

        const fs::path replayBinPath = dir / (datasetName + ".dvrbin");
        const auto runtimeConfig = CaptureRuntimeConfigSnapshot();
        if (!WriteDvrBinaryFile(replayBinPath, preTriggerFrames, &dynamicSchema, dynamicFrames, &runtimeConfig, nullptr)) {
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
