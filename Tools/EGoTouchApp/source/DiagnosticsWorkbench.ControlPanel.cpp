#include "DiagnosticsWorkbench.h"
#include "ServiceProxy.h"
#include "himax/AfeTypes.h"
#include "imgui.h"
#include "Logger.h"
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <algorithm>
#include <commdlg.h>

namespace App {

namespace {
constexpr const char* kExportRootDir = "C:/ProgramData/EGoTouchRev/exports";
} // namespace

void DiagnosticsWorkbench::DrawControlPanel() {
    ImGui::Begin("Control Panel");

    const bool allowLiveControl = m_proxy && m_proxy->IsLiveControlAllowed();
    if (!ImGui::BeginTabBar("ControlTabs")) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabItem("Service")) {

    if (ImGui::Button("EXIT APPLICATION", ImVec2(-1, 30))) {
        if (m_proxy) {
            m_proxy->Disconnect();
        }
        ::PostQuitMessage(0);
    }
    ImGui::Separator();

    if (m_proxy) {
        // ── Service 连接状态 + 手动连接按钮 ──
        bool connected = m_proxy->IsConnected();
        const bool allowLiveControl = m_proxy->IsLiveControlAllowed();
        if (connected) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.f), "● Service: Connected");
        } else {
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.f), "● Service: Disconnected");
            ImGui::TextWrapped("Service is disconnected. Click Connect to Service to try a connection.");
        }

        if (connected) {
            // Disconnect button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.f));
            if (ImGui::Button("Disconnect from Service", ImVec2(-1, 0))) {
                m_proxy->Disconnect();
            }
            ImGui::PopStyleColor();

            ImGui::Separator();

            // Start/Stop Runtime
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.f));
            if (ImGui::Button("Stop Runtime", ImVec2(-1, 0))) {
                m_proxy->StopRemoteRuntime();
            }
            ImGui::PopStyleColor();
        } else {
            // Manual connect button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.f));
            if (ImGui::Button("Connect to Service", ImVec2(-1, 0))) {
                m_proxy->TryConnect();
            }
            ImGui::PopStyleColor();
        }

        ImGui::Separator();

        if (!connected || !allowLiveControl) ImGui::BeginDisabled();

        auto clamp_u8 = [](int value) -> uint8_t {
            return static_cast<uint8_t>(std::clamp(value, 0, 255));
        };
        auto send_afe_command = [&](const char* actionName, AFE_Command cmd, int paramValue) {
            const uint8_t param = clamp_u8(paramValue);
            bool ok = m_proxy->SwitchAfeMode(static_cast<uint8_t>(cmd), param);
            if (ok) {
                m_lastAfeActionStatus = std::string(actionName) + " success (param=" +
                                        std::to_string(static_cast<unsigned int>(param)) + ")";
            } else {
                m_lastAfeActionStatus = std::string(actionName) + " failed (param=" +
                                        std::to_string(static_cast<unsigned int>(param)) + ")";
            }
        };

            ImGui::TextUnformatted("AFE Mode Control");
            m_afeIdleParam = std::clamp(m_afeIdleParam, 0, 255);
            ImGui::InputInt("Idle Param", &m_afeIdleParam);
            if (ImGui::Button("Enter Idle")) {
                send_afe_command("EnterIdle", AFE_Command::EnterIdle, m_afeIdleParam);
            }
            ImGui::SameLine();
            if (ImGui::Button("Exit Idle")) {
                send_afe_command("ForceExitIdle", AFE_Command::ForceExitIdle, 0);
            }

            m_afeCalibrationParam = std::clamp(m_afeCalibrationParam, 0, 255);
            ImGui::InputInt("Calibration Param", &m_afeCalibrationParam);
            if (ImGui::Button("Start Calibration")) {
                send_afe_command("StartCalibration", AFE_Command::StartCalibration, m_afeCalibrationParam);
            }

            m_afeClearStatusParam = std::clamp(m_afeClearStatusParam, 0, 255);
            ImGui::InputInt("Clear Status Param", &m_afeClearStatusParam);
            if (ImGui::Button("Clear Status")) {
                send_afe_command("ClearStatus", AFE_Command::ClearStatus, m_afeClearStatusParam);
            }

            m_afeForceScanRateIdx = std::clamp(m_afeForceScanRateIdx, 0, 255);
            ImGui::InputInt("Force Scan Rate Idx", &m_afeForceScanRateIdx);
            if (ImGui::Button("Force To Scan Rate")) {
                send_afe_command("ForceToScanRate", AFE_Command::ForceToScanRate, m_afeForceScanRateIdx);
            }

            // --- 120Hz / 240Hz 快速切换 ---
            ImGui::Separator();
            {
                const ImVec4 col120(0.15f, 0.7f, 0.25f, 1.f); // green
                const ImVec4 col240(0.8f, 0.4f, 0.1f, 1.f);   // orange
                const char*  label      = m_scanRateIs240Hz ? "Switch to 120 Hz" : "Switch to 240 Hz";
                const ImVec4 btnColor   = m_scanRateIs240Hz ? col120 : col240;
                const char*  stateLabel = m_scanRateIs240Hz ? "Current: 240 Hz" : "Current: 120 Hz";
                const ImVec4 stateColor = m_scanRateIs240Hz ? col240 : col120;

                ImGui::TextColored(stateColor, "%s", stateLabel);
                ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
                if (ImGui::Button(label, ImVec2(-1, 0))) {
                    m_scanRateIs240Hz = !m_scanRateIs240Hz;
                    const uint8_t rateIdx = m_scanRateIs240Hz ? 1 : 0;
                    send_afe_command(m_scanRateIs240Hz ? "ScanRate240Hz" : "ScanRate120Hz",
                                     AFE_Command::ForceToScanRate, rateIdx);
                }
                ImGui::PopStyleColor();
            }

            ImGui::TextWrapped("AFE Last Action: %s", m_lastAfeActionStatus.c_str());

            if (!connected || !allowLiveControl) ImGui::EndDisabled();
    }

    // Start/Stop Runtime toggle
    if (m_proxy && m_proxy->IsConnected()) {
        if (!allowLiveControl) ImGui::BeginDisabled();
        if (ImGui::Button("Toggle Runtime")) {
            m_proxy->StartRemoteRuntime();
        }
        if (!allowLiveControl) ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (!allowLiveControl) ImGui::BeginDisabled();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    if (ImGui::Button("Save Global Parameters")) {
        m_proxy->SaveConfig();
    }
    ImGui::PopStyleColor();
    if (!allowLiveControl) ImGui::EndDisabled();

    bool masterParserOnly = (m_proxy != nullptr) && m_proxy->IsMasterParserOnlyMode();
    if (!m_proxy || !allowLiveControl) ImGui::BeginDisabled();
    if (ImGui::Checkbox("Master Parser Only (Raw Heatmap)", &masterParserOnly) && m_proxy) {
        m_proxy->SetMasterParserOnlyMode(masterParserOnly);
    }
    if (!m_proxy || !allowLiveControl) ImGui::EndDisabled();
    if (masterParserOnly) {
        ImGui::TextUnformatted("Enabled: only Master Frame Parser is active.");
    }

    ImGui::Separator();
    bool vhfReportingEnabled = (m_proxy != nullptr) && m_proxy->IsVhfEnabled();
    if (!m_proxy || !allowLiveControl) ImGui::BeginDisabled();
    if (ImGui::Checkbox("Enable VHF Output", &vhfReportingEnabled) && m_proxy) {
        m_proxy->SetVhfEnabled(vhfReportingEnabled);
    }
    if (m_proxy) {
        ImGui::Text("VHF: %s", m_proxy->IsVhfEnabled() ? "Enabled" : "Disabled");
        bool vhfTranspose = m_proxy->IsVhfTransposeEnabled();
        if (ImGui::Checkbox("Flip VHF Orientation", &vhfTranspose)) {
            m_proxy->SetVhfTranspose(vhfTranspose);
        }
    }
    if (!m_proxy || !allowLiveControl) ImGui::EndDisabled();

    // Global Service Options
    ImGui::Separator();
    ImGui::TextUnformatted("Global Config");
    if (m_proxy) {
        if (!allowLiveControl) ImGui::BeginDisabled();
        const bool desiredFull = m_proxy->IsSrvModeFull();
        const bool activeFull = m_proxy->IsSrvActiveModeFull();
        bool isTouchOnly = !desiredFull;
        if (ImGui::Checkbox("Touch-Only Mode (Pure Finger, Disable Pen)", &isTouchOnly)) {
            m_proxy->SetSrvModeFull(!isTouchOnly);
        }
        const char* activeModeText = activeFull ? "Full" : "Touch-Only";
        const char* desiredModeText = desiredFull ? "Full" : "Touch-Only";
        if (desiredFull == activeFull) {
            ImGui::TextDisabled("Mode: active=%s", activeModeText);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                               "Mode pending: desired=%s, active=%s",
                               desiredModeText,
                               activeModeText);
        }

        bool autoMode = m_proxy->IsSrvAutoMode();
        if (ImGui::Checkbox("Auto-Mode (Hardware handshake)", &autoMode)) {
            m_proxy->SetSrvAutoMode(autoMode);
        }
        if (!allowLiveControl) ImGui::EndDisabled();
    }


    ImGui::Separator();
    ImGui::Text("Auto-Capture (Debug)");
    const bool replayModeNow = m_proxy && (m_proxy->GetFrameSourceMode() == FrameSourceMode::Playback);
    if (replayModeNow && m_autoCaptureMode != 0) {
        m_autoCaptureMode = 0;
    }
    if (replayModeNow) ImGui::BeginDisabled();
    const char* captureModes[] = { "Disabled", "Peak Appear", "Touch Drop" };
    ImGui::Combo("Capture Mode", &m_autoCaptureMode, captureModes, IM_ARRAYSIZE(captureModes));
    if (m_autoCaptureMode == 1) {
        ImGui::SliderInt("Target Peaks", &m_autoExportTargetPeaks, 1, 5, "%d Peaks");
    } else if (m_autoCaptureMode == 2) {
        ImGui::TextWrapped("Captures the frame when all tracked contacts disappear (peaks drop to 0 from >0).");
    }
    if (replayModeNow) {
        ImGui::EndDisabled();
        ImGui::TextDisabled("Auto-capture is disabled in Replay mode.");
    }



    ImGui::Checkbox("Auto-refresh Heatmap", &m_autoRefresh);
    ImGui::Checkbox("Render Visualization", &m_renderVisualization);
    if (!m_renderVisualization) {
        ImGui::TextUnformatted("Visualization disabled: acquisition/processing threads keep running.");
    }
    ImGui::Checkbox("Fullscreen Heatmap", &m_fullscreen);
    ImGui::SliderInt("Heatmap Scale", &m_heatmapScale, 1, 30);
    ImGui::SliderFloat("Color Max Range", &m_colorRange, 100.0f, 10000.0f, "%.0f");

    ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("DVR")) {
        DrawDvrPanel();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::End();
}

} // namespace App
