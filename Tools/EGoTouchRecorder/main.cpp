// EGoTouchRecorder —— 给测试者用的只读录制工具。
//
// 双击运行：临时停下 EGoTouchService（录完自动恢复），用服务同一套 DeviceRuntime 给触摸芯片上电建链，
// 录制约 30 秒的帧（Himax hx83121a 的 master / slave 原始帧、TSACore 的触点、笔管线的输出），同时读 BT MCU
// 的事件通道（col00）和压力通道（col01）。结果写到桌面上的一个文件夹：
//   recording.dvrbin   DVR 录制（每帧附带 BT MCU 的状态与报文计数，见 BuildBtSchema）
//   btmcu.csv          BT MCU 的全部原始报文，带时间与所在帧号
//   device.txt         机型（BIOS）、面板型号（建链时从显示驱动 OTP 读出）、笔的型号与固件版本
//   EGoTouchRecorder.txt  运行日志
//
// 只读的含义：不向系统注入任何触摸 / 笔输入（VHF 关闭），不写芯片 flash、不改任何配置。芯片建链与
// BT MCU 握手 / ACK 是读数据的前提，与服务平时的行为相同。
//
//   EGoTouchRecorder.exe [--seconds N] [--out DIR] [--yes]
//
// 只能用 Debug 构建：DeviceRuntime 的逐帧回调只在 Debug 里编入，Release 保持服务的最优路径。

#ifndef _DEBUG
#error "EGoTouchRecorder needs the Debug build (DeviceRuntime only calls the frame callback in Debug)"
#endif

#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "DvrBinaryIO.h"
#include "DvrFrameSlot.h"
#include "Logger.h"
#include "penevt/PenEventBridge.h"
#include "penpress/PenPressureReader.h"
#include "runtime/DeviceRuntime.h"

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

namespace {

const std::wstring kDevicePathMaster = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
const std::wstring kDevicePathSlave = L"\\\\.\\Global\\SPBTESTTOOL_SLAVE";
const wchar_t* kServiceName = L"EGoTouchService";
constexpr int kDefaultSeconds = 30;
constexpr int kBringUpTimeoutSeconds = 30;
constexpr size_t kMaxFramesPerSecond = 300;  // 预留容量用，实际约 120–240 帧/秒

std::atomic<bool> g_stop{false};

BOOL WINAPI OnConsoleCtrl(DWORD) {
    g_stop.store(true);
    return TRUE;
}

uint64_t EpochUs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string Timestamp() {
    SYSTEMTIME t{};
    GetLocalTime(&t);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04u%02u%02u_%02u%02u%02u", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    return buf;
}

fs::path DesktopDir() {
    PWSTR path = nullptr;
    fs::path dir = fs::current_path();
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &path))) dir = path;
    CoTaskMemFree(path);
    return dir;
}

bool AskYes(const char* question) {
    std::printf("%s [Y/n] ", question);
    std::fflush(stdout);
    char line[16] = {};
    if (std::fgets(line, sizeof line, stdin) == nullptr) return false;
    return line[0] == '\n' || line[0] == 'y' || line[0] == 'Y';
}

void WaitForEnter() {
    std::printf("\n按回车键退出...");
    std::fflush(stdout);
    char line[8];
    (void)std::fgets(line, sizeof line, stdin);
}

// ── 服务：录制期间要独占触摸与 BT MCU 设备 ─────────────────────────────

class ServiceGuard {
public:
    ~ServiceGuard() { Restore(); }

