#include "DiagnosticsWorkbench.h"
#include "ServiceProxy.h"
#include "ConfigUIRenderer.h"
#include "GuiLogSink.h"
#include "SystemStateMonitor.h"
#include "imgui.h"
#include "Logger.h"
#include "StylusSolver/AsaTypes.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace App {

void DiagnosticsWorkbench::DrawInspectorPanel() {
    ImGui::Begin("Inspector");
    bool masterParserOnly = (m_proxy != nullptr) && m_proxy->IsMasterParserOnlyMode();
    // Level 1: category tabs
    if (ImGui::BeginTabBar("CategoryTabs")) {
        if (ImGui::BeginTabItem("Touch")) {
            if (m_proxy) {
                if (masterParserOnly) ImGui::BeginDisabled();
                auto schema = m_proxy->GetPipeline().GetConfigSchema();

                // Level 2: per-module sub-tabs
                if (ImGui::BeginTabBar("TouchModuleTabs")) {
                    static const char* moduleTabs[] = {
                        "Frame Parser",
                        "Signal Conditioning",
                        "Peak Detection",
                        "Zone & Contact",
                        "Palm Rejection",
                        "Tracking",
                        "Stylus Suppress",
                        "Coordinate Filter",
                        "Gesture",
                    };
                    for (const char* mod : moduleTabs) {
                        if (ImGui::BeginTabItem(mod)) {
                            ConfigUIRenderer::RenderConfigSchemaByModule(schema, mod);
                            // Show save button per tab
                            ImGui::Separator();
                            if (ImGui::Button("Save & Apply")) {
                                m_proxy->SaveConfig();
                            }
                            ImGui::EndTabItem();
                        }
                    }
                    // Coordinates table (diagnostic, not config)
                    if (ImGui::BeginTabItem("Coordinates")) {
                        DrawCoordinateTable();
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                if (masterParserOnly) ImGui::EndDisabled();
            } else {
                ImGui::TextUnformatted("ServiceProxy unavailable.");
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Stylus")) {
            DrawStylusControlPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("BT MCU")) {
            DrawBtMcuPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Dynamic Debug")) {
            DrawDynamicDebugPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("System Events")) {
            DrawSystemEventsPanel();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void DiagnosticsWorkbench::DrawTouchSolverPanel() {
    if (!m_proxy) {
        ImGui::TextUnformatted("ServiceProxy unavailable.");
        return;
    }

    const bool masterParserOnly = m_proxy->IsMasterParserOnlyMode();
    if (masterParserOnly) {
        ImGui::TextUnformatted("Master Parser Only is enabled.");
        ImGui::BeginDisabled();
    }

    // Unified config schema from TouchPipeline
    auto schema = m_proxy->GetPipeline().GetConfigSchema();
    ConfigUIRenderer::RenderConfigSchema(schema, "Touch Solver (Feature Extraction)");

    if (masterParserOnly) {
        ImGui::EndDisabled();
    }
}

void DiagnosticsWorkbench::DrawTouchTrackingPanel() {
    if (!m_proxy) {
        ImGui::TextUnformatted("ServiceProxy unavailable.");
        return;
    }

    const bool masterParserOnly = m_proxy->IsMasterParserOnlyMode();
    if (masterParserOnly) {
        ImGui::TextUnformatted("Master Parser Only is enabled.");
        ImGui::BeginDisabled();
    }

    // Unified config schema from TouchPipeline
    auto schema = m_proxy->GetPipeline().GetConfigSchema();
    ConfigUIRenderer::RenderConfigSchema(schema, "Tracking & Report");

    if (masterParserOnly) {
        ImGui::EndDisabled();
    }

    // --- FPS Stats ---
    ImGui::Separator();
    ImGui::TextUnformatted("Performance");
    if (m_proxy) {
        const int fps = m_proxy->GetAcquisitionFps();
        const int slaveFps = m_proxy->GetSlaveAcquisitionFps();
        auto fpsColor = [](int f) -> ImVec4 {
            if (f >= 100) return ImVec4(0.2f, 0.9f, 0.2f, 1.f);
            if (f >=  50) return ImVec4(1.0f, 0.8f, 0.0f, 1.f);
            return            ImVec4(1.0f, 0.3f, 0.3f, 1.f);
        };
        ImGui::TextColored(fpsColor(fps),       "Master Frame Rate: %d Hz", fps);
        ImGui::TextColored(fpsColor(slaveFps),   "Slave  Frame Rate: %d Hz", slaveFps);
    }
}

void DiagnosticsWorkbench::DrawStylusControlPanel() {
    if (!m_proxy) {
        ImGui::TextUnformatted("ServiceProxy unavailable.");
        return;
    }

    const bool masterParserOnly = m_proxy->IsMasterParserOnlyMode();
    if (masterParserOnly) {
        ImGui::TextUnformatted("Master Parser Only is enabled.");
        ImGui::BeginDisabled();
    }

    // Global VHF Output Switch
    ImGui::TextUnformatted("Windows INK Output (VHF)");
    bool vhfStylus = m_proxy->IsSrvStylusVhfEnabled();
    if (ImGui::Checkbox("Enable Stylus Native Output", &vhfStylus)) {
        m_proxy->SetSrvStylusVhfEnabled(vhfStylus);
    }
    ImGui::Separator();

    // -- Stylus Pipeline Config (official ASA pipeline) --
    if (ImGui::BeginTabBar("StylusSubTabs")) {
        auto schema = m_proxy->GetStylusPipeline().GetConfigSchema();
        const auto& sd = m_currentFrame.stylus;

        if (ImGui::BeginTabItem("Solver (Coordinate)")) {
            ConfigUIRenderer::RenderConfigSchema(schema, "Stylus Solver", Solvers::ConfigParam::Solver);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f,0.85f,1.0f,1.f), "[Coord Breakdown] (dim1=Col=X, dim2=Row=Y)");
            ImGui::Text("  anchorCol(X) = %u   (%u * %.1f = %.1f)",
                sd.diag.anchorCol, sd.diag.anchorCol,
                static_cast<float>(Asa::kCoorUnit),
                static_cast<float>(sd.diag.anchorCol) * Asa::kCoorUnit);
            ImGui::Text("  anchorRow(Y) = %u   (%u * %.1f = %.1f)",
                sd.diag.anchorRow, sd.diag.anchorRow,
                static_cast<float>(Asa::kCoorUnit),
                static_cast<float>(sd.diag.anchorRow) * Asa::kCoorUnit);
            ImGui::Text("  rawDim1(X)   = %d", sd.diag.rawDim1);
            ImGui::Text("  rawDim2(Y)   = %d", sd.diag.rawDim2);
            ImGui::Text("  finalDim1(X) = %d", sd.diag.finalDim1);
            ImGui::Text("  finalDim2(Y) = %d", sd.diag.finalDim2);
            ImGui::Text("  centerOff    = %.1f", sd.diag.centerOff);
            if (sd.diag.valid) {
                ImGui::TextColored(ImVec4(0.2f,0.9f,0.4f,1),
                    "  point.X = anchorCol*kU + finalDim1 - cOff = %.2f", sd.diag.pointX);
                ImGui::TextColored(ImVec4(0.4f,0.7f,1.0f,1),
                    "  point.Y = anchorRow*kU + finalDim2 - cOff = %.2f", sd.diag.pointY);
            } else {
                ImGui::TextColored(ImVec4(0.7f,0.7f,0.0f,1), "  [coord invalid this frame]");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Filters (Smoothing)")) {
            // ── Filter mode dropdown ──
            auto& pipeline = m_proxy->GetStylusPipeline();
            {
                static const char* filterModes[] = { "IIR (TSACore Q8)", "1-Euro Adaptive", "None (Bypass)" };
                int mode = pipeline.GetFilterMode();
                if (ImGui::Combo("Smoothing Filter##sp_filter_mode", &mode, filterModes, 3)) {
                    pipeline.SetFilterMode(mode);
                }
            }

            ImGui::Separator();
            // ── Render all 'Filter' category params (auto-generated) ──
            ConfigUIRenderer::RenderConfigSchema(schema, "Stylus Filters", Solvers::ConfigParam::Filter);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f,0.85f,0.4f,1.f), "[Post-Processing]");
            ImGui::Text("  Speed: instant=%.1f  short=%.1f  full=%.1f",
                sd.diag.speedInstant, sd.diag.speedShortAvg, sd.diag.speedFullAvg);
            ImGui::Text("  IIR Coef: %.3f  %s",
                sd.diag.iirCoef,
                sd.diag.iirCoef < 0.3f ? "(strong smooth)" :
                (sd.diag.iirCoef > 0.8f ? "(fast track)" : "(moderate)"));

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.9f,0.9f,0.5f,1.f), "[Linear Filter]");
            const char* lfStates[] = {
                "Init","Wait","Collect","CurveLine",
                "EnterStraight","StraightLine","ExitStraight"};
            int lfs = std::clamp(static_cast<int>(sd.diag.linearFilterState), 0, 6);
            ImGui::Text("  State: %d (%s)", lfs, lfStates[lfs]);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Behavior (Tilt/Edge)")) {
            ConfigUIRenderer::RenderConfigSchema(schema, "Stylus Behavior", Solvers::ConfigParam::Behavior);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f,0.6f,1.0f,1.f), "[Tilt Diagnostics]");
            ImGui::Text("  TX1-TX2 Diff: dX=%.1f  dY=%.1f", sd.diag.tiltDiffX, sd.diag.tiltDiffY);
            if (sd.diag.tiltAnomalyDamped)
                ImGui::TextColored(ImVec4(1.0f,0.4f,0.4f,1), "  !! Tilt anomaly damping active");
            ImGui::Text("  Signal Ratio (TX2/TX1): %u%%", sd.diag.signalRatio);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f,0.85f,0.4f,1.f), "[Edge/Hover]");
            ImGui::Text("  Mode: %s%s",
                sd.diag.isHover ? "Hover" : "Write",
                sd.diag.isEdge  ? " + Edge" : "");
            const char* animLabels[] = {"Leave","Hover","Contact","Lifting"};
            int ai = std::clamp(static_cast<int>(sd.animState), 0, 3);
            ImGui::Text("  Lifecycle: %s", animLabels[ai]);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.5f,0.9f,1.0f,1.f), "[P3/P4 Pipeline State]");
            {
                const char* lcLabels[] = {"Leave","Hover","Contact","Lifting"};
                int li = std::clamp(static_cast<int>(sd.diag.penLifecycle), 0, 3);
                ImGui::Text("  Pen Lifecycle: %s", lcLabels[li]);
            }
            ImGui::Text("  Was Inking: %s", sd.diag.wasInking ? "YES" : "no");
            ImGui::Text("  Exit Smoothed: %s", sd.diag.exitSmoothed ? "YES" : "no");
            ImGui::Text("  CMF Enabled: %s", sd.diag.cmfEnabled ? "YES" : "no");
            if (sd.diag.coorReviserActive) {
                ImGui::TextColored(ImVec4(0.4f,1.0f,0.6f,1),
                    "  CoorReviser: ON (dX=%.1f dY=%.1f)",
                    sd.diag.coorRevDeltaX, sd.diag.coorRevDeltaY);
            } else {
                ImGui::Text("  CoorReviser: OFF");
            }
            ImGui::Text("  3Pt Avg: dim1=%d  dim2=%d", sd.diag.avg3PtDim1, sd.diag.avg3PtDim2);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Output (HID/Pressure)")) {
            ConfigUIRenderer::RenderConfigSchema(schema, "Stylus Output", Solvers::ConfigParam::Output);
            ConfigUIRenderer::RenderConfigSchema(schema, "Stylus General", Solvers::ConfigParam::General);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f,0.5f,0.3f,1.f), "[Pressure Chain]");
            ImGui::Text("  Peak Signal: %u", sd.diag.peakSignal);
            ImGui::Text("  Raw Pressure (BT MCU): %u", sd.diag.rawPressure);
            ImGui::Text("  Mapped Pressure: %u", sd.diag.mappedPressure);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.3f,1.0f,0.8f,1.f), "[VHF Pen State]");
            uint8_t ps = sd.diag.vhfPenState;
            bool inRange   = (ps >> 5) & 1;
            bool tipSwitch = (ps >> 0) & 1;
            bool barrel    = (ps >> 1) & 1;
            ImGui::Text("  byte[1] = 0x%02X", ps);
            ImGui::Text("  InRange=%s  TipSwitch=%s  Barrel=%s",
                inRange ? "YES" : "no", tipSwitch ? "YES" : "no", barrel ? "YES" : "no");
            if (inRange && tipSwitch)
                ImGui::TextColored(ImVec4(0.2f,0.9f,0.3f,1), "  => Writing (ink active)");
            else if (inRange)
                ImGui::TextColored(ImVec4(0.6f,0.8f,1.0f,1), "  => Hovering (cursor only)");
            else
                ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), "  => Out of range");

            ImGui::Separator();
            const char* stageNames[] = {
                "OK", "SlaveParseErr", "TX1Invalid", "NoPeak",
                "CoordFail", "NoiseReject"};
            int si = std::clamp(static_cast<int>(sd.pipelineStage), 0, 5);
            if (sd.pipelineStage == 0)
                ImGui::TextColored(ImVec4(0.2f,0.9f,0.3f,1), "Pipeline Status: %s", stageNames[si]);
            else
                ImGui::TextColored(ImVec4(1.0f,0.3f,0.3f,1), "Pipeline Status: %s (%d)", stageNames[si], si);

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // Stylus-Touch Interop settings are now accessible via the Touch Tracking tab's TrackTracker config.

    if (masterParserOnly) ImGui::EndDisabled();
}

