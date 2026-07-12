#pragma once
// ServiceProxy: App-side IPC proxy replacing RuntimeOrchestrator.
// Connects to EGoTouchService via Named Pipe + Shared Memory.

#include "PenButtonConfig.h"
#include "Ipc/IpcPipeClient.h"
#include "Ipc/SharedFrameBuffer.h"
#include "SolverTypes.h"
#include "ServiceProxyTypes.h"
#include "Ipc/IpcProtocol.h"
#include "TouchSolver/TouchPipeline.h"
#include "StylusPipeline.h"
#include "ConcurrentRingBuffer.h"
#include "DvrFrameSlot.h"
#include "config/ConfigSchemaSnapshot.h"
#include "config/ConfigStore.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace App {

struct TouchPipelineModuleEnableState {
    bool baselineEnabled = true;
    bool cmfEnabled = true;
    bool gridIIREnabled = true;
    bool trackerEnabled = true;
    bool coordFilterEnabled = true;
    bool gestureEnabled = true;
};

enum class ApplyConfigStatus : uint8_t {
    NotAttempted = 0,
    NoChanges,
    LiveApplyFailed,
    LiveApplied,
    RestartRequired,
};

enum class ConfigServiceSyncState : uint8_t {
    OfflineFallback = 0,
    Syncing,
    Ready,
    Failed,
};

struct ApplyConfigResult {
    ApplyConfigStatus status = ApplyConfigStatus::NotAttempted;
    bool liveApplied = false;
    bool restartRequired = false;
    bool persistAttempted = false;
    bool persisted = false;
    bool unpersistedLiveChanges = false;
    Ipc::IpcStatusCode persistStatus = Ipc::IpcStatusCode::InternalError;
};

enum class ConfigDraftApplyState : uint8_t {
    Clean = 0,
    Pending,
    LiveApplied,
    StagedRestartRequired,
    Failed,
};

enum class ConfigDraftPersistState : uint8_t {
    NotAttempted = 0,
    Persisted,
    Unpersisted,
    Failed,
};

struct ConfigDraftPathState {
    bool dirty = false;
    bool hasServiceSnapshot = false;
    bool hasDirtyBaseline = false;
    ConfigDraftApplyState applyState = ConfigDraftApplyState::Clean;
    ConfigDraftPersistState persistState = ConfigDraftPersistState::NotAttempted;
    Ipc::IpcStatusCode persistStatus = Ipc::IpcStatusCode::InternalError;
    uint8_t mutationStatus = static_cast<uint8_t>(Ipc::ConfigV3MutationStatus::Ok);
    Config::ConfigKeyId failedKeyId = Config::ConfigKeyId::MaxKeyId;
    uint32_t baselineSchemaVersion = 0;
    uint32_t baselineSnapshotVersion = 0;
    std::string errorMessage;
};

struct ConfigV3BaselineVersions {
    uint32_t catalogSchemaVersion = 0;
    uint32_t catalogSnapshotVersion = 0;
    uint32_t snapshotSchemaVersion = 0;
    uint32_t snapshotVersion = 0;
};

class ServiceProxy {
public:
    ServiceProxy();
    ~ServiceProxy();
    ServiceProxy(const ServiceProxy&) = delete;
    ServiceProxy& operator=(const ServiceProxy&) = delete;

    // Connection lifecycle
    bool Connect();           // Blocking connect attempt
    void Disconnect();
    bool IsConnected() const;

    // Manual one-shot connect attempt
    bool TryConnect();

    // Frame access (reads shared memory / playback dataset)
    bool GetLatestFrame(Solvers::HeatmapFrame& out);
    bool GetCurrentFrame(Solvers::HeatmapFrame& out);
    void UpdatePlayback();