    /// 服务在运行时征得同意后停下；返回 false 表示不能继续。
    bool StopIfRunning(bool assumeYes) {
        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (scm == nullptr) return true;
        m_service = OpenServiceW(scm, kServiceName, SERVICE_QUERY_STATUS | SERVICE_STOP | SERVICE_START);
        CloseServiceHandle(scm);
        if (m_service == nullptr) return true;  // 没装服务

        if (State() != SERVICE_RUNNING && State() != SERVICE_START_PENDING) return true;
        std::printf("EGoTouchService 正在运行，录制期间需要临时停止它（录完自动恢复）。\n");
        if (!assumeYes && !AskYes("现在停止服务并开始录制？")) return false;

        SERVICE_STATUS status{};
        if (!ControlService(m_service, SERVICE_CONTROL_STOP, &status) && GetLastError() != ERROR_SERVICE_NOT_ACTIVE) {
            std::printf("停止服务失败（错误 %lu）。\n", GetLastError());
            return false;
        }
        m_wasRunning = true;
        for (int i = 0; i < 200 && State() != SERVICE_STOPPED; ++i) Sleep(100);
        if (State() != SERVICE_STOPPED) {
            std::printf("服务没有在 20 秒内停下。\n");
            return false;
        }
        std::printf("服务已停止。\n");
        return true;
    }

    void Restore() {
        if (m_service == nullptr) return;
        if (m_wasRunning) {
            if (StartServiceW(m_service, 0, nullptr)) {
                std::printf("EGoTouchService 已重新启动。\n");
            } else {
                std::printf("重新启动 EGoTouchService 失败（错误 %lu），请手动启动或重启电脑。\n", GetLastError());
            }
            m_wasRunning = false;
        }
        CloseServiceHandle(m_service);
        m_service = nullptr;
    }

private:
    DWORD State() const {
        SERVICE_STATUS status{};
        return QueryServiceStatus(m_service, &status) ? status.dwCurrentState : 0;
    }

    SC_HANDLE m_service = nullptr;
    bool m_wasRunning = false;
};

// ── BT MCU 原始报文 ────────────────────────────────────────────────────

struct RawPacket {
    uint64_t hostUs;   // 相对录制开始
    uint64_t frameSeq; // 收到时已经录下的帧数
    char channel;      // 'E' 事件通道 col00，'P' 压力通道 col01
    std::vector<uint8_t> bytes;
};

class PacketLog {
public:
    void Begin(Clock::time_point start) {
        std::lock_guard lk(m_mu);
        m_start = start;
        m_packets.clear();
        m_recording = true;
    }
    void End() {
        std::lock_guard lk(m_mu);
        m_recording = false;
    }
    void Add(char channel, std::span<const uint8_t> packet, uint64_t frameSeq) {
        (channel == 'E' ? m_eventCount : m_pressCount).fetch_add(1);
        if (channel == 'E') m_lastEventCode.store(packet.empty() ? 0 : packet[0]);
        std::lock_guard lk(m_mu);
        if (!m_recording) return;
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - m_start).count();
        m_packets.push_back({static_cast<uint64_t>(std::max<int64_t>(us, 0)), frameSeq, channel, {packet.begin(), packet.end()}});
    }
    /// 上次调用以来两个通道各收到的报文数
    std::pair<uint32_t, uint32_t> TakeCounts() { return {m_eventCount.exchange(0), m_pressCount.exchange(0)}; }
    uint32_t LastEventCode() const { return m_lastEventCode.load(); }

    bool WriteCsv(const fs::path& path, size_t& written) const {
        std::lock_guard lk(m_mu);
        std::FILE* f = _wfopen(path.c_str(), L"w");
        if (f == nullptr) return false;
        std::fprintf(f, "host_us,frame_seq,channel,length,bytes\n");
        for (const auto& p : m_packets) {
            std::fprintf(f, "%llu,%llu,%s,%zu,", static_cast<unsigned long long>(p.hostUs),
                         static_cast<unsigned long long>(p.frameSeq), p.channel == 'E' ? "event" : "pressure", p.bytes.size());
            for (const uint8_t b : p.bytes) std::fprintf(f, "%02x", b);
            std::fputc('\n', f);
        }
        written = m_packets.size();
        return std::fclose(f) == 0;
    }