void DiagnosticsWorkbench::DrawDynamicDebugPanel() {
    if (!m_proxy) {
        ImGui::TextUnformatted("ServiceProxy unavailable.");
        return;
    }

    const auto defs = m_proxy->GetDynamicDebugFields();
    ImGui::Text("Schema Version: %u", static_cast<unsigned int>(m_proxy->GetDynamicDebugSchemaVersion()));
    ImGui::SameLine();
    ImGui::Text("Schema Hash: %u", static_cast<unsigned int>(m_proxy->GetDynamicDebugSchemaHash()));

    if (defs.empty()) {
        ImGui::TextDisabled("No dynamic debug fields from service.");
        return;
    }

    std::vector<const DynamicDebugField*> ordered;
    ordered.reserve(defs.size());
    for (const auto& d : defs) ordered.push_back(&d);
    std::stable_sort(ordered.begin(), ordered.end(), [](const DynamicDebugField* a, const DynamicDebugField* b) {
        if (a->uiGroup != b->uiGroup) return a->uiGroup < b->uiGroup;
        if (a->uiOrder != b->uiOrder) return a->uiOrder < b->uiOrder;
        return a->fieldId < b->fieldId;
    });

    std::string currentGroup;
    for (const auto* def : ordered) {
        if (def->uiGroup != currentGroup) {
            if (!currentGroup.empty()) ImGui::Separator();
            currentGroup = def->uiGroup;
            ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.0f, 1.0f), "%s", currentGroup.empty() ? "Ungrouped" : currentGroup.c_str());
        }

        DynamicDebugValue v;
        const bool has = m_proxy->GetDynamicDebugValue(def->fieldId, v);
        if (!has || !v.valid) {
            ImGui::Text("%s: N/A", def->displayName.empty() ? def->key.c_str() : def->displayName.c_str());
            continue;
        }

        const char* label = def->displayName.empty() ? def->key.c_str() : def->displayName.c_str();
        switch (v.valueType) {
        case Ipc::DebugValueType::UInt32:
            ImGui::Text("%s: %u %s", label, static_cast<unsigned int>(v.rawValue & 0xFFFFFFFFu), def->unit.c_str());
            break;
        case Ipc::DebugValueType::Int32:
            ImGui::Text("%s: %d %s", label, static_cast<int32_t>(v.rawValue & 0xFFFFFFFFu), def->unit.c_str());
            break;
        case Ipc::DebugValueType::Float32: {
            uint32_t bits = static_cast<uint32_t>(v.rawValue & 0xFFFFFFFFu);
            float fv = 0.0f;
            std::memcpy(&fv, &bits, sizeof(fv));
            ImGui::Text("%s: %.4f %s", label, fv, def->unit.c_str());
            break;
        }
        case Ipc::DebugValueType::Bool:
            ImGui::Text("%s: %s", label, (v.rawValue & 0x1ull) ? "true" : "false");
            break;
        default:
            ImGui::Text("%s: <unknown>", label);
            break;
        }
    }
}

