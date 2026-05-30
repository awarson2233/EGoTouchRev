#include "ServiceProxyInternal.h"
#include "Ipc/IpcProtocol.h"
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

    const bool serviceConnected = m_client.IsConnected();
    const bool requestedSrvModeFull = m_srvDesiredModeFull.load(std::memory_order_relaxed);
    const bool requestedSrvAutoMode = m_srvAutoMode.load(std::memory_order_relaxed);
    const bool requestedSrvStylusVhfEnabled = m_srvStylusVhfEnabled.load(std::memory_order_relaxed);
    const PenButtonMode requestedPenButtonMode = m_srvPenButtonMode.load(std::memory_order_relaxed);
    const PenButtonRoute requestedPenButtonRoute = m_srvPenButtonRoute.load(std::memory_order_relaxed);

    std::string serviceSection = BuildServiceConfigSection(
        requestedSrvModeFull,
        requestedSrvAutoMode,
        requestedSrvStylusVhfEnabled,
        requestedPenButtonMode,
        requestedPenButtonRoute);

    Ipc::ConfigMutationResultWire patchSummary{};
    bool havePatchSummary = false;
    bool skipLocalServiceSection = false;

    // Thin-client primary path: Service is authoritative for [Service] config.
    if (serviceConnected) {
        Ipc::ApplyConfigPatchRequestWire patch{};
        patch.fieldMask = Ipc::ToBits(Ipc::ServiceConfigFieldWire::Mode) |
                          Ipc::ToBits(Ipc::ServiceConfigFieldWire::AutoMode) |
                          Ipc::ToBits(Ipc::ServiceConfigFieldWire::StylusVhfEnabled) |
                          Ipc::ToBits(Ipc::ServiceConfigFieldWire::PenButtonMode) |
                          Ipc::ToBits(Ipc::ServiceConfigFieldWire::PenButtonRoute);
        patch.desiredMode = static_cast<uint8_t>(requestedSrvModeFull ? Ipc::ServiceModeWire::Full
                                                                      : Ipc::ServiceModeWire::TouchOnly);
        patch.autoMode = requestedSrvAutoMode ? 1 : 0;
        patch.stylusVhfEnabled = requestedSrvStylusVhfEnabled ? 1 : 0;
        patch.penButtonMode = static_cast<uint8_t>(requestedPenButtonMode);
        patch.penButtonRoute = static_cast<uint8_t>(requestedPenButtonRoute);

        const auto applyResp = m_client.ApplyConfigPatch(patch);
        if (!applyResp.success) {
            LOG_WARN("App", __func__, "IPC", "ApplyConfigPatch failed with status={}", static_cast<unsigned int>(applyResp.status));
            return;
        }
        if (applyResp.dataLen >= sizeof(patchSummary)) {
            std::memcpy(&patchSummary, applyResp.data, sizeof(patchSummary));
            havePatchSummary = true;
        }

        const auto persistResp = m_client.PersistConfig();
        if (!persistResp.success) {
            LOG_WARN("App", __func__, "IPC", "ApplyConfigPatch succeeded but PersistConfig failed with status={}", static_cast<unsigned int>(persistResp.status));
            // Service owns [Service] config in connected mode. If persist fails,
            // do not write local [Service] section from client-side state.
            skipLocalServiceSection = true;
        }

        // Refresh mirrors from canonical snapshot (desired vs active semantics).
        const auto snapshotResp = m_client.GetConfigSnapshot();
        if (snapshotResp.success && snapshotResp.dataLen >= sizeof(Ipc::ConfigSnapshotWire)) {
            Ipc::ConfigSnapshotWire snapshot{};
            std::memcpy(&snapshot, snapshotResp.data, sizeof(snapshot));
            const bool desiredFull = snapshot.desiredMode == static_cast<uint8_t>(Ipc::ServiceModeWire::Full);
            const bool activeFull = snapshot.activeMode == static_cast<uint8_t>(Ipc::ServiceModeWire::Full);
            const bool autoMode = snapshot.autoMode != 0;
            const bool stylusVhfEnabled = snapshot.stylusVhfEnabled != 0;
            const auto penButtonMode = static_cast<PenButtonMode>(snapshot.penButtonMode);
            const auto penButtonRoute = static_cast<PenButtonRoute>(snapshot.penButtonRoute);

            m_srvDesiredModeFull.store(desiredFull, std::memory_order_relaxed);
            m_srvActiveModeFull.store(activeFull, std::memory_order_relaxed);
            m_srvAutoMode.store(autoMode, std::memory_order_relaxed);
            m_srvStylusVhfEnabled.store(stylusVhfEnabled, std::memory_order_relaxed);
            m_srvPenButtonMode.store(penButtonMode, std::memory_order_relaxed);
            m_srvPenButtonRoute.store(penButtonRoute, std::memory_order_relaxed);

            serviceSection = BuildServiceConfigSection(
                desiredFull, autoMode, stylusVhfEnabled,
                penButtonMode, penButtonRoute);
        } else {
            LOG_WARN("App", __func__, "IPC", "GetConfigSnapshot failed after patch/persist; skip local [Service] overwrite in connected mode.");
            skipLocalServiceSection = true;
        }
    }

    // Compatibility path: persist local pipeline sections for existing load/reload behavior.
    std::ifstream in(kConfigPath, std::ios::binary);
    std::string existingText;
    if (in.is_open()) {
        existingText.assign(std::istreambuf_iterator<char>(in),
                            std::istreambuf_iterator<char>());
        in.close();
    }

    const TouchPipelineModuleEnableState* persistedModuleState = nullptr;
    if (m_masterParserOnly.load(std::memory_order_relaxed) && m_masterParserOnlySnapshot.has_value()) {
        persistedModuleState = &*m_masterParserOnlySnapshot;
    }

    const std::string mergedConfig = MergeServiceProxyConfigSections(
        existingText,
        skipLocalServiceSection ? std::string_view{} : std::string_view(serviceSection),
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

    if (!serviceConnected) {
        LOG_INFO("App", __func__, "Config", "Config saved locally while service is disconnected.");
        return;
    }

    // Legacy compatibility: pipeline/stylus settings still apply via ReloadConfig.
    const auto reloadResp = m_client.ReloadConfig();
    if (!reloadResp.success) {
        LOG_WARN("App", __func__, "IPC", "Config patched/persisted and file updated, but ReloadConfig IPC failed with status={}", static_cast<unsigned int>(reloadResp.status));
    }

    Ipc::ReloadConfigSummaryWire reloadSummary{};
    if (reloadResp.success && reloadResp.dataLen >= sizeof(reloadSummary)) {
        std::memcpy(&reloadSummary, reloadResp.data, sizeof(reloadSummary));
    }

    Ipc::ConfigMutationResultWire summary{};
    if (havePatchSummary) {
        summary = patchSummary;
    } else {
        summary.changedFields = reloadSummary.changedFields;
        summary.appliedFields = reloadSummary.appliedFields;
        summary.restartRequiredFields = reloadSummary.restartRequiredFields;
    }

    if (summary.restartRequiredFields != 0u) {
        LOG_WARN("App", __func__, "IPC",
                 "Config patched/persisted: changed=0x{:02X} applied_now=0x{:02X} restart_required=0x{:02X}.",
                 static_cast<unsigned int>(summary.changedFields),
                 static_cast<unsigned int>(summary.appliedFields),
                 static_cast<unsigned int>(summary.restartRequiredFields));
    } else {
        LOG_INFO("App", __func__, "IPC",
                 "Config patched/persisted: changed=0x{:02X} applied_now=0x{:02X} restart_required=0x{:02X}.",
                 static_cast<unsigned int>(summary.changedFields),
                 static_cast<unsigned int>(summary.appliedFields),
                 static_cast<unsigned int>(summary.restartRequiredFields));
    }
}