private:
    mutable std::mutex m_mu;
    Clock::time_point m_start{};
    bool m_recording = false;
    std::vector<RawPacket> m_packets;
    std::atomic<uint32_t> m_eventCount{0};
    std::atomic<uint32_t> m_pressCount{0};
    std::atomic<uint32_t> m_lastEventCode{0};
};

PacketLog g_packets;
std::atomic<uint64_t> g_frameSeq{0};

// 在服务原有的两个通道上截一份原始报文，其余行为不变
class RecordingEventBridge : public Himax::Pen::PenEventBridge {
public:
    ~RecordingEventBridge() override { Stop(); }

protected:
    void OnPacketReceived(std::span<const uint8_t> packet) override {
        g_packets.Add('E', packet, g_frameSeq.load());
        PenEventBridge::OnPacketReceived(packet);
    }
};

class RecordingPressureReader : public Himax::Pen::PenPressureReader {
public:
    ~RecordingPressureReader() override { Stop(); }

protected:
    void OnPacketReceived(std::span<const uint8_t> packet) override {
        g_packets.Add('P', packet, g_frameSeq.load());
        PenPressureReader::OnPacketReceived(packet);
    }
};

// ── 帧 ──────────────────────────────────────────────────────────────────

// 每帧附带的 BT MCU 字段（DVR 的动态调试区）
enum BtField : uint16_t {
    kPenStatusWord = 1,
    kPenConnected,
    kStylusId,
    kPenModeRaw,
    kEventPackets,
    kPressurePackets,
    kLastEventCode,
};

Dvr::DynamicDebugSchema BuildBtSchema() {
    Dvr::DynamicDebugSchema schema;
    schema.schemaVersion = 1;
    const auto add = [&](uint16_t id, Ipc::DebugValueType type, const char* key, const char* name) {
        Dvr::DynamicDebugField f;
        f.fieldId = id;
        f.valueType = type;
        f.sourceKind = Ipc::DebugSourceKind::PenBridgeField;
        f.uiOrder = static_cast<uint8_t>(id);
        f.dvrTarget = Ipc::DebugDvrTarget::DynamicDebug;
        f.key = key;
        f.displayName = name;
        f.uiGroup = "BtMcu";
        f.dvrColumnName = std::string("BT_") + key;
        schema.fields.push_back(std::move(f));
    };
    add(kPenStatusWord, Ipc::DebugValueType::UInt32, "pen_status_word", "Pen status word (factoryStatusFlags)");
    add(kPenConnected, Ipc::DebugValueType::Bool, "pen_connected", "Pen connected");
    add(kStylusId, Ipc::DebugValueType::UInt32, "stylus_id", "Stylus id");
    add(kPenModeRaw, Ipc::DebugValueType::UInt32, "pen_mode_raw", "Pen current mode (raw)");
    add(kEventPackets, Ipc::DebugValueType::UInt32, "event_packets", "col00 packets since previous frame");
    add(kPressurePackets, Ipc::DebugValueType::UInt32, "pressure_packets", "col01 packets since previous frame");
    add(kLastEventCode, Ipc::DebugValueType::UInt32, "last_event_code", "Last col00 event code");
    return schema;
}

struct Recording {
    std::mutex mu;
    bool active = false;
    Clock::time_point firstFrameAt{};
    uint64_t seenFrames = 0;   // 建链后收到的全部帧（含录制开始前）
    std::vector<Dvr::DvrFrameSlot> frames;
    std::vector<Dvr::DvrDynamicDebugFrameSlot> btFrames;
    std::optional<RuntimePenState> penIdentity;  // 最近一次带型号 / 固件信息的笔状态（断开后快照会清空）
};

// ── 设备信息（device.txt）────────────────────────────────────────────────

std::string Utf8(const std::wstring& w) {
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

std::string BiosString(const wchar_t* name) {
    wchar_t buf[256] = {};
    DWORD size = sizeof buf;
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", name, RRF_RT_REG_SZ, nullptr, buf,
                     &size) != ERROR_SUCCESS) {
        return "?";
    }
    return Utf8(buf);
}

