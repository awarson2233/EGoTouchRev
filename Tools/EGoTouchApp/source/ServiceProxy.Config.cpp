#include "ServiceProxyInternal.h"
#include "Ipc/IpcProtocol.h"
#include "Logger.h"
#include "SolverBuildConfig.h"
#include "config/ConfigBinder.h"
#include "config/ConfigKeyMap.h"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace App {

namespace {

Config::ConfigValue ToConfigValue(bool value) {
    return Config::ConfigValue(value);
}

Config::ConfigValue ToConfigValue(PenButtonMode value) {
    return Config::ConfigValue(std::string(ToString(value)));
}

Config::ConfigValue ToConfigValue(PenButtonRoute value) {
    return Config::ConfigValue(std::string(ToString(value)));
}

void SetValue(Config::ConfigStore& store, std::string_view path, Config::ConfigValue value) {
    store.set<Config::ConfigValue>(path, std::move(value));
}

void PopulateServiceDefaults(Config::ConfigStore& store) {
    SetValue(store, "service.mode.full", ToConfigValue(true));
    SetValue(store, "service.auto_mode", ToConfigValue(true));
    SetValue(store, "service.stylus_vhf_enabled", ToConfigValue(true));
    SetValue(store, "service.pen_button_mode", ToConfigValue(PenButtonMode::OemCustom));
    SetValue(store, "service.pen_button_route", ToConfigValue(PenButtonRoute::VhfOnly));
}

void PopulateServiceValues(Config::ConfigStore& store,
                           bool desiredModeFull,
                           bool autoMode,
                           bool stylusVhfEnabled,
                           PenButtonMode penButtonMode,
                           PenButtonRoute penButtonRoute) {
    SetValue(store, "service.mode.full", ToConfigValue(desiredModeFull));
    SetValue(store, "service.auto_mode", ToConfigValue(autoMode));
    SetValue(store, "service.stylus_vhf_enabled", ToConfigValue(stylusVhfEnabled));
    SetValue(store, "service.pen_button_mode", ToConfigValue(penButtonMode));
    SetValue(store, "service.pen_button_route", ToConfigValue(penButtonRoute));
}

} // namespace

void ServiceProxy::InitConfigSchema() {
#if !EGOTOUCH_CONFIG_ENABLED
    return;
#endif

    Config::ConfigBinder binder;
    m_pipeline.registerBindings(binder);
    m_stylusPipeline.registerBindings(binder);

    m_configDefaults = Config::ConfigStore{};
    PopulateServiceDefaults(m_configDefaults);
    binder.writeDefaults(m_configDefaults);

    m_configStore = Config::ConfigStore{};
    m_configStore.mergeFrom(m_configDefaults);
    binder.writeCurrent(m_configStore);
    PopulateServiceValues(
        m_configStore,
        m_srvDesiredModeFull.load(std::memory_order_relaxed),
        m_srvAutoMode.load(std::memory_order_relaxed),
        m_srvStylusVhfEnabled.load(std::memory_order_relaxed),
        m_srvPenButtonMode.load(std::memory_order_relaxed),
        m_srvPenButtonRoute.load(std::memory_order_relaxed));

    m_configSchema = Config::BuildMergedSchema(m_configDefaults, binder);
}

void ServiceProxy::ApplyConfigStoreToLocalRuntime() {
#if !EGOTOUCH_CONFIG_ENABLED
    static std::atomic_bool loggedNoOp{false};
    if (!loggedNoOp.exchange(true, std::memory_order_relaxed)) {
        LOG_WARN("App", __func__, "Config", "Runtime config is disabled; app-local preview apply is a no-op and does not affect the Service-side live pipeline.");
    }
    return;
#endif

    m_pipeline.applyConfig(m_configStore);
    m_stylusPipeline.applyConfig(m_configStore);
}

std::vector<std::string> ServiceProxy::GetConfigModuleTags() const {
    std::vector<std::string> tags;
    tags.reserve(m_configSchema.entries.size());
    for (const auto& entry : m_configSchema.entries) {
        if (!entry.moduleTag.empty()) {
            tags.push_back(entry.moduleTag);
        }
    }
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    return tags;
}

