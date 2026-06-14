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
#include <optional>
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

void DrawApplyConfigResultStatus(const ApplyConfigResult& result) {
    switch (result.status) {
    case ApplyConfigStatus::NotAttempted:
        ImGui::TextDisabled("Apply Global not run yet.");
        break;
    case ApplyConfigStatus::NoChanges:
        ImGui::TextDisabled("No pending live-applicable config changes.");
        break;
    case ApplyConfigStatus::LiveApplyFailed:
        ImGui::TextColored(BadColor(), "Live apply failed.");
        break;
    case ApplyConfigStatus::LiveApplied:
        ImGui::TextColored(GoodColor(), "Live applied to current Service session.");
        ImGui::TextColored(WarnColor(), "Persistent config files are not supported; changes reset after Service restart.");
        break;
    case ApplyConfigStatus::RestartRequired:
        ImGui::TextColored(WarnColor(), "Staged for current Service session; restart is required to activate this value.");
        ImGui::TextColored(WarnColor(), "Persistent config files are not supported; changes reset after Service restart.");
        break;
    }
}

ImVec4 FpsColor(int fps) {
    switch (ClassifyFps(fps)) {
    case FpsStatusClass::Good: return GoodColor();
    case FpsStatusClass::Warn: return WarnColor();
    case FpsStatusClass::Bad:
    default: return BadColor();
    }
}

ConfigUIApplyState ToConfigUIApplyState(ConfigDraftApplyState state) {
    switch (state) {
    case ConfigDraftApplyState::Clean: return ConfigUIApplyState::Clean;
    case ConfigDraftApplyState::Pending: return ConfigUIApplyState::Pending;
    case ConfigDraftApplyState::LiveApplied: return ConfigUIApplyState::LiveApplied;
    case ConfigDraftApplyState::StagedRestartRequired: return ConfigUIApplyState::StagedRestartRequired;
    case ConfigDraftApplyState::Failed: return ConfigUIApplyState::Failed;
    }
    return ConfigUIApplyState::Failed;
}

ConfigUIPersistState ToConfigUIPersistState(ConfigDraftPersistState state) {
    switch (state) {
    case ConfigDraftPersistState::NotAttempted: return ConfigUIPersistState::NotAttempted;
    case ConfigDraftPersistState::Persisted: return ConfigUIPersistState::Persisted;
    case ConfigDraftPersistState::Unpersisted: return ConfigUIPersistState::Unpersisted;
    case ConfigDraftPersistState::Failed: return ConfigUIPersistState::Failed;
    }
    return ConfigUIPersistState::Failed;
}

ConfigUIRenderer::ConfigPathStateProvider MakeConfigPathStateProvider(ServiceProxy* proxy) {
    return [proxy](std::string_view path) -> std::optional<ConfigUIPathState> {
        if (proxy == nullptr) {
            return std::nullopt;
        }

        const auto state = proxy->GetConfigDraftPathState(path);
        ConfigUIPathState result;
        result.dirty = state.dirty;
        result.applyState = ToConfigUIApplyState(state.applyState);
        result.persistState = ToConfigUIPersistState(state.persistState);
        result.failedKeyId = state.failedKeyId;
        result.errorMessage = state.errorMessage;
        return result;
    };
}

bool DrawConfigSyncGate(ServiceProxy* proxy) {
    if (proxy == nullptr || proxy->IsConfigAdjustmentAllowed()) {
        return true;
    }

    ImGui::TextColored(WarnColor(), "Config is not synchronized from Service; parameter adjustment is disabled.");
    const std::string message = proxy->GetConfigServiceSyncStatusMessage();
    if (!message.empty()) {
        ImGui::TextWrapped("%s", message.c_str());
    }
    if (proxy->IsConnected()) {
        if (ImGui::Button("Retry Config Sync")) {
            proxy->SynchronizeConfigFromServiceForEditing();
        }
    }
    return false;
}

const char* YesNo(bool value) {
    return value ? "Y" : "N";
}

const char* PenModuleModelName(uint32_t modelId) {
    switch (modelId) {
    case 0x00011B: return "CD54";
    case 0x01011B: return "CD54R";
    case 0x443002: return "CD54S";
    default: return "Unknown";
    }
}

std::string PenModuleModelText(const PenIdentityStatus& pen) {
    if (!pen.hasPenModuleModelId) {
        return "Unknown";
    }

    char buffer[64]{};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%s (0x%06X)",
        PenModuleModelName(pen.penModuleModelId),
        static_cast<unsigned int>(pen.penModuleModelId));
    return std::string(buffer);
}