// ── BT MCU Panel (PenBridge Status — via IPC) ──
void DiagnosticsWorkbench::DrawBtMcuPanel() {
    ImGui::TextWrapped(
        "BT MCU dual-channel architecture: "
        "PenEventBridge (col00) handles auto-ACK + 0x7D01 echo. "
        "PenPressureReader (col01) decodes continuous 'U' reports.");
    ImGui::Separator();

    // ── Status + Pressure (via IPC GetPenBridgeStatus) ──
    auto ps = m_proxy ? m_proxy->GetPenBridgeStatus() : App::PenBridgeStatus{};

    ImGui::Text("EventBridge (col00):");
    ImGui::SameLine();
    if (ps.evtRunning)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "RUNNING");
    else
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "STOPPED");

    ImGui::Text("PressReader (col01):");
    ImGui::SameLine();
    if (ps.pressRunning)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "RUNNING");
    else
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "STOPPED");

    ImGui::Separator();
    ImGui::Text("Live Pressure Data");
    ImGui::SameLine();
    ImGui::TextDisabled("(polled every ~500ms)");

    ImGui::Text("Report: 0x%02X (%c)  |  BT Freq: %d / %d",
                ps.reportType,
                ps.reportType >= 0x20 ? static_cast<char>(ps.reportType) : '?',
                ps.freq1, ps.freq2);

    for (int k = 0; k < 4; ++k) {
        ImGui::PushID(k);
        char overlay[32];
        snprintf(overlay, sizeof(overlay), "P%d: %u", k, ps.press[k]);
        ImGui::ProgressBar(static_cast<float>(ps.press[k]) / 8192.0f,
                           ImVec2(-1, 18), overlay);
        ImGui::PopID();
    }

    // ── MCU 相关日志 ──
    ImGui::Separator();
    ImGui::Text("MCU-related Service Logs");
    ImGui::SameLine();
    ImGui::TextDisabled("(refreshed every ~1s via IPC)");

    auto allLines = Common::GuiLogSink::Instance()->GetLines();
    ImGui::BeginChild("McuLogFilter", ImVec2(0, 0), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& line : allLines) {
        if (line.find("PenEventBridge") != std::string::npos ||
            line.find("PenPressureReader") != std::string::npos ||
            line.find("PenBridge") != std::string::npos ||
            line.find("MCU") != std::string::npos ||
            line.find("PenEventCb") != std::string::npos ||
            line.find("PenConn") != std::string::npos ||
            line.find("PenFreq") != std::string::npos) {
            ImVec4 col(0.4f, 0.8f, 1.0f, 1.0f);
            if (line.find("ACK") != std::string::npos ||
                line.find("7D01") != std::string::npos)
                col = ImVec4(0.3f, 1.0f, 0.5f, 1.0f);
            if (line.find("[error") != std::string::npos ||
                line.find("failed") != std::string::npos)
                col = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
            if (line.find("[warn") != std::string::npos)
                col = ImVec4(1.0f, 0.85f, 0.3f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        }
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
}

// ── System Events Panel ──
void DiagnosticsWorkbench::DrawSystemEventsPanel() {
    ImGui::TextWrapped("Simulate system state events by signaling named events "
                       "that SystemStateMonitor is listening to.");
    ImGui::Separator();

#if defined(_DEBUG) || !defined(NDEBUG)
    struct EventInfo {
        const char* label;
        Host::SystemStateNamedEventId id;
        ImVec4 color;
    };
    const EventInfo items[] = {
        {"Display ON (Power)",     Host::SystemStateNamedEventId::MonitorPowerOn, {0.3f, 1.f, 0.3f, 1.f}},
        {"Display OFF (Power)",    Host::SystemStateNamedEventId::MonitorPowerOff, {1.f, 0.3f, 0.3f, 1.f}},
        {"Display ON (Console)",   Host::SystemStateNamedEventId::MonitorConsoleDisplayOn, {0.3f, 1.f, 0.3f, 1.f}},
        {"Display OFF (Console)",  Host::SystemStateNamedEventId::MonitorConsoleDisplayOff, {1.f, 0.3f, 0.3f, 1.f}},
        {"Lid ON",                 Host::SystemStateNamedEventId::MonitorLidOn, {0.3f, 0.8f, 1.f, 1.f}},
        {"Lid OFF",                Host::SystemStateNamedEventId::MonitorLidOff, {1.f, 0.6f, 0.2f, 1.f}},
        {"Shutdown",               Host::SystemStateNamedEventId::MonitorShutDown, {1.f, 0.2f, 0.2f, 1.f}},
        {"Resume Automatic",       Host::SystemStateNamedEventId::PbtApmResumeAutomatic, {0.5f, 1.f, 0.5f, 1.f}},
    };

    for (const auto& item : items) {
        ImGui::PushStyleColor(ImGuiCol_Button, item.color);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
        if (ImGui::Button(item.label, ImVec2(-1, 28))) {
            if (Host::SystemStateMonitor::SignalNamedEvent(item.id)) {
                LOG_INFO("App", __func__, "UI", "Signaled: {}", item.label);
            } else {
                LOG_WARN("App", __func__, "UI", "Failed to signal named event for {}", item.label);
            }
        }
        ImGui::PopStyleColor(2);
    }
#else
    ImGui::TextDisabled("Named-event signaling controls are available in debug/test builds only.");
#endif
}

} // namespace App