// thp_afe_open_project 按 OTP 机型号选固件与笔表 / PixelGain 表的分支
std::string PanelVendor(const Himax::PanelIdentity& p) {
    if (p.projectId == "W273AS2700") return "CSOT";
    if (p.projectId == "W273AS2710") return "CSOT New";
    if (p.projectId == "W273AS1310") return "BOE New";
    if (p.projectId == "W273AS1300") return p.revisionCode == 'J' ? "BOE Old" : "BOE New";
    return "unknown";
}

std::string DescribeDevice(const std::optional<Himax::PanelIdentity>& panel, const std::optional<RuntimePenState>& pen) {
    std::string out;
    const auto line = [&](const char* key, const std::string& value) { out += std::string(key) + " = " + value + "\n"; };
    char hex[32];

    line("system.manufacturer", BiosString(L"SystemManufacturer"));
    line("system.product", BiosString(L"SystemProductName"));
    line("system.family", BiosString(L"SystemFamily"));
    line("system.sku", BiosString(L"SystemSKU"));
    line("system.bios", BiosString(L"BIOSVersion"));

    if (panel) {
        line("panel.project_id", panel->projectId + (panel->known ? "" : " (not in the stock id table)"));
        line("panel.vendor", PanelVendor(*panel));
        std::snprintf(hex, sizeof hex, "0x%02X", panel->revisionCode);
        line("panel.revision_code", hex);
        std::snprintf(hex, sizeof hex, "0x%02X", panel->e2);
        line("panel.e2h", hex);
    } else {
        line("panel.project_id", "? (OTP read failed, see EGoTouchRecorder.txt)");
    }

    if (pen && pen->hasPenModuleModelId) {
        std::snprintf(hex, sizeof hex, " (0x%06X)", pen->penModuleModelId);
        line("pen.model", std::string(Himax::Pen::ToString(pen->penModuleModel)) + hex);
    } else {
        line("pen.model", "? (no pen connected during the recording)");
    }
    if (pen && pen->hasFirmwareVersion) line("pen.firmware", pen->firmwareVersion);
    if (pen && pen->hasHardwareVersion) line("pen.hardware", pen->hardwareVersion);
    if (pen && pen->hasStylusId) line("pen.stylus_id", std::to_string(pen->stylusId));
    return out;
}