std::string PenIdentitySummary(const PenIdentityStatus& pen) {
    std::string summary = pen.connected ? "connected" : "disconnected/unknown";
    summary += " | stylusId=";
    summary += pen.hasStylusId ? std::to_string(static_cast<unsigned int>(pen.stylusId)) : "Unknown";
    summary += " | modelId=";
    summary += PenModuleModelText(pen);
    summary += " | hardwareVersion=";
    summary += (pen.hasHardwareVersion && !pen.hardwareVersion.empty()) ? pen.hardwareVersion : "Unknown";
    return summary;
}

std::string StylusPacketBytes(const Solvers::StylusPacket& packet) {
    const auto displayLength = std::min<std::size_t>(packet.length, packet.bytes.size());

    std::string result;
    result.reserve(displayLength * 3 + (packet.length > packet.bytes.size() ? 37U : 0U));
    char byteText[4]{};
    for (std::size_t i = 0; i < displayLength; ++i) {
        std::snprintf(byteText, sizeof(byteText), "%02x", static_cast<unsigned int>(packet.bytes[i]));
        result += byteText;
        if (i + 1 < displayLength) {
            result += ' ';
        }
    }
    if (packet.length > packet.bytes.size()) {
        if (!result.empty()) {
            result += ' ';
        }
        result += "... (declared length exceeds buffer)";
    }
    return result;
}

template <typename FourValueArray>
std::string FourU16ValuesText(const FourValueArray& values) {
    char buffer[96]{};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%u / %u / %u / %u",
        static_cast<unsigned int>(values[0]),
        static_cast<unsigned int>(values[1]),
        static_cast<unsigned int>(values[2]),
        static_cast<unsigned int>(values[3]));
    return std::string(buffer);
}

} // namespace

