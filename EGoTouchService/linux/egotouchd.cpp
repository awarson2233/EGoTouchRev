// egotouchd —— Linux 上的触摸守护进程：himax-thp 取帧 → TSACore 解算 → uhid 触摸屏。
//
// 相当于 Windows 上 DeviceRuntime 的手指部分，按它的流式一帧走同样的几步（取帧、笔管线、TSACore、
// AFE 闭环、上报），建链与重建链简化成一个循环：总线就绪才 Init，失败退避重试；SOS 或总线掉线（灭屏）
// 时抬起所有触点、停掉解算，回到等总线。笔还没有上报，只把笔连接状态交给 TSACore。
//
//   egotouchd [--dev PATH] [--transpose] [--master-only]
//
//   --dev PATH      设备节点，默认 /dev/himax_thp
//   --transpose     与 Windows 的 VHF transpose 开关相同：换一种坐标轴映射
//   --master-only   不用 slave（笔）
//
// 需要能读写设备节点和 /dev/uhid（默认只有 root）。

#include "FrameLayout.h"
#include "Logger.h"
#include "himax/HimaxChip.h"
#include "Port/TouchSink.h"
#include "StylusSolver/StylusPipeline.h"
#include "TouchSolver/TsaCore/PanelProfile.h"
#include "TouchSolver/TsaCore/TsaCoreSolver.h"
#include "vhf/VhfReporterTouchPacketHelper.h"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>

