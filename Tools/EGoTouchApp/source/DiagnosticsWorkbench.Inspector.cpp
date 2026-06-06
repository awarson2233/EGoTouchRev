#include "DiagnosticsWorkbench.h"
#include "DiagnosticsWorkbenchInternal.h"
#include "ConfigUIRenderer.h"
#include "ServiceProxy.h"
#include "GuiLogSink.h"
#include "SystemStateMonitor.h"
#include "imgui.h"
#include "Logger.h"
#include "SolverBuildConfig.h"
#include "StylusSolver/AsaTypes.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace App {

namespace {

ImVec4 GoodColor() { return ImVec4(0.2f, 0.9f, 0.3f, 1.0f); }
ImVec4 WarnColor() { return ImVec4(1.0f, 0.8f, 0.2f, 1.0f); }
ImVec4 BadColor() { return ImVec4(1.0f, 0.35f, 0.3f, 1.0f); }
ImVec4 InfoColor() { return ImVec4(0.4f, 0.85f, 1.0f, 1.0f); }

std::vector<std::string> CollectModuleTagsWithPrefix(
    const Config::ConfigSchemaSnapshot& schema,
    std::string_view prefix) {
    std::vector<std::string> tags;
    for (const auto& tag : ConfigUIRenderer::CollectModuleTags(schema)) {
        if (tag.rfind(prefix, 0) == 0) {
            tags.push_back(tag);
        }
    }
    return tags;
}

const char* ModuleDisplayName(const std::string& tag) {
    const auto slash = tag.rfind('/');
    if (slash == std::string::npos) {
        return tag.c_str();
    }
    auto pos = slash + 1;
    while (pos < tag.size() && tag[pos] == ' ') {
        ++pos;
    }
    return tag.c_str() + pos;
}

ImVec4 StatusColor(bool ok) {
    return ok ? GoodColor() : BadColor();
}

ImVec4 FpsColor(int fps) {
    switch (ClassifyFps(fps)) {
    case FpsStatusClass::Good: return GoodColor();
    case FpsStatusClass::Warn: return WarnColor();
    case FpsStatusClass::Bad:
    default: return BadColor();
    }
}

} // namespace