void DiagnosticsWorkbench::DrawApplyConfigResultStatus() const {
    if (!m_proxy) {
        return;
    }

    ::App::DrawApplyConfigResultStatus(m_proxy->GetLastApplyConfigResult());
}

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


    const auto& schema = m_proxy->GetConfigSchemaSnapshot();
    const auto modules = CollectModuleTagsWithPrefix(schema, "Touch /");
    if (modules.empty()) {
        ImGui::TextDisabled("No ConfigStore/ConfigBinder touch parameters are registered.");
        return;
    }

    const int moduleCount = static_cast<int>(modules.size());
    m_touchConfigModuleIndex = std::clamp(m_touchConfigModuleIndex, 0, moduleCount - 1);
    const bool masterParserOnly = m_proxy->IsMasterParserOnlyMode();

    ImGui::TextWrapped("Edit touch pipeline parameters by processing stage. Apply Global sends supported keys to the Service for live apply; persistence depends on Service/build support.");
    if (masterParserOnly) {
        ImGui::TextColored(WarnColor(), "Master Parser Only is enabled. Pipeline configuration controls are disabled.");
    }
    const bool configAdjustmentAllowed = DrawConfigSyncGate(m_proxy);
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

        const bool editable = configAdjustmentAllowed && !masterParserOnly;
        Config::ConfigStore& draftView = m_proxy->GetMutableConfigDraftStoreForUi();
        std::vector<std::string> changedPaths;
        ConfigUIRenderer::RenderConfigStoreByModule(
            schema,
            draftView,
            activeModule,
            editable ? &changedPaths : nullptr,
            MakeConfigPathStateProvider(m_proxy),
            editable);
        if (editable) {
            m_proxy->CommitConfigDraftEdits(changedPaths);
        }

        if (!configAdjustmentAllowed) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Apply Global")) {
            m_proxy->ApplyConfigStoreGlobally();
        }
        if (!configAdjustmentAllowed) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Live-applies supported keys to the current Service session only; persistent config files are disabled.");
        DrawApplyConfigResultStatus();

        ImGui::EndTable();
    }
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

    const bool connected = m_proxy->IsConnected();
    const bool masterParserOnly = m_proxy->IsMasterParserOnlyMode();
    const bool stylusVhfEnabled = m_proxy->IsSrvStylusVhfEnabled();
    const auto pen = m_proxy->GetPenIdentityStatus();
    const auto& stylus = m_currentFrame.stylus;
    const auto& output = stylus.output;
    const std::string modelText = PenModuleModelText(pen);

    if (ImGui::BeginTable("StylusStatusStrip", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("Service");
        ImGui::TextColored(StatusColor(connected), "%s", connected ? "● Connected" : "● Disconnected");
        ImGui::TextDisabled("%s", FrameSourceModeLabel(m_proxy->GetFrameSourceMode()));

        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("Pipeline");
        ImGui::TextColored(masterParserOnly ? WarnColor() : GoodColor(), "%s", masterParserOnly ? "Master Parser Only" : "Full Pipeline");
        ImGui::TextColored(StatusColor(stylusVhfEnabled), "Stylus VHF: %s", stylusVhfEnabled ? "Enabled" : "Disabled");

        ImGui::TableSetColumnIndex(2);
        ImGui::TextDisabled("Pen");
        ImGui::TextColored(StatusColor(pen.connected), "%s", pen.connected ? "Connected" : "Disconnected / Unknown");
        ImGui::TextDisabled("Model: %s", modelText.c_str());

        ImGui::TableSetColumnIndex(3);
        ImGui::TextDisabled("Current Sample");
        ImGui::TextColored(StatusColor(output.valid), "%s", output.valid ? "Valid" : "Invalid");
        ImGui::SameLine();
        ImGui::Text("%s%s", output.inRange ? " | InRange" : " | OutOfRange", output.tipDown ? " | TipDown" : "");
        ImGui::Text("Pressure: %u  Stage: %u", static_cast<unsigned int>(output.pressure), static_cast<unsigned int>(output.pipelineStage));
        ImGui::EndTable();
    }

    if (masterParserOnly) {
        ImGui::TextColored(WarnColor(), "Master Parser Only is enabled. Read-only diagnostics remain available; stylus pipeline parameter controls are disabled.");
    }

    ImGui::Separator();
    if (ImGui::BeginTabBar("StylusInspectorTabs")) {
        if (ImGui::BeginTabItem("Overview")) {
            DrawStylusOverviewPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Pipeline Config")) {
            DrawStylusPipelineConfigPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Coordinates")) {
            DrawStylusCoordinatePanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Packets")) {
            DrawStylusPacketDetails();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void DiagnosticsWorkbench::DrawStylusOverviewPanel() {
    if (!m_proxy) {
        ImGui::TextUnformatted("ServiceProxy unavailable.");
        return;
    }

    const auto pen = m_proxy->GetPenIdentityStatus();
    const auto& stylus = m_currentFrame.stylus;
    const auto& output = stylus.output;
    const auto& interop = stylus.interop;
    const auto& diag = stylus.debug.coord;

    if (ImGui::BeginTable("StylusOverviewColumns", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextColored(InfoColor(), "Runtime");
        if (ImGui::BeginTable("StylusRuntimeTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            auto drawRow = [](const char* label, const auto& drawValue) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(label);
                ImGui::TableSetColumnIndex(1);
                drawValue();
            };

            drawRow("Service", [&] { ImGui::TextColored(StatusColor(m_proxy->IsConnected()), "%s", m_proxy->IsConnected() ? "Connected" : "Disconnected"); });
            drawRow("Source", [&] { ImGui::TextUnformatted(FrameSourceModeLabel(m_proxy->GetFrameSourceMode())); });
            drawRow("Pipeline", [&] { ImGui::TextColored(m_proxy->IsMasterParserOnlyMode() ? WarnColor() : GoodColor(), "%s", m_proxy->IsMasterParserOnlyMode() ? "Master Parser Only" : "Full Pipeline"); });
            drawRow("Stylus VHF", [&] { ImGui::TextColored(StatusColor(m_proxy->IsSrvStylusVhfEnabled()), "%s", m_proxy->IsSrvStylusVhfEnabled() ? "Enabled" : "Disabled"); });
            drawRow("Pen Button Mode", [&] { ImGui::TextUnformatted(ToString(m_proxy->GetPenButtonMode())); });
            drawRow("Injection Route", [&] { ImGui::TextUnformatted(ToString(m_proxy->GetPenButtonRoute())); });
            drawRow("Pen Identity", [&] {
                const std::string summary = PenIdentitySummary(pen);
                ImGui::TextWrapped("%s", summary.c_str());
            });
            ImGui::EndTable();
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(InfoColor(), "Frame");
        if (ImGui::BeginTable("StylusFrameTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            auto drawRow = [](const char* label, const auto& drawValue) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(label);
                ImGui::TableSetColumnIndex(1);
                drawValue();
            };

            drawRow("Service Timestamp", [&] { ImGui::Text("%llu", static_cast<unsigned long long>(m_currentFrame.timestamp)); });
            drawRow("App Receive Epoch", [&] { ImGui::Text("%llu", static_cast<unsigned long long>(m_currentFrame.receiveSystemEpochUs)); });
            drawRow("Raw Length", [&] { ImGui::Text("%llu bytes", static_cast<unsigned long long>(m_currentFrame.rawLen)); });
            drawRow("Master Suffix", [&] { ImGui::TextColored(StatusColor(m_currentFrame.masterSuffixValid), "%s", m_currentFrame.masterSuffixValid ? "Valid" : "Invalid"); });
            drawRow("Slave Suffix", [&] { ImGui::TextColored(StatusColor(m_currentFrame.slaveSuffixValid), "%s", m_currentFrame.slaveSuffixValid ? "Valid" : "Invalid"); });
            drawRow("Stylus Output", [&] {
                ImGui::TextColored(StatusColor(output.valid), "%s", output.valid ? "Valid" : "Invalid");
                ImGui::SameLine();
                ImGui::Text("%s | %s | %s", output.inRange ? "InRange" : "OutOfRange", output.tipDown ? "TipDown" : "TipUp", output.buttonActive ? "Button" : "NoButton");
            });
            drawRow("Pressure", [&] {
                ImGui::Text("final=%u  raw=%u  mapped=%u  real=%s  age=%u",
                    static_cast<unsigned int>(output.pressure),
                    static_cast<unsigned int>(diag.rawPressure),
                    static_cast<unsigned int>(diag.mappedPressure),
                    diag.pressureIsReal ? "yes" : "no",
                    static_cast<unsigned int>(diag.predictedAgeFrames));
            });
            drawRow("Confidence", [&] { ImGui::Text("%.3f", output.confidence); });
            drawRow("Pipeline Stage", [&] { ImGui::Text("%u", static_cast<unsigned int>(output.pipelineStage)); });
            drawRow("Interop Recheck", [&] {
                ImGui::Text("%s / %s / %s  th=%u multi=%u",
                    interop.recheckEnabled ? "Enabled" : "Disabled",
                    interop.recheckPassed ? "Pass" : "Fail",
                    interop.recheckOverlap ? "Overlap" : "NoOverlap",
                    static_cast<unsigned int>(interop.recheckThreshold),
                    static_cast<unsigned int>(interop.recheckThresholdMulti));
            });
            drawRow("Touch Suppress", [&] {
                ImGui::Text("active=%s  nullLike=%s  frames=%u",
                    YesNo(interop.touchSuppressActive),
                    YesNo(interop.touchNullLike),
                    static_cast<unsigned int>(interop.touchSuppressFrames));
            });
            drawRow("Signal TX1/TX2", [&] { ImGui::Text("%u / %u", static_cast<unsigned int>(interop.signalX), static_cast<unsigned int>(interop.signalY)); });
            drawRow("Max Raw Peak", [&] { ImGui::Text("%u", static_cast<unsigned int>(interop.maxRawPeak)); });
            ImGui::EndTable();
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    DrawStylusServicePolicyPanel();
}

void DiagnosticsWorkbench::DrawStylusServicePolicyPanel() {
    if (!m_proxy) {
        return;
    }

    ImGui::TextColored(InfoColor(), "Service Policy");
    const bool configAdjustmentAllowed = DrawConfigSyncGate(m_proxy);
#ifdef _DEBUG
    const char* modeItems[] = {"OEM Custom", "Native Barrel", "Native Eraser"};
    const char* routeItems[] = {"VHF Only", "Win32 Only", "VHF + Win32"};

    if (!configAdjustmentAllowed) {
        ImGui::BeginDisabled();
    }
    ImGui::TextUnformatted("Windows INK Output (VHF)");
    bool vhfStylus = m_proxy->IsSrvStylusVhfEnabled();
    if (ImGui::Checkbox("Enable Stylus Native Output", &vhfStylus)) {
        m_proxy->SetSrvStylusVhfEnabled(vhfStylus);
    }
    ImGui::TextDisabled("Staged; click Apply Global to live-apply service policy.");

    ImGui::Separator();
    ImGui::TextUnformatted("Pen Button Injection");
    int curMode = static_cast<int>(m_proxy->GetPenButtonMode());
    if (ImGui::Combo("Button Mode", &curMode, modeItems, IM_ARRAYSIZE(modeItems))) {
        m_proxy->SetPenButtonMode(static_cast<PenButtonMode>(curMode));
    }

    int curRoute = static_cast<int>(m_proxy->GetPenButtonRoute());
    if (ImGui::Combo("Injection Route", &curRoute, routeItems, IM_ARRAYSIZE(routeItems))) {
        m_proxy->SetPenButtonRoute(static_cast<PenButtonRoute>(curRoute));
    }
    ImGui::TextDisabled("Button policy changes are staged; click Apply Global to live-apply service policy.");
    if (ImGui::Button("Apply Global##StylusServicePolicy")) {
        m_proxy->ApplyConfigStoreGlobally();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Live-applies supported keys to the current Service session only; persistent config files are disabled.");
    DrawApplyConfigResultStatus();
    if (!configAdjustmentAllowed) {
        ImGui::EndDisabled();
    }
#else
    ImGui::TextWrapped("Release builds show the live Service policy snapshot without mutating Service configuration.");
    ImGui::TextDisabled("Stylus Native Output: %s", m_proxy->IsSrvStylusVhfEnabled() ? "Enabled" : "Disabled");
    ImGui::TextDisabled("Button Mode: %s", ToString(m_proxy->GetPenButtonMode()));
    ImGui::TextDisabled("Injection Route: %s", ToString(m_proxy->GetPenButtonRoute()));
    DrawApplyConfigResultStatus();
#endif
}

void DiagnosticsWorkbench::DrawStylusPipelineConfigPanel() {
    if (!m_proxy) {
        ImGui::TextUnformatted("ServiceProxy unavailable.");
        return;
    }

    const auto& schema = m_proxy->GetConfigSchemaSnapshot();
    const auto modules = CollectModuleTagsWithPrefix(schema, "Stylus /");
    if (modules.empty()) {
        ImGui::TextDisabled("No ConfigStore/ConfigBinder stylus parameters are registered.");
        return;
    }

    const int moduleCount = static_cast<int>(modules.size());
    m_stylusConfigModuleIndex = std::clamp(m_stylusConfigModuleIndex, 0, moduleCount - 1);
    const bool masterParserOnly = m_proxy->IsMasterParserOnlyMode();

    ImGui::TextWrapped("Edit stylus pipeline parameters by processing stage. Apply Global sends supported keys to the Service for live apply; persistence depends on Service/build support.");
    if (masterParserOnly) {
        ImGui::TextColored(WarnColor(), "Master Parser Only is enabled. Parameter editing is disabled; module selection and status remain readable.");
    }
    const bool configAdjustmentAllowed = DrawConfigSyncGate(m_proxy);
    ImGui::Separator();

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float desiredSelectorWidth = ImGui::CalcTextSize("Signal Conditioning").x + ImGui::GetStyle().FramePadding.x * 4.0f;
    const float maxSelectorWidth = std::max(140.0f, availableWidth * 0.42f);
    const float selectorWidth = std::min(std::max(desiredSelectorWidth, 150.0f), maxSelectorWidth);

    if (ImGui::BeginTable("StylusConfigLayout", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Modules", ImGuiTableColumnFlags_WidthFixed, selectorWidth);
        ImGui::TableSetupColumn("Parameters", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        const float moduleItemWidth = std::max(1.0f, ImGui::GetContentRegionAvail().x);
        for (int i = 0; i < moduleCount; ++i) {
            if (ImGui::Selectable(ModuleDisplayName(modules[static_cast<size_t>(i)]), m_stylusConfigModuleIndex == i, 0, ImVec2(moduleItemWidth, 0.0f))) {
                m_stylusConfigModuleIndex = i;
            }
        }

        ImGui::TableSetColumnIndex(1);
        const std::string& activeModule = modules[static_cast<size_t>(m_stylusConfigModuleIndex)];
        ImGui::TextColored(InfoColor(), "%s", activeModule.c_str());
        ImGui::Separator();

        const bool editable = configAdjustmentAllowed && !masterParserOnly;
        Config::ConfigStore& draftView = m_proxy->GetMutableConfigDraftStoreForUi();
        std::vector<std::string> changedPaths;
        ConfigUIRenderer::RenderConfigStoreByModule(
            schema,
            draftView,
            activeModule,
            editable ? &changedPaths : nullptr,
            MakeConfigPathStateProvider(m_proxy),
            editable);
        if (editable) {
            m_proxy->CommitConfigDraftEdits(changedPaths);
        }

        if (!configAdjustmentAllowed) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Apply Global##StylusPipelineConfig")) {
            m_proxy->ApplyConfigStoreGlobally();
        }
        if (!configAdjustmentAllowed) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Live-applies supported keys to the current Service session only; persistent config files are disabled.");
        DrawApplyConfigResultStatus();

        ImGui::EndTable();
    }
}

void DiagnosticsWorkbench::DrawStylusCoordinatePanel() {
    if (!m_proxy) {
        ImGui::TextUnformatted("ServiceProxy unavailable.");
        return;
    }

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

    ImGui::BeginChild("StylusCoordinatesScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::TextColored(InfoColor(), "Output Coordinate");
    if (ImGui::BeginTable("StylusOutputCoordinateTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        auto drawRow = [](const char* label, const auto& drawValue) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(label);
            ImGui::TableSetColumnIndex(1);
            drawValue();
        };

        drawRow("State", [&] {
            ImGui::TextColored(sd.output.valid ? GoodColor() : BadColor(), "%s%s%s",
                sd.output.valid ? "Valid" : "Invalid",
                sd.output.inRange ? " | InRange" : "",
                sd.output.tipDown ? " | TipDown" : "");
        });
        drawRow("Sensor", [&] { ImGui::Text("X=%.1f / %.1f  Y=%.1f / %.1f", point.x, activeCols, point.y, activeRows); });
        drawRow("Report", [&] { ImGui::Text("X=%u  Y=%u", static_cast<unsigned int>(point.reportX), static_cast<unsigned int>(point.reportY)); });
        drawRow("Screen", [&] { ImGui::Text("X=%u / 16000  Y=%u / 25600", static_cast<unsigned int>(screenX), static_cast<unsigned int>(screenY)); });
        drawRow("TX1/TX2", [&] { ImGui::Text("TX1=(%.3f, %.3f)  TX2=(%.3f, %.3f)", point.tx1X, point.tx1Y, point.tx2X, point.tx2Y); });
        drawRow("Composite Signal", [&] { ImGui::Text("TX1=%u  TX2=%u", static_cast<unsigned int>(point.peakTx1), static_cast<unsigned int>(point.peakTx2)); });
        if (finalPointMismatch) {
            drawRow("Final Mismatch", [&] {
                ImGui::TextColored(BadColor(), "final=(%d,%d)  point=(%.1f,%.1f)", diag.finalDim1, diag.finalDim2, point.x, point.y);
            });
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::TextColored(InfoColor(), "Coordinate Pipeline");
    if (ImGui::BeginTable("StylusCoordinatePipelineTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Stage", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        auto drawRow = [](const char* label, const auto& drawValue) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(label);
            ImGui::TableSetColumnIndex(1);
            drawValue();
        };

        drawRow("Anchor", [&] { ImGui::Text("row=%u  col=%u", static_cast<unsigned int>(diag.anchorRow), static_cast<unsigned int>(diag.anchorCol)); });
        drawRow("Local", [&] { ImGui::Text("dim1=%d  dim2=%d", diag.localCoorDim1, diag.localCoorDim2); });
        drawRow("Raw", [&] { ImGui::Text("dim1=%d  dim2=%d", diag.rawDim1, diag.rawDim2); });
        drawRow("3PtAvg", [&] { ImGui::Text("dim1=%d  dim2=%d", diag.avg3PtDim1, diag.avg3PtDim2); });
        drawRow("Final", [&] { ImGui::Text("dim1=%d  dim2=%d", diag.finalDim1, diag.finalDim2); });
        drawRow("Point", [&] { ImGui::Text("x=%.1f  y=%.1f", point.x, point.y); });
        drawRow("Speed", [&] { ImGui::Text("instant=%.1f  short=%.1f  full=%.1f", diag.speedInstant, diag.speedShortAvg, diag.speedFullAvg); });
        drawRow("IIR", [&] { ImGui::Text("coef=%.0f", diag.iirCoef); });
        drawRow("Linear Filter", [&] { ImGui::Text("state=%u", static_cast<unsigned int>(diag.linearFilterState)); });
        drawRow("CoorRevise", [&] { ImGui::Text("%s  delta=(%.1f, %.1f)", diag.coorReviserActive ? "on" : "off", diag.coorRevDeltaX, diag.coorRevDeltaY); });
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::TextColored(InfoColor(), "Tilt / Edge");
    if (ImGui::BeginTable("StylusTiltEdgeTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        auto drawRow = [](const char* label, const auto& drawValue) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(label);
            ImGui::TableSetColumnIndex(1);
            drawValue();
        };

        drawRow("TX1/TX2 Diff", [&] { ImGui::Text("filtered=(%.1f, %.1f)  raw=(%d, %d)", diag.tiltDiffX, diag.tiltDiffY, diag.tiltRawDiffDim1, diag.tiltRawDiffDim2); });
        drawRow("Tilt Angle", [&] { ImGui::Text("pre=(%d, %d)  report=(%d, %d)", static_cast<int>(diag.preTiltDim1), static_cast<int>(diag.preTiltDim2), static_cast<int>(diag.reportTiltDim1), static_cast<int>(diag.reportTiltDim2)); });
        drawRow("Signal Ratio", [&] { ImGui::Text("TX2/TX1=%u%%  lenLimit=%u", static_cast<unsigned int>(diag.signalRatio), static_cast<unsigned int>(diag.tiltLenLimit)); });
        drawRow("Mode", [&] { ImGui::Text("%s%s", diag.isHover ? "Hover" : "Writing", diag.isEdge ? " + Edge" : ""); });
        drawRow("Edge Flags", [&] { ImGui::Text("dim1=%s  dim2=%s", diag.dim1Edge ? "yes" : "no", diag.dim2Edge ? "yes" : "no"); });
        drawRow("Peak", [&] { ImGui::Text("tx1=%u sum3x3=%u  tx2=%u sum3x3=%u  tx2Valid=%s", diag.tx1PeakValue, diag.tx1Sum3x3, diag.tx2PeakValue, diag.tx2Sum3x3, diag.tx2Valid ? "yes" : "no"); });
        drawRow("BT Press Suppress", [&] { ImGui::TextUnformatted(diag.btPressSuppressActive ? "active" : "inactive"); });
        drawRow("Edge Signal Low", [&] { ImGui::TextUnformatted(diag.edgeSignalTooLowLatched ? "active" : "inactive"); });
        drawRow("Fake Pressure", [&] { ImGui::Text("%s  framesLeft=%u", diag.fakePressureDecreaseActive ? "active" : "inactive", static_cast<unsigned int>(diag.fakePressureDecreaseFramesLeft)); });
        if (diag.tiltAnomalyDamped) {
            drawRow("Tilt Anomaly", [&] { ImGui::TextColored(BadColor(), "damping active"); });
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();
}

void DiagnosticsWorkbench::DrawStylusPacketDetails() {
    const auto& stylus = m_currentFrame.stylus;
    const auto& input = stylus.input;
    const auto& packet = stylus.output.packet;

    ImGui::TextWrapped("Raw stylus HID packet mirror and parser input details for the current frame.");
    ImGui::BeginChild("StylusPacketScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (ImGui::BeginTable("StylusPacketsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Packet", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Valid", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("RID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Length", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("StylusPacket");
        ImGui::TableSetColumnIndex(1); ImGui::TextColored(StatusColor(packet.valid), "%s", packet.valid ? "Valid" : "Invalid");
        ImGui::TableSetColumnIndex(2); ImGui::Text("0x%02X", static_cast<unsigned int>(packet.reportId));
        ImGui::TableSetColumnIndex(3); ImGui::Text("%u", static_cast<unsigned int>(packet.length));
        ImGui::TableSetColumnIndex(4);
        if (packet.valid) {
            const std::string bytes = StylusPacketBytes(packet);
            ImGui::TextUnformatted(bytes.c_str());
        } else {
            ImGui::TextDisabled("N/A");
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::TextColored(InfoColor(), "Parser Input");
    if (ImGui::BeginTable("StylusParserInputTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        auto drawRow = [](const char* label, const auto& drawValue) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(label);
            ImGui::TableSetColumnIndex(1);
            drawValue();
        };

        drawRow("slaveValid", [&] { ImGui::TextColored(StatusColor(input.slaveValid), "%s", YesNo(input.slaveValid)); });
        drawRow("checksumOk", [&] { ImGui::TextColored(StatusColor(input.checksumOk), "%s", YesNo(input.checksumOk)); });
        drawRow("slaveWordOffset", [&] { ImGui::Text("%u", static_cast<unsigned int>(input.slaveWordOffset)); });
        drawRow("checksum16", [&] { ImGui::Text("0x%04X", static_cast<unsigned int>(input.checksum16)); });
        drawRow("tx1BlockValid", [&] { ImGui::TextColored(StatusColor(input.tx1BlockValid), "%s", YesNo(input.tx1BlockValid)); });
        drawRow("tx2BlockValid", [&] { ImGui::TextColored(StatusColor(input.tx2BlockValid), "%s", YesNo(input.tx2BlockValid)); });
        drawRow("status", [&] { ImGui::Text("0x%08X", static_cast<unsigned int>(input.status)); });
        drawRow("hpp2LineValid", [&] { ImGui::TextColored(StatusColor(input.hpp2LineValid), "%s", YesNo(input.hpp2LineValid)); });
        drawRow("masterSuffixValid", [&] { ImGui::TextColored(StatusColor(m_currentFrame.masterSuffixValid), "%s", YesNo(m_currentFrame.masterSuffixValid)); });
        drawRow("slaveSuffixValid", [&] { ImGui::TextColored(StatusColor(m_currentFrame.slaveSuffixValid), "%s", YesNo(m_currentFrame.slaveSuffixValid)); });
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::TextColored(InfoColor(), "BT Sample");
    if (ImGui::BeginTable("StylusBtSampleTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        auto drawRow = [](const char* label, const auto& drawValue) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(label);
            ImGui::TableSetColumnIndex(1);
            drawValue();
        };

        drawRow("hasSample", [&] { ImGui::TextColored(StatusColor(input.btSample.hasSample), "%s", YesNo(input.btSample.hasSample)); });
        drawRow("seq", [&] { ImGui::Text("%u", static_cast<unsigned int>(input.btSample.seq)); });
        drawRow("pressure", [&] {
            const std::string values = FourU16ValuesText(input.btSample.pressure);
            ImGui::TextUnformatted(values.c_str());
        });
        drawRow("rawPressure", [&] {
            const std::string values = FourU16ValuesText(input.btSample.rawPressure);
            ImGui::TextUnformatted(values.c_str());
        });
        drawRow("freq1", [&] { ImGui::Text("%u", static_cast<unsigned int>(input.btSample.freq1)); });
        drawRow("freq2", [&] { ImGui::Text("%u", static_cast<unsigned int>(input.btSample.freq2)); });
        ImGui::EndTable();
    }

    ImGui::EndChild();
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
    auto pen = m_proxy ? m_proxy->GetPenIdentityStatus() : App::PenIdentityStatus{};

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
    ImGui::Text("Current Pen Identity");
    ImGui::Text("Pen Connection:");
    ImGui::SameLine();
    if (pen.connected)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "CONNECTED");
    else
        ImGui::TextDisabled("DISCONNECTED / UNKNOWN");

    if (pen.hasStylusId)
        ImGui::Text("Current Stylus ID: %u (0x%02X)", pen.stylusId, pen.stylusId);
    else
        ImGui::TextDisabled("Current Stylus ID: Unknown");

    if (pen.hasPenModuleModelId) {
        const std::string modelText = PenModuleModelText(pen);
        ImGui::Text("Pen Module Model ID: %s", modelText.c_str());
    } else {
        ImGui::TextDisabled("Pen Module Model ID: Unknown");
    }

    ImGui::TextUnformatted("Hardware Version:");
    ImGui::SameLine();
    if (pen.hasHardwareVersion && !pen.hardwareVersion.empty())
        ImGui::TextUnformatted(pen.hardwareVersion.c_str());
    else
        ImGui::TextDisabled("Unknown");

    ImGui::Separator();
    ImGui::Text("Manual Command Queries (Direct)");
#ifdef _DEBUG
    const bool mcuQueryAllowed = m_proxy && m_proxy->IsLiveControlAllowed();
#else
    const bool mcuQueryAllowed = false;
#endif

    if (!mcuQueryAllowed) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Query Hardware Version (0x0201)", ImVec2(-1, 24))) {
        m_proxy->TriggerQueryHardwareVersion();
    }
    if (ImGui::Button("Query Pen Status (0x7101)", ImVec2(-1, 24))) {
        m_proxy->TriggerQueryPenStatus();
    }
    if (ImGui::Button("Query MCU Status (0x7701)", ImVec2(-1, 24))) {
        m_proxy->TriggerQueryPenInfo();
    }
    if (!mcuQueryAllowed) {
        ImGui::EndDisabled();
    }
    if (!mcuQueryAllowed) {
        ImGui::TextDisabled("Manual queries are available in live-control debug mode only.");
    }

    ImGui::Separator();
    ImGui::Text("Pressure Range Mode");

    int pressureMode = ps.pressureMode == 0 ? 0 : 1;
    const char* pressureModes[] = {"4096 raw12", "16382 raw14 / 4"};
#ifdef _DEBUG
    if (ImGui::Combo("##PenPressureMode", &pressureMode, pressureModes, 2) && m_proxy) {
        m_proxy->SetPenPressureMode(static_cast<uint8_t>(pressureMode));
    }
#else
    ImGui::TextDisabled("%s", pressureModes[pressureMode]);
    ImGui::TextWrapped("Release builds do not live-mutate Service pen pressure mode.");
#endif

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