void ServiceProxy::SaveConfig() {
#if !EGOTOUCH_CONFIG_ENABLED
    return;
#endif
    if (!IsLiveControlAllowed() || !m_client.IsConnected()) return;

    Ipc::ApplyConfigPatchRequestWire patch{};
    patch.fieldMask = Ipc::ToBits(Ipc::ServiceConfigFieldWire::Mode) |
                      Ipc::ToBits(Ipc::ServiceConfigFieldWire::AutoMode) |
                      Ipc::ToBits(Ipc::ServiceConfigFieldWire::StylusVhfEnabled) |
                      Ipc::ToBits(Ipc::ServiceConfigFieldWire::PenButtonMode) |
                      Ipc::ToBits(Ipc::ServiceConfigFieldWire::PenButtonRoute);
    patch.desiredMode = static_cast<uint8_t>(
        m_srvDesiredModeFull.load(std::memory_order_relaxed)
            ? Ipc::ServiceModeWire::Full
            : Ipc::ServiceModeWire::TouchOnly);
    patch.autoMode = m_srvAutoMode.load(std::memory_order_relaxed) ? 1 : 0;
    patch.stylusVhfEnabled = m_srvStylusVhfEnabled.load(std::memory_order_relaxed) ? 1 : 0;
    patch.penButtonMode = static_cast<uint8_t>(m_srvPenButtonMode.load(std::memory_order_relaxed));
    patch.penButtonRoute = static_cast<uint8_t>(m_srvPenButtonRoute.load(std::memory_order_relaxed));

    const auto applyResp = m_client.ApplyConfigPatch(patch);
    if (!applyResp.success) {
        LOG_WARN("App", __func__, "IPC", "ApplyConfigPatch failed with status={}", static_cast<unsigned int>(applyResp.status));
        return;
    }

    const auto persistResp = m_client.PersistConfig();
    if (!persistResp.success) {
        LOG_WARN("App", __func__, "IPC", "PersistConfig failed with status={}", static_cast<unsigned int>(persistResp.status));
        return;
    }

    const auto snapshotResp = m_client.GetConfigSnapshot();
    if (snapshotResp.success && snapshotResp.dataLen >= sizeof(Ipc::ConfigSnapshotWire)) {
        Ipc::ConfigSnapshotWire snapshot{};
        std::memcpy(&snapshot, snapshotResp.data, sizeof(snapshot));
        m_srvDesiredModeFull.store(snapshot.desiredMode == static_cast<uint8_t>(Ipc::ServiceModeWire::Full), std::memory_order_relaxed);
        m_srvActiveModeFull.store(snapshot.activeMode == static_cast<uint8_t>(Ipc::ServiceModeWire::Full), std::memory_order_relaxed);
        m_srvAutoMode.store(snapshot.autoMode != 0, std::memory_order_relaxed);
        m_srvStylusVhfEnabled.store(snapshot.stylusVhfEnabled != 0, std::memory_order_relaxed);
        m_srvPenButtonMode.store(static_cast<PenButtonMode>(snapshot.penButtonMode), std::memory_order_relaxed);
        m_srvPenButtonRoute.store(static_cast<PenButtonRoute>(snapshot.penButtonRoute), std::memory_order_relaxed);
        PopulateServiceValues(
            m_configStore,
            m_srvDesiredModeFull.load(std::memory_order_relaxed),
            m_srvAutoMode.load(std::memory_order_relaxed),
            m_srvStylusVhfEnabled.load(std::memory_order_relaxed),
            m_srvPenButtonMode.load(std::memory_order_relaxed),
            m_srvPenButtonRoute.load(std::memory_order_relaxed));
    }

    LOG_INFO("App", __func__, "IPC", "Config patch persisted through service IPC; local INI persistence is disabled.");
}

void ServiceProxy::RefreshConfigSnapshot() {
#if !EGOTOUCH_CONFIG_ENABLED
    return;
#endif
    if (!m_client.IsConnected()) return;

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
        PopulateServiceValues(
            m_configStore,
            m_srvDesiredModeFull.load(std::memory_order_relaxed),
            m_srvAutoMode.load(std::memory_order_relaxed),
            m_srvStylusVhfEnabled.load(std::memory_order_relaxed),
            m_srvPenButtonMode.load(std::memory_order_relaxed),
            m_srvPenButtonRoute.load(std::memory_order_relaxed));
    }
}

void ServiceProxy::SetSrvModeFull(bool full) {
    if (!IsLiveControlAllowed()) return;
    m_srvDesiredModeFull.store(full, std::memory_order_relaxed);
    SetValue(m_configStore, "service.mode.full", ToConfigValue(full));
}

void ServiceProxy::SetSrvStylusVhfEnabled(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvStylusVhfEnabled.store(enabled, std::memory_order_relaxed);
    SetValue(m_configStore, "service.stylus_vhf_enabled", ToConfigValue(enabled));
}

void ServiceProxy::SetSrvAutoMode(bool enabled) {
    if (!IsLiveControlAllowed()) return;
    m_srvAutoMode.store(enabled, std::memory_order_relaxed);
    SetValue(m_configStore, "service.auto_mode", ToConfigValue(enabled));
}

void ServiceProxy::SetPenButtonMode(PenButtonMode m) {
    if (!IsLiveControlAllowed()) return;
    m_srvPenButtonMode.store(m, std::memory_order_relaxed);
    SetValue(m_configStore, "service.pen_button_mode", ToConfigValue(m));
}

void ServiceProxy::SetPenButtonRoute(PenButtonRoute r) {
    if (!IsLiveControlAllowed()) return;
    m_srvPenButtonRoute.store(r, std::memory_order_relaxed);
    SetValue(m_configStore, "service.pen_button_route", ToConfigValue(r));
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