    // DVR playback/import
    bool LoadDvrDataset(const std::filesystem::path& inputPath);
    bool ExportLoadedDvrDatasetToCsv(const std::filesystem::path& outputDirectory, std::string* outError = nullptr) const;
    void UnloadDvrDataset();
    bool HasPlaybackDataset() const;
    void SetFrameSourceMode(FrameSourceMode mode);
    FrameSourceMode GetFrameSourceMode() const { return m_frameSourceMode.load(); }
    bool IsLiveControlAllowed() const { return m_frameSourceMode.load() == FrameSourceMode::Live; }
    int GetPlaybackFormatVersion() const { return m_playbackFormatVersion.load(); }
    uint32_t GetPlaybackFlags() const { return m_playbackFlags.load(); }
    std::string GetPlaybackStatusMessage() const;
    void PlayPlayback();
    void PausePlayback();
    bool IsPlaybackPlaying() const { return m_playbackPlaying.load(); }
    void StepPlaybackForward();
    void StepPlaybackBackward();
    void SeekPlaybackFrame(size_t index);
    void SeekPlaybackTimeUs(uint64_t timeUs);
    size_t GetPlaybackFrameIndex() const { return m_playbackFrameIndex.load(); }
    size_t GetPlaybackFrameCount() const;
    uint64_t GetPlaybackCurrentTimeUs() const { return m_playbackCurrentTimeUs.load(); }
    uint64_t GetPlaybackStartTimeUs() const;
    uint64_t GetPlaybackEndTimeUs() const;
    uint64_t GetPlaybackCurrentSourceTimeUs() const;
    uint64_t GetPlaybackCurrentHostReceiveTimeUs() const;
    PlaybackTimingMode GetPlaybackTimingMode() const;

    // Pipeline for GUI config UI (local copy)
    Solvers::TouchPipeline& GetPipeline() { return m_pipeline; }
    Solvers::StylusPipeline& GetStylusPipeline() { return m_stylusPipeline; }

    // Remote commands
    bool SwitchAfeMode(uint8_t afeCmd, uint8_t param = 0);
    bool StartRemoteRuntime();
    bool StopRemoteRuntime();

    // Config sync
    // Connected mode uses v3 catalog/snapshot for reads and v3 patch/persist for apply/save.
    // App-local binder/YAML state is preserved only for offline/local fallback and v3 fetch failures.
    bool SynchronizeConfigFromServiceForEditing();
    bool IsConfigAdjustmentAllowed() const;
    ConfigServiceSyncState GetConfigServiceSyncState() const { return m_configServiceSyncState.load(std::memory_order_relaxed); }
    std::string GetConfigServiceSyncStatusMessage() const;
    void SaveConfig();
    bool ApplyConfigStoreGlobally();
    ApplyConfigResult GetLastApplyConfigResult() const;
    bool HasUnpersistedLiveConfigChanges() const { return m_hasUnpersistedLiveConfigChanges.load(std::memory_order_relaxed); }
    void MarkConfigPathsDirty(const std::vector<std::string>& paths);
    void RefreshConfigSnapshot();
    bool ApplyConfigV3CatalogBytesForTest(const uint8_t* data, size_t size);
    bool ApplyConfigV3SnapshotBytesForTest(const uint8_t* data, size_t size, bool overwriteDirtyDraft = false);
    ConfigV3BaselineVersions GetConfigV3BaselineVersionsForTest() const;
#if defined(EGOTOUCH_APP_SERVICE_PROXY_TEST)
    void SetConfigV3IpcTestResponses(bool connected,
                                     const Ipc::IpcResponse& applyResponse,
                                     const Ipc::IpcResponse& persistResponse);
    void SetConfigV3IpcTestResponses(bool connected,
                                     std::vector<Ipc::IpcResponse> applyResponses,
                                     const Ipc::IpcResponse& persistResponse,
                                     std::vector<uint8_t> snapshotBytes = {});
    bool HasLastConfigV3ApplyRequestForTest() const { return m_hasLastConfigV3ApplyRequestForTest; }
    Ipc::ApplyConfigPatchV3RequestWire GetLastConfigV3ApplyRequestForTest() const { return m_lastConfigV3ApplyRequestForTest; }
    int GetConfigV3ApplyRequestCountForTest() const { return m_configV3ApplyRequestCountForTest; }
    int GetConfigV3PersistRequestCountForTest() const { return m_configV3PersistRequestCountForTest; }
#endif
    // Applies ConfigStore edits to the app-local preview pipelines only.
    // Does not live-apply Service-side pipelines; no-op when runtime config is disabled.
    void ApplyConfigStoreToLocalRuntime();
    const Config::ConfigSchemaSnapshot& GetConfigSchemaSnapshot() const { return m_configSchema; }
    const Config::ConfigStore& GetConfigDraftStore() const { return m_configDraft.editableDraft; }
    Config::ConfigStore& GetMutableConfigDraftStoreForUi() { return m_configDraft.editableDraft; }
    const Config::ConfigStore& GetConfigStore() const { return GetConfigDraftStore(); }
    void SetConfigDraftValue(std::string_view path, Config::ConfigValue value);
    void CommitConfigDraftEdits(const std::vector<std::string>& paths);
    ConfigDraftPathState GetConfigDraftPathState(std::string_view path) const;
#if defined(EGOTOUCH_APP_SERVICE_PROXY_TEST)
    const Config::ConfigStore& GetServiceConfigSnapshotStoreForTest() const { return m_configDraft.serviceSnapshot; }
#endif
    std::vector<std::string> GetConfigModuleTags() const;

