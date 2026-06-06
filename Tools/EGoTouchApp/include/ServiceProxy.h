#pragma once
// ServiceProxy: App-side IPC proxy replacing RuntimeOrchestrator.
// Connects to EGoTouchService via Named Pipe + Shared Memory.

#include "PenButtonConfig.h"
#include "Ipc/IpcPipeClient.h"
#include "Ipc/SharedFrameBuffer.h"
#include "Ipc/ConfigSync.h"
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
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
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
    void SaveConfig();
    void RefreshConfigSnapshot();
    // Applies ConfigStore edits to the app-local preview pipelines only.
    // Does not live-apply Service-side pipelines; no-op when runtime config is disabled.
    void ApplyConfigStoreToLocalRuntime();
    const Config::ConfigSchemaSnapshot& GetConfigSchemaSnapshot() const { return m_configSchema; }
    Config::ConfigStore& GetConfigStore() { return m_configStore; }
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
    PenBridgeStatus GetPenBridgeStatus() const {
        std::lock_guard<std::mutex> lk(m_penMutex);
        return m_penStatus;
    }

    // Local performance stats
    int  GetAcquisitionFps() const { return m_fps.load(); }
    int  GetSlaveAcquisitionFps() const { return m_slaveFps.load(); }

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
    Dvr::DvrDynamicDebugFrameSlot CaptureDvrDynamicDebugFrameSlot(uint64_t dvrSeq) const;
    DvrRuntimeConfigSnapshot CaptureRuntimeConfigSnapshot() const;
    void InitConfigSchema();

    static constexpr const wchar_t* kSharedMemName =
        L"Global\\EGoTouchSharedFrame";
    static constexpr int kDvrCapacity = 960;
    static constexpr size_t kDvrPreTriggerFrames = 480;

    Ipc::IpcPipeClient    m_client;
    Ipc::SharedFrameReader m_frameReader;
    Solvers::TouchPipeline m_pipeline;
    Solvers::StylusPipeline m_stylusPipeline;
    Config::ConfigSchemaSnapshot m_configSchema;
    Config::ConfigStore m_configStore;
    Config::ConfigStore m_configDefaults;

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

    // Dynamic debug schema/value cache
    mutable std::mutex m_dynamicDebugMutex;
    std::vector<DynamicDebugField> m_dynamicDebugFields;
    std::unordered_map<uint16_t, DynamicDebugValue> m_dynamicDebugValues;
    std::atomic<uint16_t> m_dynamicSchemaVersion{0};
    std::atomic<uint32_t> m_dynamicSchemaHash{0};
    DvrDynamicDebugSchema m_lastDvrDynamicSchema;

    bool RefreshDynamicDebugSchema();
    bool PollDynamicDebugSnapshot();

    // FPS measurement
    std::atomic<int> m_fps{0};
    std::atomic<int> m_slaveFps{0};

    // Global Service config mirrors
    std::atomic<bool> m_srvDesiredModeFull{true};
    std::atomic<bool> m_srvActiveModeFull{true};
    std::atomic<bool> m_srvAutoMode{true};
    std::atomic<bool> m_srvStylusVhfEnabled{true};
    std::atomic<PenButtonMode> m_srvPenButtonMode{PenButtonMode::OemCustom};
    std::atomic<PenButtonRoute> m_srvPenButtonRoute{PenButtonRoute::VhfOnly};
};

} // namespace App