void SetSample(Dvr::DvrDynamicDebugFrameSlot& slot, uint16_t id, Ipc::DebugValueType type, uint64_t value) {
    auto& s = slot.samples[slot.sampleCount++];
    s.fieldId = id;
    s.valueType = static_cast<uint8_t>(type);
    s.valid = 1;
    s.rawValue = value;
}

}  // namespace

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCtrlHandler(OnConsoleCtrl, TRUE);

    int seconds = kDefaultSeconds;
    fs::path outRoot;
    bool assumeYes = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) seconds = std::clamp(std::atoi(argv[++i]), 1, 600);
        else if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc) outRoot = argv[++i];
        else if (std::strcmp(argv[i], "--yes") == 0) assumeYes = true;
        else {
            std::printf("usage: EGoTouchRecorder [--seconds N] [--out DIR] [--yes]\n");
            return 2;
        }
    }

    std::printf("EGoTouch 录制工具\n");
    std::printf("将录制 %d 秒的触摸 / 笔原始数据，录制期间不会产生任何触摸或笔输入。\n\n", seconds);

    const fs::path outDir = (outRoot.empty() ? DesktopDir() : outRoot) / ("EGoTouchRecording_" + Timestamp());
    std::error_code ec;
    fs::create_directories(outDir, ec);
    if (ec) {
        std::printf("无法创建输出目录 %s：%s\n", outDir.string().c_str(), ec.message().c_str());
        WaitForEnter();
        return 1;
    }

    ServiceGuard service;
    if (!service.StopIfRunning(assumeYes)) {
        WaitForEnter();
        return 1;
    }
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    Common::Logger::Init("EGoTouchRecorder", outDir, nullptr);

    Recording rec;
    rec.frames.reserve(static_cast<size_t>(seconds) * kMaxFramesPerSecond);
    rec.btFrames.reserve(rec.frames.capacity());

    auto runtime = std::make_unique<DeviceRuntime>(kDevicePathMaster, kDevicePathSlave, kDevicePathMaster);
    runtime->SetVhfEnabled(false);
    runtime->ApplyServicePolicy(/*autoMode=*/true, /*stylusVhfEnabled=*/false, PenButtonMode::OemCustom,
                                PenButtonRoute::VhfOnly, /*penButtonRouteExplicit=*/true);
    DeviceRuntime* rt = runtime.get();
    runtime->SetFramePushCallback([&rec, rt](const Solvers::HeatmapFrame& frame) {
        const uint64_t epochUs = EpochUs();
        const auto [eventPackets, pressurePackets] = g_packets.TakeCounts();
        std::lock_guard lk(rec.mu);
        if (rec.seenFrames++ == 0) rec.firstFrameAt = Clock::now();
        if (!rec.active || rec.frames.size() == rec.frames.capacity()) return;

        auto& slot = rec.frames.emplace_back();
        slot.CopyFrom(frame);
        slot.receiveSystemEpochUs = epochUs;
        slot.dvrSeq = rec.frames.size() - 1;

        const RuntimePenState pen = rt->GetPenStateSnapshot();
        if (pen.hasPenModuleModelId || pen.hasFirmwareVersion) rec.penIdentity = pen;
        auto& bt = rec.btFrames.emplace_back();
        bt.dvrSeq = slot.dvrSeq;
        SetSample(bt, kPenStatusWord, Ipc::DebugValueType::UInt32, pen.factoryStatusFlags);
        SetSample(bt, kPenConnected, Ipc::DebugValueType::Bool, pen.connected ? 1 : 0);
        SetSample(bt, kStylusId, Ipc::DebugValueType::UInt32, pen.hasStylusId ? pen.stylusId : 0);
        SetSample(bt, kPenModeRaw, Ipc::DebugValueType::UInt32, pen.currentModeRaw);
        SetSample(bt, kEventPackets, Ipc::DebugValueType::UInt32, eventPackets);
        SetSample(bt, kPressurePackets, Ipc::DebugValueType::UInt32, pressurePackets);
        SetSample(bt, kLastEventCode, Ipc::DebugValueType::UInt32, g_packets.LastEventCode());
        g_frameSeq.store(rec.frames.size());
    });

    // BT MCU：与服务相同的两个通道；笔没配对或通道打不开时只录触摸
    RecordingEventBridge eventBridge;
    RecordingPressureReader pressureReader;
    eventBridge.SetEventCallback([rt](const Himax::Pen::PenEvent& ev) { rt->IngestPenEvent(ev); });
    pressureReader.SetPressureCallback([rt](const Himax::Pen::PenPressureStats& s) {
        rt->IngestBtMcuPressurePacket({s.press[0], s.press[1], s.press[2], s.press[3]},
                                      {s.rawPress[0], s.rawPress[1], s.rawPress[2], s.rawPress[3]}, s.freq1, s.freq2);
    });
    const bool eventOk = eventBridge.Start();
    const bool pressureOk = pressureReader.Start();
    std::printf("BT MCU：事件通道%s，压力通道%s\n", eventOk ? "已打开" : "未打开", pressureOk ? "已打开" : "未打开");

    int exitCode = 0;
    if (!runtime->Start()) {
        std::printf("触摸运行时启动失败，详见 EGoTouchRecorder.txt。\n");
        exitCode = 1;
    } else {
        std::printf("正在给触摸芯片上电并建链（请保持屏幕点亮）...\n");
        const auto bringUpDeadline = Clock::now() + std::chrono::seconds(kBringUpTimeoutSeconds);
        while (!g_stop.load() && Clock::now() < bringUpDeadline) {
            {
                std::lock_guard lk(rec.mu);
                if (rec.seenFrames != 0) break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        bool gotFrames = false;
        {
            std::lock_guard lk(rec.mu);
            gotFrames = rec.seenFrames != 0;
        }
        if (!gotFrames) {
            std::printf("%d 秒内没有收到任何帧：触摸芯片没有建链。请确认屏幕亮着、驱动正常，详见 EGoTouchRecorder.txt。\n",
                        kBringUpTimeoutSeconds);
            exitCode = 1;
        } else {
            const auto start = Clock::now();
            g_packets.Begin(start);
            {
                std::lock_guard lk(rec.mu);
                rec.active = true;
            }
            std::printf("开始录制。请正常使用：多指点按、滑动、手掌放上去，有笔的话也用笔写一写。Ctrl+C 可提前结束。\n");
            for (int s = 1; s <= seconds && !g_stop.load(); ++s) {
                std::this_thread::sleep_until(start + std::chrono::seconds(s));
                size_t frames = 0;
                {
                    std::lock_guard lk(rec.mu);
                    frames = rec.frames.size();
                }
                std::printf("\r  %2d / %d 秒，已录 %zu 帧   ", s, seconds, frames);
                std::fflush(stdout);
            }
            std::printf("\n");
            std::lock_guard lk(rec.mu);
            rec.active = false;
            g_packets.End();
        }
    }

    // 面板型号在建链时从 OTP 读出，运行时停掉之前取
    const auto panel = runtime->GetPanelIdentity();

    // 先停数据源再写文件：回调与报文线程都结束之后 rec / g_packets 不再变化
    runtime->SetFramePushCallback(nullptr);
    pressureReader.Stop();
    eventBridge.Stop();
    runtime->Stop();
    runtime.reset();
    service.Restore();

    const std::string device = DescribeDevice(panel, rec.penIdentity);
    if (std::FILE* f = _wfopen((outDir / "device.txt").c_str(), L"w")) {
        std::fputs(device.c_str(), f);
        std::fclose(f);
    }
    std::printf("\n设备信息：\n%s", device.c_str());

    if (!rec.frames.empty()) {
        const fs::path dvrPath = outDir / "recording.dvrbin";
        const auto schema = BuildBtSchema();
        uint32_t flags = 0;
        // 回放按录制里的面板 id 选面板配置；OTP 没读出来就不写，回放走缺省机型
        const auto config = panel ? Dvr::RuntimeConfigSnapshot::WithPanelProjectId(panel->projectId)
                                  : Dvr::RuntimeConfigSnapshot{};
        const bool dvrOk = Dvr::WriteBinaryFile(dvrPath, rec.frames, &schema, &rec.btFrames,
                                                config.Empty() ? nullptr : &config, &flags);
        size_t packetCount = 0;
        const bool csvOk = g_packets.WriteCsv(outDir / "btmcu.csv", packetCount);
        size_t withMaster = 0;
        for (const auto& f : rec.frames) withMaster += f.masterWasRead ? 1 : 0;
        std::printf("\n录制完成：%zu 帧（其中 %zu 帧含手指数据），BT MCU 报文 %zu 条。\n", rec.frames.size(), withMaster,
                    packetCount);
        if (!dvrOk || !csvOk) {
            std::printf("写文件失败（dvrbin %s，btmcu.csv %s）。\n", dvrOk ? "成功" : "失败", csvOk ? "成功" : "失败");
            exitCode = 1;
        }
    } else if (exitCode == 0) {
        std::printf("没有录到帧。\n");
        exitCode = 1;
    }

    LOG_INFO("Recorder", "main", "Done", "exit code {}", exitCode);
    Common::Logger::Shutdown();
    std::printf("\n文件在：%s\n请把整个文件夹压缩后发回来。\n", outDir.string().c_str());
    WaitForEnter();
    return exitCode;
}