namespace {

using Clock = std::chrono::steady_clock;

volatile std::sig_atomic_t g_stop = 0;

void OnSignal(int) { g_stop = 1; }

// 不带 SA_RESTART：阻塞在取帧里的 read 被信号打断后以 NoData 返回，主循环马上看到 g_stop
void InstallSignals() {
    struct sigaction sa {};
    sa.sa_handler = OnSignal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

void SleepUnlessStopped(std::chrono::milliseconds d) {
    const auto until = Clock::now() + d;
    while (!g_stop && Clock::now() < until) std::this_thread::sleep_for(std::chrono::milliseconds{20});
}

constexpr auto kBusPollInterval = std::chrono::milliseconds{200};
// 连续这么久没有帧就去探一次总线：TSACore 让 IC 进 IDLE 之后帧会变慢，但不会停这么久
constexpr auto kNoDataProbeAfter = std::chrono::seconds{1};

std::chrono::milliseconds Backoff(int attempts) {
    return std::chrono::milliseconds{std::min(200 << std::min(attempts - 1, 5), 5000)};
}

class Daemon {
public:
    Daemon(const std::filesystem::path& dev, bool slave, bool transpose, Port::ITouchSink& sink)
        : m_chip(dev, dev), m_transpose(transpose), m_sink(sink) {
        m_chip.SetSlaveEnabled(slave);
    }

    int Run() {
        if (!m_chip.IsReady(Himax::DeviceType::Master) || !m_chip.IsReady(Himax::DeviceType::Slave)) {
            LOG_ERROR("egotouchd", __func__, "open", "Cannot open the bus device (himax-thp bound? running as root?).");
            return 1;
        }
        while (!g_stop) {
            if (!BringUp()) continue;
            Stream();
            LeaveStreaming();
        }
        m_chip.Quiesce();
        (void)m_chip.Deinit(true);
        return 0;
    }

private:
    // 总线就绪才 Init；返回 true 表示已经进入流式
    bool BringUp() {
        const auto probe = m_chip.ProbeBus();
        if (!probe.ioOk || !probe.Ready()) {
            if (!m_waitingForBus) {
                LOG_NOTICE("egotouchd", __func__, "bus", "Waiting for the bus (io={}, master=0x{:02X}, slave=0x{:02X}).",
                           probe.ioOk, probe.master, probe.slave);
                m_waitingForBus = true;
            }
            SleepUnlessStopped(kBusPollInterval);
            return false;
        }
        m_waitingForBus = false;

        m_chip.Quiesce();
        if (auto res = m_chip.Init(); !res) {
            ++m_attempts;
            const auto delay = Backoff(m_attempts);
            LOG_NOTICE("egotouchd", __func__, "init", "Init attempt {} failed at '{}' ({}); retrying in {} ms.",
                       m_attempts, m_chip.LastInitFailStep(), ToString(res.error()), delay.count());
            SleepUnlessStopped(delay);
            return false;
        }
        m_attempts = 0;
        // IDLE 由 TSACore 请求
        m_chip.SetAutoIdleEnabled(false);

        const auto panel = m_chip.GetPanelIdentity();
        const std::string_view projectId = Solvers::Tsa::ResolvePanelProjectId(panel ? panel->projectId : "");
        if (!panel || panel->projectId != projectId) {
            LOG_WARN("egotouchd", __func__, "panel", "Panel project id unavailable ({}), using {}.",
                     panel ? panel->projectId : std::string("OTP read failed"), projectId);
        }
        if (projectId != m_tsa.PanelProjectId() && !m_tsa.SetPanelProjectId(projectId)) {
            LOG_WARN("egotouchd", __func__, "panel", "Panel changed to {} after TSACore loaded {}; keeping it.",
                     projectId, m_tsa.PanelProjectId());
        }
        m_tsa.OnAfeStarted();
        LOG_NOTICE("egotouchd", __func__, "streaming", "Chip initialized, streaming (panel {}).", m_tsa.PanelProjectId());
        return true;
    }

    void Stream() {
        auto lastFrame = Clock::now();
        while (!g_stop) {
            const auto read = m_chip.GetFrame();
            if (read == Himax::Chip::FrameRead::Sos) {
                LOG_NOTICE("egotouchd", __func__, "sos", "AFE SOS: {}; restarting the link.", m_chip.LastSosReason());
                m_tsa.OnAfeSos();
                return;
            }
            if (read == Himax::Chip::FrameRead::NoData) {
                if (Clock::now() - lastFrame < kNoDataProbeAfter) continue;
                // 灭屏时 TDDI 的触摸部分跟着掉电，帧停了、寄存器读回 0
                if (const auto probe = m_chip.ProbeBus(); !probe.Ready()) {
                    LOG_NOTICE("egotouchd", __func__, "bus", "No frames and the bus is not ready; leaving streaming.");
                    return;
                }
                lastFrame = Clock::now();
                continue;
            }
            lastFrame = Clock::now();
            OnFrame();
        }
    }

    void OnFrame() {
        const auto& raw = m_chip.GetFrameBuffer();
        Solvers::HeatmapFrame frame;
        frame.rawPtr = raw.data();
        frame.rawLen = Frame::kTotalFrameSize;
        frame.masterWasRead = m_chip.GetLastMasterWasRead();
        frame.timestamp = m_chip.GetLastFrameTimestamp();

        m_stylus.Process(frame);
        m_tsa.SetStylusConnected(m_chip.IsStylusConnected());
        if (m_tsa.Process(frame)) DispatchTouch(frame);
        ExecuteAfeActions(m_tsa.PendingAfeActions());
    }

    // 与 VhfReporter::DispatchTouch 相同：有触点就发，触点从有到无时补一次全部抬起
    void DispatchTouch(Solvers::HeatmapFrame& frame) {
        const auto packets = VhfTouchPacket::Build(frame.touch.output.contacts.span(), m_transpose);
        const bool hasTouch = packets[0].valid;
        if (!hasTouch) {
            if (m_hadTouch) m_sink.ReleaseAll();
            m_hadTouch = false;
            return;
        }
        m_hadTouch = true;
        for (const auto& p : packets) {
            if (p.valid && !m_sink.WriteReport(p.bytes.data(), p.length)) {
                LOG_WARN("egotouchd", __func__, "uhid", "uhid write failed: errno {}.", m_sink.GetError());
                return;
            }
        }
    }

    void ExecuteAfeActions(const Solvers::Tsa::AfeActionList& actions) {
        using Solvers::Tsa::AfeActionType;
        for (const auto& action : actions) {
            command cmd{};
            cmd.param = action.param;
            switch (action.type) {
            case AfeActionType::ClearStatus: cmd.type = AFE_Command::ClearStatus; break;
            case AfeActionType::ForceToFreqPoint: cmd.type = AFE_Command::ForceToFreqPoint; break;
            case AfeActionType::ForceToScanRate: cmd.type = AFE_Command::ForceToScanRate; break;
            case AfeActionType::EnterIdle: cmd.type = AFE_Command::EnterIdle; break;
            case AfeActionType::StartCalibration: cmd.type = AFE_Command::StartCalibration; break;
            }
            const bool ok = static_cast<bool>(m_chip.SendAfeCommand(cmd));
            if (!ok) {
                LOG_WARN("egotouchd", __func__, "afe", "AFE command {} (param {}) failed.",
                         static_cast<int>(cmd.type), cmd.param);
            }
            m_tsa.OnAfeActionExecuted(action, ok);
        }
    }

    // 不再有帧来驱动抬起：在场的手指由这里统一收尾
    void LeaveStreaming() {
        m_tsa.OnAfeStopped();
        m_sink.ReleaseAll();
        m_hadTouch = false;
        m_chip.Quiesce();
    }

    Himax::Chip m_chip;
    Solvers::Tsa::TsaCoreSolver m_tsa;
    Solvers::StylusPipeline m_stylus;
    bool m_transpose;
    Port::ITouchSink& m_sink;
    bool m_hadTouch = false;
    bool m_waitingForBus = false;
    int m_attempts = 0;
};

} // namespace

int main(int argc, char** argv) {
    std::string dev = "/dev/himax_thp";
    bool transpose = false;
    bool slave = true;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--dev" && i + 1 < argc) dev = argv[++i];
        else if (a == "--transpose") transpose = true;
        else if (a == "--master-only") slave = false;
        else {
            std::fprintf(stderr, "usage: %s [--dev PATH] [--transpose] [--master-only]\n", argv[0]);
            return 2;
        }
    }

    InstallSignals();
    Common::Logger::Init("egotouchd", std::filesystem::temp_directory_path() / "egotouch-logs", nullptr);

    auto sink = Port::CreateTouchSink();
    if (!sink->IsValid()) {
        LOG_ERROR("egotouchd", __func__, "uhid", "Cannot create the uhid touchscreen: errno {}.", sink->GetError());
        Common::Logger::Shutdown();
        return 1;
    }
    int rc = 0;
    {
        Daemon daemon(dev, slave, transpose, *sink);
        rc = daemon.Run();
    }
    sink.reset();
    Common::Logger::Shutdown();
    return rc;
}
