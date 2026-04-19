#include "ServiceProxyInternal.h"
#include "IpcProtocol.h"
#include "Logger.h"
#include <cstring>
#include <fstream>
#include <iterator>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace App {

void ServiceProxy::SaveConfig() {
    if (!IsLiveControlAllowed()) return;

    std::ifstream in(kConfigPath, std::ios::binary);
    std::string existingText;
    if (in.is_open()) {
        existingText.assign(std::istreambuf_iterator<char>(in),
                            std::istreambuf_iterator<char>());
    }

    const TouchPipelineModuleEnableState* persistedModuleState = nullptr;
    if (m_masterParserOnly && m_masterParserOnlySnapshot.has_value()) {
        persistedModuleState = &*m_masterParserOnlySnapshot;
    }

    const std::string mergedConfig = MergeServiceProxyConfigSections(
        existingText,
        BuildServiceConfigSection(m_srvModeFull, m_srvAutoMode, m_srvStylusVhfEnabled),
        BuildTouchPipelineConfigSection(m_pipeline, persistedModuleState),
        BuildStylusPipelineConfigSection(m_stylusPipeline));

    const std::string tempPath = kConfigPath + ".tmp";
    {
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) return;
        out << mergedConfig;
        out.flush();
        if (!out.good()) return;
    }

    const std::wstring configPathWide(kConfigPath.begin(), kConfigPath.end());
    const std::wstring tempPathWide(tempPath.begin(), tempPath.end());
    if (!MoveFileExW(tempPathWide.c_str(), configPathWide.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const DWORD err = GetLastError();
        DeleteFileW(tempPathWide.c_str());
        LOG_WARN("App", __func__, "Config", "Failed to replace config file: {}", static_cast<unsigned long>(err));
        return;
    }

    // Notify Service to reload from config.ini
    m_configDirty.SetDirty();
    const auto resp = m_client.ReloadConfig();
    if (!resp.success) {
        LOG_WARN("App", __func__, "IPC", "Config saved but ReloadConfig IPC failed.");
        return;
    }

    if (resp.dataLen >= sizeof(Ipc::ReloadConfigSummaryWire)) {
        Ipc::ReloadConfigSummaryWire summary{};
        std::memcpy(&summary, resp.data, sizeof(summary));
        if (summary.restartRequiredFields != 0u) {
            LOG_WARN("App", __func__, "IPC",
                     "Config saved/reloaded. Restart required for some [Service] fields: changed=0x{:02X} applied_now=0x{:02X} restart_required=0x{:02X}.",
                     static_cast<unsigned int>(summary.changedFields),
                     static_cast<unsigned int>(summary.appliedFields),
                     static_cast<unsigned int>(summary.restartRequiredFields));
        } else {
            LOG_INFO("App", __func__, "IPC",
                     "Config saved/reloaded: changed=0x{:02X} applied_now=0x{:02X} restart_required=0x{:02X}.",
                     static_cast<unsigned int>(summary.changedFields),
                     static_cast<unsigned int>(summary.appliedFields),
                     static_cast<unsigned int>(summary.restartRequiredFields));
        }
    } else {
        LOG_INFO("App", __func__, "IPC", "Config saved and Service notified to reload.");
    }
}

void ServiceProxy::LoadConfig() {
    std::ifstream in(kConfigPath);
    if (!in.is_open()) return;
    std::string line, section;
    while (std::getline(in, line)) {
        const std::string trimmed = TrimCopy(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            section = TrimCopy(std::string_view(trimmed).substr(1, trimmed.size() - 2));
            continue;
        }

        std::string key;
        std::string value;
        if (!ParseIniKeyValue(trimmed, key, value)) continue;

        if (section == "Service") {
            if (key == "mode") m_srvModeFull = (value == "full");
            else if (key == "auto_mode") m_srvAutoMode = (value == "1" || value == "true");
            else if (key == "stylus_vhf_enabled") m_srvStylusVhfEnabled = (value == "1" || value == "true");
        } else if (section == "TouchPipeline") {
            m_pipeline.LoadConfig(key, value);
        } else if (section == "StylusPipeline") {
            m_stylusPipeline.LoadConfig(key, value);
        } else if (IsLegacyTouchSection(section)) {
            const auto mappedKey = MapLegacyTouchKey(section, key);
            if (mappedKey.has_value()) {
                m_pipeline.LoadConfig(*mappedKey, value);
            }
        }
    }
}

void ServiceProxy::NotifyConfigDirty() {
    m_configDirty.SetDirty();
}

void ServiceProxy::SetSrvModeFull(bool full) {
    if (!IsLiveControlAllowed()) return;
    m_srvModeFull = full;
}

void ServiceProxy::SetSrvStylusVhfEnabled(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvStylusVhfEnabled = enabled;
}

void ServiceProxy::SetSrvAutoMode(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvAutoMode = enabled;
}

// ── MasterParser-only mode (local) ──
void ServiceProxy::SetMasterParserOnlyMode(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    if (enabled == m_masterParserOnly) return;

    if (enabled) {
        m_masterParserOnlySnapshot = CaptureTouchPipelineModuleEnableState(m_pipeline);
        TouchPipelineModuleEnableState parserOnlyState = *m_masterParserOnlySnapshot;
        parserOnlyState.baselineEnabled = false;
        parserOnlyState.cmfEnabled = false;
        parserOnlyState.gridIIREnabled = false;
        parserOnlyState.trackerEnabled = false;
        parserOnlyState.coordFilterEnabled = false;
        parserOnlyState.gestureEnabled = false;
        ApplyTouchPipelineModuleEnableState(m_pipeline, parserOnlyState);
    } else if (m_masterParserOnlySnapshot.has_value()) {
        ApplyTouchPipelineModuleEnableState(m_pipeline, *m_masterParserOnlySnapshot);
        m_masterParserOnlySnapshot.reset();
    }

    m_masterParserOnly = enabled;
}

} // namespace App
