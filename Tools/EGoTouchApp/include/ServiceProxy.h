#pragma once
// ServiceProxy: App-side IPC proxy replacing RuntimeOrchestrator.
// Connects to EGoTouchService via Named Pipe + Shared Memory.

#include "IpcPipeClient.h"
#include "SharedFrameBuffer.h"
#include "ConfigSync.h"
#include "EngineTypes.h"
#include "IpcProtocol.h"
#include "FramePipeline.h"
#include "StylusPipeline.h"
#include "ConcurrentRingBuffer.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>

namespace App {

// Lightweight mirror of Service-side Pen channel status (no PenBridge.h dependency)
struct PenBridgeStatus {
    bool     evtRunning   = false;  // col00 事件通道运行中
    bool     pressRunning = false;  // col01 压力通道运行中
    uint8_t  reportType   = 0;
    uint8_t  freq1        = 0;
    uint8_t  freq2        = 0;
    uint16_t press[4]     = {0,0,0,0};
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

    // Auto-discovery: background thread retries Connect periodically
    void StartAutoDiscovery(int intervalMs = 2000);
    void StopAutoDiscovery();

    // Manual one-shot connect attempt (non-blocking, returns success)
    bool TryConnect();

    // Frame access (reads shared memory)
    bool GetLatestFrame(Engine::HeatmapFrame& out);

    // Pipeline for GUI config UI (local copy)
    Engine::FramePipeline& GetPipeline() { return m_pipeline; }
    Engine::StylusPipeline& GetStylusPipeline() { return m_stylusPipeline; }

    // Remote commands
    bool SwitchAfeMode(uint8_t afeCmd, uint8_t param = 0);
    bool StartRemoteRuntime();
    bool StopRemoteRuntime();

    // Config sync
    void SaveConfig();
    void LoadConfig();
    void NotifyConfigDirty();

    // VHF control (forwarded to Service via IPC)
    bool SetVhfEnabled(bool enabled);
    bool SetVhfTranspose(bool enabled);
    bool IsVhfEnabled() const { return m_vhfEnabled.load(); }
    bool IsVhfTransposeEnabled() const { return m_vhfTranspose.load(); }

    // Auto AFE freq-shift sync
    bool SetAutoAfeSync(bool enabled);
    bool IsAutoAfeSyncEnabled() const { return m_autoAfeSync.load(); }

    // MasterParser-only mode (local pipeline control)
    void SetMasterParserOnlyMode(bool enabled);
    bool IsMasterParserOnlyMode() const { return m_masterParserOnly; }

    // Local DVR export
    void TriggerDVRExport(bool heatmap, bool master, bool slave);

    // Global Service config (UI mirrors)
    bool IsSrvModeFull() const { return m_srvModeFull; }
    void SetSrvModeFull(bool full) { m_srvModeFull = full; }
    bool IsSrvStylusVhfEnabled() const { return m_srvStylusVhfEnabled; }
    void SetSrvStylusVhfEnabled(bool enabled) { m_srvStylusVhfEnabled = enabled; }
    bool IsSrvAutoMode() const { return m_srvAutoMode; }
    void SetSrvAutoMode(bool enabled) { m_srvAutoMode = enabled; }

    // PenBridge status (polled from Service)
    PenBridgeStatus GetPenBridgeStatus() const {
        std::lock_guard<std::mutex> lk(m_penMutex);
        return m_penStatus;
    }

    // Local performance stats
    int  GetAcquisitionFps() const { return m_fps.load(); }

private:
    static constexpr const wchar_t* kSharedMemName =
        L"Global\\EGoTouchSharedFrame";
    static constexpr int kDvrCapacity = 480;

    Ipc::IpcPipeClient    m_client;
    Ipc::SharedFrameReader m_frameReader;
    Ipc::ConfigDirtyFlag  m_configDirty;
    Engine::FramePipeline m_pipeline;
    Engine::StylusPipeline m_stylusPipeline;

    // Latest frame snapshot for GUI
    std::mutex m_frameMutex;
    Engine::HeatmapFrame m_latestFrame;
    std::atomic<bool> m_hasNewFrame{false};

    // Polling thread reads shared memory
    std::atomic<bool> m_polling{false};
    std::thread m_pollThread;
    void PollLoop();
    HANDLE m_pollStopEvent = nullptr;
    HANDLE m_logEvent = nullptr;
    HANDLE m_penEvent = nullptr;

    // Auto-discovery thread
    std::atomic<bool> m_discovering{false};
    std::thread m_discoveryThread;
    int m_discoveryIntervalMs = 2000;
    void DiscoveryLoop();

    // DVR ring buffer (local)
    std::unique_ptr<RingBuffer<Engine::HeatmapFrame, 480>> m_dvrBuffer;

    // Remote state mirrors
    std::atomic<bool> m_vhfEnabled{true};
    std::atomic<bool> m_vhfTranspose{false};
    std::atomic<bool> m_autoAfeSync{true};

    // MasterParser-only mode
    bool m_masterParserOnly = false;
    std::vector<bool> m_savedProcessorStates;

    // PenBridge status (polled alongside logs)
    mutable std::mutex m_penMutex;
    PenBridgeStatus m_penStatus;

    // FPS measurement
    std::atomic<int> m_fps{0};

    // Global Service config mirrors
    bool m_srvModeFull = true;
    bool m_srvAutoMode = true;
    bool m_srvStylusVhfEnabled = true;
};

} // namespace App