    // VHF control (forwarded to Service via IPC)
    bool SetVhfEnabled(bool enabled);
    bool SetVhfTranspose(bool enabled);
    bool IsVhfEnabled() const { return m_vhfEnabled.load(); }
    bool IsVhfTransposeEnabled() const { return m_vhfTranspose.load(); }

    // MasterParser-only mode (local pipeline control)
    void SetMasterParserOnlyMode(bool enabled);
    bool IsMasterParserOnlyMode() const { return m_masterParserOnly.load(std::memory_order_relaxed); }

    // Local DVR export
    void TriggerDvrBinaryExport();
    bool IsDvrExporting() const { return m_dvrExporting.load(); }

    // Global Service config (UI mirrors)
    // NOTE: IsSrvModeFull() returns desired mode (config target), not runtime active mode.
    bool IsSrvModeFull() const { return m_srvDesiredModeFull.load(std::memory_order_relaxed); }
    bool IsSrvActiveModeFull() const { return m_srvActiveModeFull.load(std::memory_order_relaxed); }
    void SetSrvModeFull(bool full);
    bool IsSrvStylusVhfEnabled() const { return m_srvStylusVhfEnabled.load(std::memory_order_relaxed); }
    void SetSrvStylusVhfEnabled(bool enabled);
    PenButtonMode GetPenButtonMode() const { return m_srvPenButtonMode.load(std::memory_order_relaxed); }
    void SetPenButtonMode(PenButtonMode m);
    PenButtonRoute GetPenButtonRoute() const { return m_srvPenButtonRoute.load(std::memory_order_relaxed); }
    void SetPenButtonRoute(PenButtonRoute r);
    bool IsSrvAutoMode() const { return m_srvAutoMode.load(std::memory_order_relaxed); }
    void SetSrvAutoMode(bool enabled);

    // PenBridge status (polled from Service)
    bool SetPenPressureMode(uint8_t mode);
    bool TriggerQueryHardwareVersion();
    bool TriggerQueryPenStatus();
    bool TriggerQueryPenInfo();
    bool TriggerSendScanMode(uint8_t freq1, uint8_t freq2, uint8_t mode);
    bool TriggerSendPairInfoSet(uint8_t value);
    PenBridgeStatus GetPenBridgeStatus() const {

        std::lock_guard<std::mutex> lk(m_penMutex);
        return m_penStatus;
    }
    PenIdentityStatus GetPenIdentityStatus() const {
        std::lock_guard<std::mutex> lk(m_penMutex);
        return m_penIdentityStatus;
    }

    // Local performance stats
    int  GetAcquisitionFps() const { return m_fps.load(); }
    int  GetSlaveAcquisitionFps() const { return m_slaveFps.load(); }
    ServiceRuntimeStatus GetServiceRuntimeStatus() const;
    std::vector<ServiceRuntimeTransition> GetServiceRuntimeTransitions() const;