void ServiceProxy::LoadConfig() {
    bool loadedServiceFromSnapshot = false;
    if (m_client.IsConnected()) {
        const auto resp = m_client.GetConfigSnapshot();
        if (resp.success && resp.dataLen >= sizeof(Ipc::ConfigSnapshotWire)) {
            Ipc::ConfigSnapshotWire snapshot{};
            std::memcpy(&snapshot, resp.data, sizeof(snapshot));
            m_srvDesiredModeFull.store(
                snapshot.desiredMode == static_cast<uint8_t>(Ipc::ServiceModeWire::Full),
                std::memory_order_relaxed);
            m_srvActiveModeFull.store(
                snapshot.activeMode == static_cast<uint8_t>(Ipc::ServiceModeWire::Full),
                std::memory_order_relaxed);
            m_srvAutoMode.store(snapshot.autoMode != 0, std::memory_order_relaxed);
            m_srvStylusVhfEnabled.store(snapshot.stylusVhfEnabled != 0, std::memory_order_relaxed);
            m_srvPenButtonMode.store(
                static_cast<PenButtonMode>(snapshot.penButtonMode),
                std::memory_order_relaxed);
            m_srvPenButtonRoute.store(
                static_cast<PenButtonRoute>(snapshot.penButtonRoute),
                std::memory_order_relaxed);
            loadedServiceFromSnapshot = true;
        }
    }

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
            if (!loadedServiceFromSnapshot) {
                if (key == "mode") {
                    const bool desiredFull = (value == "full");
                    m_srvDesiredModeFull.store(desiredFull, std::memory_order_relaxed);
                    // Offline fallback has no runtime distinction; mirror desired->active.
                    m_srvActiveModeFull.store(desiredFull, std::memory_order_relaxed);
                } else if (key == "auto_mode") {
                    m_srvAutoMode.store(value == "1" || value == "true", std::memory_order_relaxed);
                } else if (key == "stylus_vhf_enabled") {
                    m_srvStylusVhfEnabled.store(value == "1" || value == "true", std::memory_order_relaxed);
                } else if (key == "pen_button_mode") {
                    int ival = std::atoi(value.c_str());
                    m_srvPenButtonMode.store(
                        static_cast<PenButtonMode>(std::clamp(ival, 0, 2)),
                        std::memory_order_relaxed);
                } else if (key == "pen_button_route") {
                    int ival = std::atoi(value.c_str());
                    m_srvPenButtonRoute.store(
                        static_cast<PenButtonRoute>(std::clamp(ival, 0, 2)),
                        std::memory_order_relaxed);
                }
            }
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

void ServiceProxy::SetSrvModeFull(bool full) {
    if (!IsLiveControlAllowed()) return;
    m_srvDesiredModeFull.store(full, std::memory_order_relaxed);
}

void ServiceProxy::SetSrvStylusVhfEnabled(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvStylusVhfEnabled.store(enabled, std::memory_order_relaxed);
}

void ServiceProxy::SetSrvAutoMode(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvAutoMode.store(enabled, std::memory_order_relaxed);
}

void ServiceProxy::SetPenButtonMode(PenButtonMode m) {
    if (!IsLiveControlAllowed()) return;
    m_srvPenButtonMode.store(m, std::memory_order_relaxed);
}

void ServiceProxy::SetPenButtonRoute(PenButtonRoute r) {
    if (!IsLiveControlAllowed()) return;
    m_srvPenButtonRoute.store(r, std::memory_order_relaxed);
}

// ── MasterParser-only mode (local) ──
void ServiceProxy::SetMasterParserOnlyMode(bool enabled) {
    if (enabled == m_masterParserOnly.load(std::memory_order_relaxed)) return;

    if (IsConnected() && IsLiveControlAllowed()) {
        Ipc::IpcRequest req{};
        req.command = Ipc::IpcCommand::SetMasterParserOnly;
        req.param[0] = enabled ? 1 : 0;
        req.paramLen = 1;
        m_client.Send(req);
    }

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

    m_masterParserOnly.store(enabled, std::memory_order_relaxed);
}

} // namespace App
