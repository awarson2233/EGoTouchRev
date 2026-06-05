#include "ServiceProxyInternal.h"
#include "Ipc/IpcProtocol.h"
#include "Logger.h"
#include "SolverBuildConfig.h"
#include <cstring>


namespace App {

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