    // Dynamic debug schema/value access
    uint16_t GetDynamicDebugSchemaVersion() const { return m_dynamicSchemaVersion.load(); }
    uint32_t GetDynamicDebugSchemaHash() const { return m_dynamicSchemaHash.load(); }
    std::vector<DynamicDebugField> GetDynamicDebugFields() const;
    bool GetDynamicDebugValue(uint16_t fieldId, DynamicDebugValue& out) const;
    DvrDynamicDebugSchema GetCurrentDvrDynamicDebugSchema() const { return CaptureDynamicDebugSchema(); }
    DvrDynamicDebugFrame GetCurrentDvrDynamicDebugFrame() const { return CaptureDynamicDebugFrame(); }

private:
    DvrDynamicDebugSchema CaptureDynamicDebugSchema() const;
    DvrDynamicDebugFrame CaptureDynamicDebugFrame() const;
    Dvr::DvrDynamicDebugFrameSlot CaptureDynamicDebugFrameSlot(uint64_t dvrSeq) const;
    bool RefreshDynamicDebugSnapshot(uint64_t* outFrameTimestamp = nullptr);
    void ClearDynamicDebugState();
    DvrRuntimeConfigSnapshot CaptureRuntimeConfigSnapshot() const;
    void UpdateServiceRuntimeStatusFromSharedFrame(const Ipc::SharedFrameData& frame,
                                                   uint64_t frameId,
                                                   uint64_t appReceiveEpochUs);
    void UpdateServiceRuntimeStatusFromWire(const Ipc::RuntimeStatusWire& wire,
                                            uint64_t appReceiveEpochUs);
    void ResetServiceRuntimeStatus();
    void InitConfigSchema();
    void SetConfigServiceSyncState(ConfigServiceSyncState state, std::string message);
    bool RefreshConfigCatalogV3();
    bool RefreshConfigSnapshotV3(bool overwriteDirtyDraft = false);
    std::optional<std::vector<uint8_t>> FetchConfigV3Bytes(Ipc::ConfigV3PayloadKind payloadKind);
    bool ApplyConfigV3CatalogBytes(const uint8_t* data, size_t size);
    bool ApplyConfigV3SnapshotBytes(const uint8_t* data, size_t size, bool overwriteDirtyDraft = false);
    bool IsConfigIpcConnectedForApply() const;
    Ipc::IpcResponse SendApplyConfigPatchV3Request(const Ipc::ApplyConfigPatchV3RequestWire& request);
    Ipc::IpcResponse SendPersistConfigV3Request();

    static constexpr const wchar_t* kSharedMemName =
        L"Global\\EGoTouchSharedFrame";
    static constexpr int kDvrCapacity = 960;
    static constexpr size_t kDvrPreTriggerFrames = 480;
    static constexpr size_t kServiceRuntimeTransitionCapacity = 64;

    Ipc::IpcPipeClient    m_client;
    Ipc::SharedFrameReader m_frameReader;
    Solvers::TouchPipeline m_pipeline;
    Solvers::StylusPipeline m_stylusPipeline;
    Config::ConfigSchemaSnapshot m_configSchema;
    struct ConfigDraft {
        Config::ConfigStore catalogDefaults;
        Config::ConfigStore serviceSnapshot;
        Config::ConfigStore editableDraft;
        std::unordered_map<std::string, Config::ConfigValue> dirtyBaseline;
        std::unordered_map<std::string, ConfigDraftPathState> pathStates;
        uint32_t catalogSchemaVersion = 0;
        uint32_t catalogSnapshotVersion = 0;
        uint32_t snapshotSchemaVersion = 0;
        uint32_t snapshotVersion = 0;
    };
    ConfigDraft m_configDraft;
    bool m_configV3CatalogReady = false;
    std::atomic<ConfigServiceSyncState> m_configServiceSyncState{ConfigServiceSyncState::OfflineFallback};
    mutable std::mutex m_configServiceSyncMessageMutex;
    std::string m_configServiceSyncStatusMessage{"Service config has not been synchronized."};
    std::atomic<ApplyConfigStatus> m_lastApplyConfigStatus{ApplyConfigStatus::NotAttempted};
    std::atomic<bool> m_lastApplyConfigLiveApplied{false};
    std::atomic<bool> m_lastApplyConfigRestartRequired{false};
    std::atomic<bool> m_lastApplyConfigPersistAttempted{false};
    std::atomic<bool> m_lastApplyConfigPersisted{false};
    std::atomic<bool> m_hasUnpersistedLiveConfigChanges{false};
    std::atomic<uint8_t> m_lastApplyConfigPersistStatus{static_cast<uint8_t>(Ipc::IpcStatusCode::InternalError)};
#if defined(EGOTOUCH_APP_SERVICE_PROXY_TEST)
    bool m_configV3IpcTestConnected = false;
    Ipc::IpcResponse m_configV3IpcTestApplyResponse{};
    std::vector<Ipc::IpcResponse> m_configV3IpcTestApplyResponses;
    size_t m_configV3IpcTestApplyResponseIndex = 0;
    Ipc::IpcResponse m_configV3IpcTestPersistResponse{};
    std::vector<uint8_t> m_configV3IpcTestSnapshotBytes;
    Ipc::ApplyConfigPatchV3RequestWire m_lastConfigV3ApplyRequestForTest{};
    bool m_hasLastConfigV3ApplyRequestForTest = false;
    int m_configV3ApplyRequestCountForTest = 0;
    int m_configV3PersistRequestCountForTest = 0;
#endif

