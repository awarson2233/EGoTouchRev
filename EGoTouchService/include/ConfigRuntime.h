#pragma once

#include "ConfigTarget.h"
#include "ServiceConfigCore.h"
#include "Ipc/IpcProtocol.h"
#include "config/ConfigPath.h"
#include "config/ConfigStore.h"
#include "config/ConfigTlv.h"
#include "config/ConfigValue.h"
#include "config/ConfigSchemaSnapshot.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace Service {

class ConfigRuntime {
public:
    using StartupValidator = std::function<bool(const Config::ConfigStore&)>;

    struct V3ApplyResult {
        Ipc::IpcStatusCode ipcStatus = Ipc::IpcStatusCode::Ok;
        Ipc::ConfigV3MutationStatus status = Ipc::ConfigV3MutationStatus::Ok;
        size_t entryCount = 0;
        size_t changedCount = 0;
        size_t appliedCount = 0;
        size_t restartRequiredCount = 0;
        size_t rejectedCount = 0;
        Config::ConfigKeyId failedKeyId = Config::ConfigKeyId::MaxKeyId;
        Config::ConfigValueType failedValueType = Config::ConfigValueType::Null;
        ServiceConfigState desiredServiceConfig{};
        Config::ConfigStore pipelineConfig{};
        std::vector<ConfigTargetResult> targetResults;
        std::vector<ConfigApplyAction> applyActions;
    };

    struct V3PersistResult {
        Ipc::IpcStatusCode ipcStatus = Ipc::IpcStatusCode::Ok;
        Ipc::ConfigV3MutationStatus status = Ipc::ConfigV3MutationStatus::Ok;
        size_t persistedCount = 0;
        size_t skippedCount = 0;
        size_t failedCount = 0;
    };

    struct ConfigV3Blob {
        std::vector<uint8_t> bytes;
        uint32_t schemaVersion = 0;
        uint32_t snapshotVersion = 0;
        uint32_t checksum = 0;
    };

    ConfigRuntime();

    static Config::ConfigStore BuildFactoryDefaultStore();
    static Config::ConfigSchemaSnapshot BuildFactoryDefaultSchema();

    void RegisterConfigTarget(std::unique_ptr<IConfigTarget> target);
    bool Initialize(const std::string& configPath, const StartupValidator& validateStartupConfig);
    ConfigV3Blob BuildCatalogV3Blob() const;
    ConfigV3Blob BuildSnapshotV3Blob() const;
    Config::ConfigStore SnapshotStore() const;
    ServiceConfigState ServiceState() const;
    void WriteServiceState(const ServiceConfigState& config);
    bool PersistServicePolicyConfig(const ServiceConfigState& config);
    V3ApplyResult ApplyConfigPatchV3(uint32_t baseSchemaVersion,
                                     uint32_t baseSnapshotVersion,
                                     const uint8_t* data,
                                     size_t size);
    V3PersistResult PersistConfigV3();

private:
    bool ValidateStartupConfig(const Config::ConfigStore& store, const StartupValidator& validateStartupConfig) const;
    ServiceConfigState ReadServiceConfigStateFromStoreLocked(const Config::ConfigStore& store,
                                                             bool penButtonRouteExplicit) const;
    ServiceConfigState ReadActiveServiceConfigStateLocked() const;
    void WriteServiceConfigStateToStoreLocked(Config::ConfigStore& store,
                                              bool& penButtonRouteExplicit,
                                              const ServiceConfigState& config);
    bool ApplyPatchPayloadLocked(const uint8_t* data,
                                 size_t size,
                                 V3ApplyResult& result);
    void RegisterDefaultConfigTargets();

    // Lock boundary: ConfigRuntime owns config store/schema/paths/TLV session state.
    // Callers must not invoke DeviceRuntime or IPC callbacks while m_mutex is held.
    mutable std::mutex m_mutex;
    Config::ConfigStore m_defaults;
    Config::ConfigStore m_store;
    Config::ConfigStore m_activeStore;
    Config::ConfigSchemaSnapshot m_schema;
    std::optional<Config::ConfigPaths> m_paths;
    bool m_penButtonRouteExplicit = false;
    bool m_activePenButtonRouteExplicit = false;
    std::vector<std::unique_ptr<IConfigTarget>> m_targets;
};

} // namespace Service