void DiagnosticsWorkbench::DrawInspectorPanel() {
    ImGui::Begin("Inspector");
    if (ImGui::BeginTabBar("CategoryTabs")) {
        if (ImGui::BeginTabItem("Touch")) {
            DrawTouchInspectorPanel();
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
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void DiagnosticsWorkbench::DrawTouchInspectorPanel() {
    if (!m_proxy) {
        ImGui::TextUnformatted("ServiceProxy unavailable.");
        return;
    }

    const bool connected = m_proxy->IsConnected();
    const bool masterParserOnly = m_proxy->IsMasterParserOnlyMode();
    const bool vhfEnabled = m_proxy->IsVhfEnabled();
    const int masterFps = m_proxy->GetAcquisitionFps();
    const int slaveFps = m_proxy->GetSlaveAcquisitionFps();

    if (ImGui::BeginTable("TouchStatusStrip", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Service");
        ImGui::TextColored(StatusColor(connected), "%s", connected ? "● Connected" : "● Disconnected");
        ImGui::TextDisabled("%s", FrameSourceModeLabel(m_proxy->GetFrameSourceMode()));

        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("Pipeline");
        ImGui::TextColored(masterParserOnly ? WarnColor() : GoodColor(), "%s", masterParserOnly ? "Master Parser Only" : "Full Pipeline");
        ImGui::TextColored(StatusColor(vhfEnabled), "Touch VHF: %s", vhfEnabled ? "Enabled" : "Disabled");

        ImGui::TableSetColumnIndex(2);
        ImGui::TextDisabled("Frame Rate");
        ImGui::TextColored(FpsColor(masterFps), "Master: %d Hz", masterFps);
        ImGui::TextColored(FpsColor(slaveFps), "Slave: %d Hz", slaveFps);

        ImGui::TableSetColumnIndex(3);
        ImGui::TextDisabled("Current Frame");
        ImGui::Text("Contacts: %zu", m_currentFrame.touch.output.contacts.size());
#if EGOTOUCH_DIAG
        ImGui::Text("Peaks: %zu", m_currentFrame.touch.debug.peaks.size());
#else
        ImGui::TextDisabled("Peaks: N/A");
#endif
        ImGui::EndTable();
    }

    if (masterParserOnly) {
        ImGui::TextColored(WarnColor(), "Master Parser Only is enabled. Read-only diagnostics remain available; pipeline controls are disabled.");
    }

    ImGui::Separator();
    if (ImGui::BeginTabBar("TouchInspectorTabs")) {
        if (ImGui::BeginTabItem("Overview")) {
            DrawTouchOverviewPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Pipeline Config")) {
            DrawTouchPipelineConfigPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Coordinates")) {
            ImGui::BeginChild("TouchCoordinatesScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            DrawCoordinateTable();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Packets")) {
            DrawTouchPacketDetails();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void DiagnosticsWorkbench::DrawTouchOverviewPanel() {
    if (!m_proxy) {
        ImGui::TextUnformatted("ServiceProxy unavailable.");
        return;
    }

    if (ImGui::BeginTable("TouchOverviewColumns", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextColored(InfoColor(), "Runtime");
        if (ImGui::BeginTable("TouchRuntimeTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Service");
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(StatusColor(m_proxy->IsConnected()), "%s", m_proxy->IsConnected() ? "Connected" : "Disconnected");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Source");
            ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(FrameSourceModeLabel(m_proxy->GetFrameSourceMode()));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Pipeline");
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(m_proxy->IsMasterParserOnlyMode() ? WarnColor() : GoodColor(), "%s", m_proxy->IsMasterParserOnlyMode() ? "Master Parser Only" : "Full Pipeline");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Touch VHF");
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(StatusColor(m_proxy->IsVhfEnabled()), "%s", m_proxy->IsVhfEnabled() ? "Enabled" : "Disabled");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Frame Rate");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(FpsColor(m_proxy->GetAcquisitionFps()), "Master %d Hz", m_proxy->GetAcquisitionFps());
            ImGui::SameLine();
            ImGui::TextColored(FpsColor(m_proxy->GetSlaveAcquisitionFps()), "Slave %d Hz", m_proxy->GetSlaveAcquisitionFps());
            ImGui::EndTable();
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(InfoColor(), "Frame");
        if (ImGui::BeginTable("TouchFrameTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Service Timestamp");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%llu", static_cast<unsigned long long>(m_currentFrame.timestamp));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("App Receive Epoch");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%llu", static_cast<unsigned long long>(m_currentFrame.receiveSystemEpochUs));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Raw Length");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%llu bytes", static_cast<unsigned long long>(m_currentFrame.rawLen));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Master Suffix");
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(StatusColor(m_currentFrame.masterSuffixValid), "%s", m_currentFrame.masterSuffixValid ? "Valid" : "Invalid");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Slave Suffix");
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(StatusColor(m_currentFrame.slaveSuffixValid), "%s", m_currentFrame.slaveSuffixValid ? "Valid" : "Invalid");
            ImGui::EndTable();
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::TextColored(InfoColor(), "Contacts");
    DrawTouchContactSummaryTable();
}

void DiagnosticsWorkbench::DrawTouchPipelineConfigPanel() {
    if (!m_proxy) {
        ImGui::TextUnformatted("ServiceProxy unavailable.");
        return;
    }

#if !EGOTOUCH_CONFIG_ENABLED
    ImGui::TextColored(WarnColor(), "Runtime config is disabled in this build (EGOTOUCH_CONFIG_ENABLED=0).");
    ImGui::TextWrapped("Touch pipeline parameters are not editable here, and this panel cannot live-apply changes to the Service-side pipeline.");
    return;
#else
    const auto& schema = m_proxy->GetConfigSchemaSnapshot();
    const auto modules = CollectModuleTagsWithPrefix(schema, "Touch /");
    if (modules.empty()) {
        ImGui::TextDisabled("No ConfigStore/ConfigBinder touch parameters are registered.");
        return;
    }

    const int moduleCount = static_cast<int>(modules.size());
    m_touchConfigModuleIndex = std::clamp(m_touchConfigModuleIndex, 0, moduleCount - 1);
    const bool masterParserOnly = m_proxy->IsMasterParserOnlyMode();

    ImGui::TextWrapped("Edit touch pipeline parameters by processing stage. Apply updates only the app-local preview pipeline; it does not live-apply the Service-side pipeline.");
    if (masterParserOnly) {
        ImGui::TextColored(WarnColor(), "Master Parser Only is enabled. Pipeline configuration controls are disabled.");
    }
    ImGui::Separator();

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float desiredSelectorWidth = ImGui::CalcTextSize("Signal Conditioning").x + ImGui::GetStyle().FramePadding.x * 4.0f;
    const float maxSelectorWidth = std::max(140.0f, availableWidth * 0.42f);
    const float selectorWidth = std::min(std::max(desiredSelectorWidth, 150.0f), maxSelectorWidth);

    if (ImGui::BeginTable("TouchConfigLayout", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Modules", ImGuiTableColumnFlags_WidthFixed, selectorWidth);
        ImGui::TableSetupColumn("Parameters", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        const float moduleItemWidth = std::max(1.0f, ImGui::GetContentRegionAvail().x);
        for (int i = 0; i < moduleCount; ++i) {
            if (ImGui::Selectable(ModuleDisplayName(modules[static_cast<size_t>(i)]), m_touchConfigModuleIndex == i, 0, ImVec2(moduleItemWidth, 0.0f))) {
                m_touchConfigModuleIndex = i;
            }
        }

        ImGui::TableSetColumnIndex(1);
        const std::string& activeModule = modules[static_cast<size_t>(m_touchConfigModuleIndex)];
        ImGui::TextColored(InfoColor(), "%s", activeModule.c_str());
        ImGui::Separator();

        if (masterParserOnly) {
            ImGui::BeginDisabled();
        }
        ConfigUIRenderer::RenderConfigStoreByModule(schema, m_proxy->GetConfigStore(), activeModule);
        if (masterParserOnly) {
            ImGui::EndDisabled();
        }

        if (ImGui::Button("Apply App-local Preview")) {
            m_proxy->ApplyConfigStoreToLocalRuntime();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Service pipeline is not changed.");

        ImGui::EndTable();
    }
#endif
}

void DiagnosticsWorkbench::DrawTouchPacketDetails() {
    auto drawPacketRow = [](const char* label, const Solvers::TouchPacket& packet) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(label);

        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(StatusColor(packet.valid), "%s", packet.valid ? "Valid" : "Invalid");

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("0x%02X", static_cast<unsigned int>(packet.reportId));

        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%u", static_cast<unsigned int>(packet.length));

        ImGui::TableSetColumnIndex(4);
        if (packet.valid) {
            const std::string bytes = TouchPacketBytes(packet);
            ImGui::TextUnformatted(bytes.c_str());
        } else {
            ImGui::TextDisabled("N/A");
        }
    };

    ImGui::TextWrapped("Raw touch HID packet mirror for the current frame.");
    ImGui::BeginChild("TouchPacketScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    if (ImGui::BeginTable("TouchPacketsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Packet", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Valid", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("RID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Length", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        drawPacketRow("TouchPacket[0]", m_currentFrame.touch.output.touchPackets[0]);
        drawPacketRow("TouchPacket[1]", m_currentFrame.touch.output.touchPackets[1]);
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

void DiagnosticsWorkbench::DrawTouchContactSummaryTable() {
    if (m_currentFrame.touch.output.contacts.empty()) {
        ImGui::TextDisabled("No contacts in current frame.");
        return;
    }

    std::vector<const Solvers::TouchContact*> orderedContacts;
    orderedContacts.reserve(m_currentFrame.touch.output.contacts.size());
    for (const auto& contact : m_currentFrame.touch.output.contacts) {
        orderedContacts.push_back(&contact);
    }
    std::stable_sort(orderedContacts.begin(), orderedContacts.end(), [](const Solvers::TouchContact* a, const Solvers::TouchContact* b) {
        return a->id < b->id;
    });

    if (ImGui::BeginTable("TouchContactSummaryTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Area", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("SigSum", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Reported", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("RptEvt", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableHeadersRow();

        for (const auto* contactPtr : orderedContacts) {
            const auto& contact = *contactPtr;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", contact.id);
            ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(TouchStateLabel(contact.state));
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.3f", contact.x);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.3f", contact.y);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%d", contact.area);
            ImGui::TableSetColumnIndex(5); ImGui::Text("%d", contact.signalSum);
            ImGui::TableSetColumnIndex(6); ImGui::TextUnformatted(contact.isReported ? "Y" : "N");
            ImGui::TableSetColumnIndex(7); ImGui::TextUnformatted(TouchReportEventLabel(contact.reportEvent));
        }
        ImGui::EndTable();
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
    ImGui::TextUnformatted("Pen Button Injection");
    {
        int curMode = static_cast<int>(m_proxy->GetPenButtonMode());
        const char* modeItems[] = {"OEM Custom", "Native Barrel", "Native Eraser"};
        if (ImGui::Combo("Button Mode", &curMode, modeItems, IM_ARRAYSIZE(modeItems))) {
            m_proxy->SetPenButtonMode(static_cast<PenButtonMode>(curMode));
        }

        int curRoute = static_cast<int>(m_proxy->GetPenButtonRoute());
        const char* routeItems[] = {"VHF Only", "Win32 Only", "VHF + Win32"};
        if (ImGui::Combo("Injection Route", &curRoute, routeItems, IM_ARRAYSIZE(routeItems))) {
            m_proxy->SetPenButtonRoute(static_cast<PenButtonRoute>(curRoute));
        }
    }
    ImGui::Separator();

    if (ImGui::BeginTabBar("StylusSubTabs")) {
        const auto& sd = m_currentFrame.stylus;
        const auto& diag = sd.debug.coord;
        const auto& point = sd.output.point;

        const float activeRows = std::max(1.0f, static_cast<float>(std::max(m_proxy->GetStylusPipeline().GetPacketSensorRows(), 1)) * static_cast<float>(Asa::kCoorUnit));
        const float activeCols = std::max(1.0f, static_cast<float>(std::max(m_proxy->GetStylusPipeline().GetPacketSensorCols(), 1)) * static_cast<float>(Asa::kCoorUnit));
        const float clampedY = std::clamp(point.y, 0.0f, activeRows);
        const float clampedX = std::clamp(point.x, 0.0f, activeCols);
        const uint16_t screenX = static_cast<uint16_t>(std::clamp(static_cast<int>(std::lround((clampedY / activeRows) * 16000.0f)), 0, 65535));
        const uint16_t screenY = static_cast<uint16_t>(std::clamp(static_cast<int>(std::lround((1.0f - (clampedX / activeCols)) * 25600.0f)), 0, 65535));
        const bool finalPointMismatch = diag.valid &&
            (std::abs(point.x - static_cast<float>(diag.finalDim1)) > 0.5f ||
             std::abs(point.y - static_cast<float>(diag.finalDim2)) > 0.5f);

        if (ImGui::BeginTabItem("Live Summary")) {
            ImGui::TextColored(sd.output.valid ? ImVec4(0.2f,0.9f,0.3f,1) : ImVec4(1.0f,0.45f,0.25f,1),
                "State: %s%s%s",
                sd.output.valid ? "Valid" : "Invalid",
                sd.output.inRange ? " | InRange" : "",
                sd.output.tipDown ? " | TipDown" : "");
            ImGui::Text("Pipeline Stage: %u", static_cast<unsigned int>(sd.output.pipelineStage));
            ImGui::Text("Pressure: final=%u  raw=%u  mapped=%u  real=%s  age=%u",
                sd.output.pressure,
                diag.rawPressure,
                diag.mappedPressure,
                diag.pressureIsReal ? "yes" : "no",
                static_cast<unsigned int>(diag.predictedAgeFrames));
            ImGui::Text("Signal: peak=%u  tx1=%u  tx2=%u", diag.peakSignal, sd.interop.signalX, sd.interop.signalY);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f,0.85f,1.0f,1.f), "Output Coordinate");
            ImGui::Text("Sensor: X=%.1f / %.1f  Y=%.1f / %.1f", point.x, activeCols, point.y, activeRows);
            ImGui::Text("Report: X=%u  Y=%u", static_cast<unsigned int>(point.reportX), static_cast<unsigned int>(point.reportY));
            ImGui::Text("Screen: X=%u / 16000  Y=%u / 25600", static_cast<unsigned int>(screenX), static_cast<unsigned int>(screenY));
            if (finalPointMismatch) {
                ImGui::TextColored(ImVec4(1.0f,0.35f,0.25f,1),
                    "Final coordinate and output point differ: final=(%d,%d) point=(%.1f,%.1f)",
                    diag.finalDim1, diag.finalDim2, point.x, point.y);
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Coordinate Pipeline")) {
            ImGui::TextColored(ImVec4(0.4f,0.85f,1.0f,1.f), "Coordinate Stages (dim1=X/Col, dim2=Y/Row)");
            ImGui::Text("Anchor: row=%u  col=%u", diag.anchorRow, diag.anchorCol);
            ImGui::Text("Local:  dim1=%d  dim2=%d", diag.localCoorDim1, diag.localCoorDim2);
            ImGui::Text("Raw:    dim1=%d  dim2=%d", diag.rawDim1, diag.rawDim2);
            ImGui::Text("3PtAvg: dim1=%d  dim2=%d", diag.avg3PtDim1, diag.avg3PtDim2);
            ImGui::Text("Final:  dim1=%d  dim2=%d", diag.finalDim1, diag.finalDim2);
            ImGui::Text("Point:  x=%.1f  y=%.1f", point.x, point.y);
            if (finalPointMismatch) {
                ImGui::TextColored(ImVec4(1.0f,0.35f,0.25f,1), "Final/Point mismatch: downstream finalCoor changes may not be reflected in output.point.");
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f,0.85f,0.4f,1.f), "Post Filters");
            ImGui::Text("Speed: instant=%.1f  short=%.1f  full=%.1f", diag.speedInstant, diag.speedShortAvg, diag.speedFullAvg);
            ImGui::Text("IIR: coef=%.0f", diag.iirCoef);
            ImGui::Text("Linear Filter: state=%u", static_cast<unsigned int>(diag.linearFilterState));
            ImGui::Text("CoorRevise: %s  delta=(%.1f, %.1f)",
                diag.coorReviserActive ? "on" : "off",
                diag.coorRevDeltaX,
                diag.coorRevDeltaY);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Tilt / Edge")) {
            ImGui::TextColored(ImVec4(0.8f,0.6f,1.0f,1.f), "Tilt");
            ImGui::Text("TX1/TX2 diff: filtered=(%.1f, %.1f)  raw=(%d, %d)",
                diag.tiltDiffX, diag.tiltDiffY, diag.tiltRawDiffDim1, diag.tiltRawDiffDim2);
            ImGui::Text("Tilt angle: pre=(%d, %d)  report=(%d, %d)",
                static_cast<int>(diag.preTiltDim1),
                static_cast<int>(diag.preTiltDim2),
                static_cast<int>(diag.reportTiltDim1),
                static_cast<int>(diag.reportTiltDim2));
            ImGui::Text("Signal ratio TX2/TX1: %u%%  lenLimit=%u",
                static_cast<unsigned int>(diag.signalRatio),
                static_cast<unsigned int>(diag.tiltLenLimit));
            if (diag.tiltAnomalyDamped) {
                ImGui::TextColored(ImVec4(1.0f,0.4f,0.4f,1), "Tilt anomaly damping active");
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f,0.85f,0.4f,1.f), "Edge / Signal Gates");
            ImGui::Text("Mode: %s%s", diag.isHover ? "Hover" : "Write", diag.isEdge ? " + Edge" : "");
            ImGui::Text("Edge flags: dim1=%s  dim2=%s", diag.dim1Edge ? "yes" : "no", diag.dim2Edge ? "yes" : "no");
            ImGui::Text("Peak: tx1=%u sum3x3=%u  tx2=%u sum3x3=%u tx2Valid=%s",
                diag.tx1PeakValue,
                diag.tx1Sum3x3,
                diag.tx2PeakValue,
                diag.tx2Sum3x3,
                diag.tx2Valid ? "yes" : "no");
            ImGui::Text("BT press suppress: %s", diag.btPressSuppressActive ? "active" : "inactive");
            ImGui::Text("Edge signal low latch: %s", diag.edgeSignalTooLowLatched ? "active" : "inactive");
            ImGui::Text("Fake pressure decrease: %s  framesLeft=%u",
                diag.fakePressureDecreaseActive ? "active" : "inactive",
                static_cast<unsigned int>(diag.fakePressureDecreaseFramesLeft));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Config")) {
#if !EGOTOUCH_CONFIG_ENABLED
            ImGui::TextColored(WarnColor(), "Runtime config is disabled in this build (EGOTOUCH_CONFIG_ENABLED=0).");
            ImGui::TextWrapped("Stylus pipeline parameters are not editable here, and this tab cannot live-apply changes to the Service-side pipeline.");
#else
            ImGui::TextWrapped("Edit stylus pipeline parameters by module. Apply updates only the app-local preview pipeline; it does not live-apply the Service-side pipeline.");
            const auto& schema = m_proxy->GetConfigSchemaSnapshot();
            const auto modules = CollectModuleTagsWithPrefix(schema, "Stylus /");
            if (modules.empty()) {
                ImGui::TextDisabled("No ConfigStore/ConfigBinder stylus parameters are registered.");
            } else if (ImGui::BeginTabBar("StylusConfigTabs")) {
                for (const auto& module : modules) {
                    if (ImGui::BeginTabItem(ModuleDisplayName(module))) {
                        ConfigUIRenderer::RenderConfigStoreByModule(schema, m_proxy->GetConfigStore(), module);
                        if (ImGui::Button("Apply App-local Preview")) {
                            m_proxy->ApplyConfigStoreToLocalRuntime();
                        }
                        ImGui::SameLine();
                        ImGui::TextDisabled("Service pipeline is not changed.");
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
#endif
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Advanced Debug")) {
            ImGui::TextColored(ImVec4(0.9f,0.9f,0.5f,1.f), "Linear Filter Internals");
            ImGui::Text("State=%u  lfState=%u  cos=%d  straightCount=%d  drag=%d",
                static_cast<unsigned int>(diag.linearFilterState),
                static_cast<unsigned int>(diag.lfStateMachine),
                diag.lfCos1000,
                diag.lfStraightBufCount,
                diag.lfDragApplied);
            ImGui::Text("Line fit: valid=%s  slope=%.4f  intercept=%.4f",
                diag.lfLineFitValid ? "yes" : "no",
                diag.lfLineFitSlopeA,
                diag.lfLineFitInterceptB);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f,0.5f,0.3f,1.f), "Pressure Internals");
            ImGui::Text("BT raw=%u  preIIR=%u  polySegment=%u  freqDebounceLeft=%u",
                diag.btRawPressure,
                diag.preIirPressure,
                static_cast<unsigned int>(diag.polySegment),
                static_cast<unsigned int>(diag.btFreqShiftDebounceFramesLeft));

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.5f,0.9f,1.0f,1.f), "Legacy / Derived Flags");
            ImGui::Text("lifecycle=%u  wasInking=%s  exitSmoothed=%s  cmf=%s  sigSuppress=%s  vhfPenState=0x%02X",
                static_cast<unsigned int>(diag.penLifecycle),
                diag.wasInking ? "yes" : "no",
                diag.exitSmoothed ? "yes" : "no",
                diag.cmfEnabled ? "yes" : "no",
                diag.sigSuppressActive ? "yes" : "no",
                static_cast<unsigned int>(diag.vhfPenState));

            ImGui::Separator();
            DrawDynamicDebugPanel();
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
    ImGui::Text("Pressure Range Mode");
    int pressureMode = ps.pressureMode == 0 ? 0 : 1;
    const char* pressureModes[] = {"4096 raw12", "16382 raw14 / 4"};
    if (ImGui::Combo("##PenPressureMode", &pressureMode, pressureModes, 2) && m_proxy) {
        m_proxy->SetPenPressureMode(static_cast<uint8_t>(pressureMode));
    }

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
        snprintf(overlay, sizeof(overlay), "P%d: %u raw:%u", k, ps.press[k], ps.rawPress[k]);
        const float denom = static_cast<float>(std::max<uint16_t>(ps.pressureMax, 1));
        ImGui::ProgressBar(static_cast<float>(ps.press[k]) / denom,
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