    // Latest frame snapshot for GUI
    std::mutex m_frameMutex;
    Solvers::HeatmapFrame m_latestFrame;
    std::atomic<bool> m_hasNewFrame{false};

    // Polling thread reads shared memory
    mutable std::mutex m_connectionMutex;
    std::atomic<bool> m_polling{false};
    std::thread m_pollThread;
    void PollLoop();
    void DisconnectLocked();
    HANDLE m_pollStopEvent = nullptr;
    HANDLE m_logEvent = nullptr;
    HANDLE m_penEvent = nullptr;

    // DVR ring buffer (POD slots — zero heap allocation per frame)
    std::unique_ptr<RingBuffer<Dvr::DvrFrameSlot, kDvrCapacity>> m_dvrBuffer;
    std::unique_ptr<RingBuffer<Dvr::DvrDynamicDebugFrameSlot, kDvrCapacity>> m_dvrDynamicDebugBuffer;
    std::atomic<uint64_t> m_dvrSeqCounter{0};
    std::atomic<bool> m_dvrExporting{false};
    std::thread m_dvrThread;

    // Playback state
    DvrPlaybackDataset m_playbackDataset;
    mutable std::mutex m_playbackMutex;
    std::string m_playbackStatusMessage;
    std::atomic<FrameSourceMode> m_frameSourceMode{FrameSourceMode::Live};
    std::atomic<bool> m_playbackPlaying{false};
    std::atomic<size_t> m_playbackFrameIndex{0};
    std::atomic<uint64_t> m_playbackCurrentTimeUs{0};
    std::atomic<int> m_playbackFormatVersion{0};
    std::atomic<uint32_t> m_playbackFlags{0};
    std::chrono::steady_clock::time_point m_lastPlaybackAdvance{};

    // Remote state mirrors
    std::atomic<bool> m_vhfEnabled{true};
    std::atomic<bool> m_vhfTranspose{false};

    // MasterParser-only mode
    std::atomic<bool> m_masterParserOnly{false};
    std::optional<TouchPipelineModuleEnableState> m_masterParserOnlySnapshot;

    // PenBridge status (polled alongside logs)
    mutable std::mutex m_penMutex;
    PenBridgeStatus m_penStatus;
    PenIdentityStatus m_penIdentityStatus;

    // Dynamic debug schema/value cache
    mutable std::mutex m_dynamicDebugMutex;
    std::vector<DynamicDebugField> m_dynamicDebugFields;
    std::unordered_map<uint16_t, DynamicDebugValue> m_dynamicDebugValues;
    std::atomic<uint16_t> m_dynamicSchemaVersion{0};
    std::atomic<uint32_t> m_dynamicSchemaHash{0};
    DvrDynamicDebugSchema m_lastDvrDynamicSchema;
    std::chrono::steady_clock::time_point m_nextDynamicDebugSchemaRetry{};

    bool RefreshDynamicDebugSchema();

    // FPS measurement
    std::atomic<int> m_fps{0};
    std::atomic<int> m_slaveFps{0};

    // Service runtime status mirror
    mutable std::mutex m_serviceRuntimeMutex;
    ServiceRuntimeStatus m_serviceRuntimeStatus;
    std::vector<ServiceRuntimeTransition> m_serviceRuntimeTransitions;

    // Global Service config mirrors
    std::atomic<bool> m_srvDesiredModeFull{true};
    std::atomic<bool> m_srvActiveModeFull{true};
    std::atomic<bool> m_srvAutoMode{true};
    std::atomic<bool> m_srvStylusVhfEnabled{true};
    std::atomic<PenButtonMode> m_srvPenButtonMode{PenButtonMode::OemCustom};
    std::atomic<PenButtonRoute> m_srvPenButtonRoute{PenButtonRoute::VhfOnly};
};

} // namespace App
