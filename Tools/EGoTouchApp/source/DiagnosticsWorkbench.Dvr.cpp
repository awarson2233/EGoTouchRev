#include "DiagnosticsWorkbench.h"
#include "DiagnosticsWorkbenchInternal.h"
#include "ServiceProxy.h"
#include "imgui.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>

namespace App {

namespace {
constexpr const char* kExportRootDir = "C:/ProgramData/EGoTouchRev/exports";
} // namespace

void DiagnosticsWorkbench::ExitPlaybackToLivePreview() {
    if (!m_proxy) return;
    m_proxy->PausePlayback();
    m_proxy->SetFrameSourceMode(FrameSourceMode::Live);
}

void DiagnosticsWorkbench::ExportSelectedDvrDatasetToCsv() {
    if (!m_proxy) return;

    const std::filesystem::path outputDir = BuildDvrCsvExportPath(kExportRootDir, m_currentFrame.timestamp);

    std::string error;
    if (m_proxy->ExportLoadedDvrDatasetToCsv(outputDir, &error)) {
        m_lastCsvExportStatus = "Exported loaded DVR dataset CSV to " + outputDir.string();
    } else {
        m_lastCsvExportStatus = error.empty() ? "Failed to export loaded DVR dataset CSV." : error;
    }
}

void DiagnosticsWorkbench::DrawDvrPanel() {
    if (!m_proxy) {
        ImGui::TextUnformatted("ServiceProxy unavailable.");
        return;
    }

    const bool allowLiveControl = m_proxy->IsLiveControlAllowed();
    const bool hasPlayback = m_proxy->HasPlaybackDataset();
    const bool playbackModeNow = (m_proxy->GetFrameSourceMode() == FrameSourceMode::Playback);

    ImGui::TextUnformatted("Binary DVR Export");
    if (!allowLiveControl) {
        ImGui::TextDisabled("Live DVR capture is disabled in Replay mode.");
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Export DVR Binary (Pre-trigger 480)")) {
        m_proxy->TriggerDvrBinaryExport();
    }
    if (m_proxy->IsDvrExporting()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.f), "Exporting binary...");
    }
    if (!allowLiveControl) {
        ImGui::EndDisabled();
    }

    ImGui::Separator();
    ImGui::Text("Auto-Capture (Debug)");
    if (playbackModeNow && m_autoCaptureMode != 0) {
        m_autoCaptureMode = 0;
    }
    if (playbackModeNow) ImGui::BeginDisabled();
    const char* captureModes[] = { "Disabled", "Peak Appear", "Touch Drop" };
    ImGui::Combo("Capture Mode", &m_autoCaptureMode, captureModes, IM_ARRAYSIZE(captureModes));
    if (m_autoCaptureMode == 1) {
        ImGui::SliderInt("Target Peaks", &m_autoExportTargetPeaks, 1, 5, "%d Peaks");
    } else if (m_autoCaptureMode == 2) {
        ImGui::TextWrapped("Captures the frame when all tracked contacts disappear (peaks drop to 0 from >0).");
    }
    if (playbackModeNow) {
        ImGui::EndDisabled();
        ImGui::TextDisabled("Auto-capture is disabled in Replay mode.");
    }

    ImGui::Separator();
    ImGui::TextUnformatted("DVR Playback");
    ImGui::TextWrapped("Import root: %s", kExportRootDir);

    if (ImGui::Button("Import DVR Dataset")) {
        wchar_t filePath[MAX_PATH] = {};
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFile = filePath;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = L"DVR Replay Binary\0*.dvrbin\0\0";
        ofn.nFilterIndex = 1;
        const std::wstring initialDir = m_dvrImportDirectory.wstring();
        ofn.lpstrInitialDir = initialDir.c_str();
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        ofn.lpstrTitle = L"Select DVR Export File";
        if (GetOpenFileNameW(&ofn)) {
            const std::filesystem::path selectedFile(filePath);
            const auto validation = ValidateDvrImportSelection(m_dvrImportDirectory, selectedFile);
            if (!validation.ok) {
                m_lastDvrImportStatus = validation.status;
            } else {
                const bool ok = m_proxy->LoadDvrDataset(validation.canonicalPath);
                m_lastDvrImportStatus = ok
                    ? m_proxy->GetPlaybackStatusMessage()
                    : m_proxy->GetPlaybackStatusMessage();
            }
        }
    }

    if (!m_lastDvrImportStatus.empty()) {
        ImGui::TextWrapped("%s", m_lastDvrImportStatus.c_str());
    }

    if (hasPlayback) {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.f),
                           "Dataset: DVR2 (format v%d, flags=%u)",
                           m_proxy->GetPlaybackFormatVersion(),
                           static_cast<unsigned int>(m_proxy->GetPlaybackFlags()));
    }

    bool playbackMode = playbackModeNow;
    if (!hasPlayback) ImGui::BeginDisabled();
    if (ImGui::Checkbox("Playback Mode", &playbackMode)) {
        if (playbackMode) {
            m_proxy->SetFrameSourceMode(FrameSourceMode::Playback);
        } else {
            ExitPlaybackToLivePreview();
        }
    }
    if (!hasPlayback) ImGui::EndDisabled();

    if (playbackModeNow) {
        ImGui::SameLine();
        if (ImGui::Button("Return to Live Preview")) {
            ExitPlaybackToLivePreview();
        }
    }

    if (!hasPlayback) ImGui::BeginDisabled();
    const bool isPlaying = m_proxy->IsPlaybackPlaying();
    if (ImGui::Button(isPlaying ? "Pause" : "Play")) {
        if (isPlaying) m_proxy->PausePlayback();
        else m_proxy->PlayPlayback();
    }
    ImGui::SameLine();
    if (ImGui::Button("Prev Frame")) {
        m_proxy->StepPlaybackBackward();
    }
    ImGui::SameLine();
    if (ImGui::Button("Next Frame")) {
        m_proxy->StepPlaybackForward();
    }

    const int totalFrames = static_cast<int>(m_proxy->GetPlaybackFrameCount());
    int currentFrame = static_cast<int>(m_proxy->GetPlaybackFrameIndex());
    if (totalFrames > 0) {
        const uint64_t playbackStartUs = m_proxy->GetPlaybackStartTimeUs();
        const uint64_t playbackEndUs = m_proxy->GetPlaybackEndTimeUs();

        const int maxFrameIndex = totalFrames - 1;
        if (ImGui::SliderInt("Frame", &currentFrame, 0, maxFrameIndex)) {
            m_proxy->SeekPlaybackFrame(static_cast<size_t>(currentFrame));
            currentFrame = static_cast<int>(m_proxy->GetPlaybackFrameIndex());
        }

        const char* timingModeText = "Synthetic frame timing";
        switch (m_proxy->GetPlaybackTimingMode()) {
        case PlaybackTimingMode::HostReceiveEpochUs:
            timingModeText = "Host high-resolution timing";
            break;
        case PlaybackTimingMode::LegacyServiceTimestamp:
            timingModeText = "Legacy service timestamp";
            break;
        case PlaybackTimingMode::SyntheticFrameIndex:
        default:
            timingModeText = "Synthetic frame timing";
            break;
        }

        ImGui::Text("Playback: %d / %d | Playback Timeline Us: %llu / [%llu, %llu]",
                    currentFrame + 1,
                    totalFrames,
                    static_cast<unsigned long long>(m_proxy->GetPlaybackCurrentTimeUs()),
                    static_cast<unsigned long long>(playbackStartUs),
                    static_cast<unsigned long long>(playbackEndUs));
        ImGui::Text("Host Receive Epoch Us: %llu | Service Timestamp (raw): %llu",
                    static_cast<unsigned long long>(m_proxy->GetPlaybackCurrentHostReceiveTimeUs()),
                    static_cast<unsigned long long>(m_proxy->GetPlaybackCurrentSourceTimeUs()));
        ImGui::Text("Replay timing mode: %s", timingModeText);
    } else {
        ImGui::TextUnformatted("No playback dataset loaded.");
    }
    if (!hasPlayback) ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::TextUnformatted("CSV Export");

    if (!hasPlayback) ImGui::BeginDisabled();
    if (ImGui::Button("Convert Loaded DVR Dataset to CSV")) {
        ExportSelectedDvrDatasetToCsv();
    }
    if (!hasPlayback) ImGui::EndDisabled();

    if (ImGui::Button("Export Current Frame CSV")) {
        ExportCurrentFrameCsv(false);
    }

    if (!m_lastCsvExportStatus.empty()) {
        ImGui::TextWrapped("%s", m_lastCsvExportStatus.c_str());
    }
}

} // namespace App
