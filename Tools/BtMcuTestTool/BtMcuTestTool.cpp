#include <windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <mutex>
#include <deque>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "btmcu/PenUsbTransport.h"
#include "Logger.h"

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ---- File Logger ----
static std::ofstream g_logFile;
static std::mutex g_logFileMutex;
static bool g_fileLoggingEnabled = true;
static std::string g_logFileName;

static std::string MakeLogFilename() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    struct tm parts;
    localtime_s(&parts, &now_c);
    char buf[128];
    snprintf(buf, sizeof(buf), "btmcu_%04d%02d%02d_%02d%02d%02d.log",
             parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday,
             parts.tm_hour, parts.tm_min, parts.tm_sec);
    return buf;
}

static void EnsureLogFileOpen() {
    if (g_logFile.is_open()) return;
    g_logFileName = MakeLogFilename();
    g_logFile.open(g_logFileName, std::ios::out | std::ios::app);
    if (g_logFile.is_open()) {
        g_logFile << "=== BtMcuTestTool Log Started ===" << std::endl;
    }
}

static void WriteLogLine(const std::string& time, const std::string& channel,
                          const std::string& dir, const std::vector<uint8_t>& data) {
    if (!g_fileLoggingEnabled) return;
    std::lock_guard<std::mutex> lock(g_logFileMutex);
    EnsureLogFileOpen();
    if (!g_logFile.is_open()) return;
    std::ostringstream oss;
    oss << time << " [" << channel << "] " << dir << " |";
    for (auto b : data) {
        oss << ' ' << std::uppercase << std::hex
            << std::setw(2) << std::setfill('0') << (int)b;
    }
    oss << '\n';
    g_logFile << oss.str();
    g_logFile.flush();
}

static void CloseLogFile() {
    std::lock_guard<std::mutex> lock(g_logFileMutex);
    if (g_logFile.is_open()) {
        g_logFile << "=== Log Closed ===" << std::endl;
        g_logFile.close();
    }
}

// Hex Dump state
struct LogMessage {
    std::string time;
    std::string channel; // "Evt" or "Press"
    std::string direction; // "Tx" or "Rx"
    std::vector<uint8_t> data;
};

static std::mutex g_logMutex;
static std::deque<LogMessage> g_logs;
static const size_t MAX_LOG_COUNT = 500;

static std::string GetTimeStr() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    struct tm parts;
    localtime_s(&parts, &now_c);
    char timeStr[64];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d.%03d",
             parts.tm_hour, parts.tm_min, parts.tm_sec, (int)ms.count());
    return timeStr;
}

void AddLog(const std::string& channel, const std::string& dir, const std::vector<uint8_t>& data) {
    auto ts = GetTimeStr();
    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        g_logs.push_back({ts, channel, dir, data});
        if (g_logs.size() > MAX_LOG_COUNT) {
            g_logs.pop_front();
        }
    }
    WriteLogLine(ts, channel, dir, data);
}

// Live pressure decode
static std::atomic<uint16_t> g_pressLive[4] = {0,0,0,0};
static std::atomic<uint8_t> g_pressFreq1{0}, g_pressFreq2{0};
static std::atomic<uint8_t> g_pressReportType{0};

#include <setupapi.h>
#include <hidsdi.h>
#include <optional>
#include <algorithm>
#include <cwctype>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

// ---- Channel identification ----
// Event channel: uses a dedicated device interface GUID (from THP_Service)
static const GUID EVENT_DEVICE_GUID = {0xdd0ebedb, 0xf1d6, 0x4cfa, {0xac, 0xca, 0x71, 0xe6, 0x6d, 0x31, 0x78, 0xca}};
// Pressure channel: matched by HID device path substring
static constexpr const wchar_t* PRESSURE_HID_MATCH = L"vid_12d1&pid_10b8&mi_00&col01";

// ---- Global Device State ----
// Event channel
static std::unique_ptr<Himax::Pen::IPenUsbTransport> g_evtTransport;
static std::thread g_evtReadThread;
static std::atomic<bool> g_evtKeepReading{false};
// Pressure channel
static std::unique_ptr<Himax::Pen::IPenUsbTransport> g_pressTransport;
static std::thread g_pressReadThread;
static std::atomic<bool> g_pressKeepReading{false};

static bool ContainsCaseInsensitive(const std::wstring& haystack, const wchar_t* needle) {
    std::wstring lower = haystack;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    std::wstring needleLower = needle;
    std::transform(needleLower.begin(), needleLower.end(), needleLower.begin(), ::towlower);
    return lower.find(needleLower) != std::wstring::npos;
}

// Find event channel device via dedicated GUID
std::optional<std::wstring> FindEventDevicePath() {
    HDEVINFO devInfo = SetupDiGetClassDevsW(&EVENT_DEVICE_GUID,
                                            nullptr, nullptr,
                                            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return std::nullopt;

    std::optional<std::wstring> result;
    for (DWORD i = 0;; ++i) {
        SP_DEVICE_INTERFACE_DATA ifData{};
        ifData.cbSize = sizeof(ifData);
        if (!SetupDiEnumDeviceInterfaces(devInfo, nullptr, &EVENT_DEVICE_GUID, i, &ifData)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) break;
            continue;
        }
        DWORD reqSize = 0;
        SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, nullptr, 0, &reqSize, nullptr);
        if (reqSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) continue;
        std::vector<uint8_t> buf(reqSize, 0);
        auto* det = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
        det->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, det, reqSize, nullptr, nullptr)) {
            result = det->DevicePath;
            break;
        }
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return result;
}

// Find pressure channel device via HID enumeration + path matching
std::optional<std::wstring> FindPressureDevicePath() {
    GUID hidGuid{};
    HidD_GetHidGuid(&hidGuid);
    HDEVINFO devInfo = SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr,
                                            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return std::nullopt;

    std::optional<std::wstring> result;
    for (DWORD i = 0;; ++i) {
        SP_DEVICE_INTERFACE_DATA ifData{};
        ifData.cbSize = sizeof(ifData);
        if (!SetupDiEnumDeviceInterfaces(devInfo, nullptr, &hidGuid, i, &ifData)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) break;
            continue;
        }
        DWORD reqSize = 0;
        SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, nullptr, 0, &reqSize, nullptr);
        if (reqSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) continue;
        std::vector<uint8_t> buf(reqSize, 0);
        auto* det = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
        det->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, det, reqSize, nullptr, nullptr)) continue;
        std::wstring path = det->DevicePath;
        if (ContainsCaseInsensitive(path, PRESSURE_HID_MATCH)) {
            result = path;
            break;
        }
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return result;
}

// ---- Auto-ACK & Handshake Responder ----
// The MCU expects an ACK (cmd 0x8001) for every event it sends.
// Additionally, PEN_REP_PARAM (0x7B) carries init config that MUST be
// echoed back via cmd 0x7D01 or the MCU times out and stops pressure data.
static std::atomic<bool> g_autoAckEnabled{true};
static std::atomic<uint32_t> g_ackSentCount{0};
static std::atomic<bool> g_handshakeComplete{false};

static const char* GetEventName(uint8_t code) {
    switch (code) {
    case 0x70: return "PEN_AC_STATUS";
    case 0x71: return "PEN_CONN_STATUS";
    case 0x72: return "PEN_CUR_STATUS";
    case 0x73: return "PEN_TYPE_INFO";
    case 0x74: return "PEN_ROATE_ANGLE";
    case 0x75: return "PEN_TOUCH_MODE";
    case 0x76: return "PEN_GLOBAL_PREVENT";
    case 0x78: return "PEN_HOLSTER";
    case 0x79: return "PEN_FREQ_JUMP";
    case 0x7B: return "PEN_REP_PARAM";
    case 0x7C: return "PEN_GLOBAL_ANNOTATION";
    case 0x7F: return "ERASER_TOGGLE";
    case 0x2F: return "PEN_CURRENT_FUNC";
    default:   return nullptr;
    }
}

static int GetAckCodeForEvent(uint8_t eventCode) {
    switch (eventCode) {
    case 0x70: return 0;   case 0x71: return 1;   case 0x72: return 2;
    case 0x73: return 0xD; case 0x74: return 3;   case 0x75: return 4;
    case 0x76: return 5;   case 0x77: return 6;   case 0x78: return 7;
    case 0x79: return 8;   case 0x7B: return 0xA;  case 0x7C: return 0xC;
    case 0x7F: return 9;   case 0x2F: return 0xB;
    default:   return -1;
    }
}

static void SendRawEvtPacket(const std::vector<uint8_t>& pkt, const char* label) {
    if (!g_evtTransport || !g_evtTransport->IsOpen()) return;
    auto res = g_evtTransport->WritePacket(pkt);
    if (res.has_value()) {
        AddLog("Evt", label, pkt);
    }
}

static void SendAckDirect(uint8_t ackCode) {
    std::vector<uint8_t> pkt = {
        0x07, 0x01, 0x02, 0x00,
        0x01, 0x80, 0x11, 0x20, ackCode
    };
    SendRawEvtPacket(pkt, "ACK");
    g_ackSentCount.fetch_add(1);
}

// Send 0x7D01 with echoed config payload (up to 32 bytes)
static void SendInitParamEcho(const uint8_t* data, size_t len) {
    size_t payloadLen = std::min(len, size_t(32));
    std::vector<uint8_t> pkt(8 + payloadLen, 0);
    pkt[0] = 0x07; pkt[1] = 0x01; pkt[2] = 0x02; pkt[3] = 0x00;
    pkt[4] = 0x01; pkt[5] = 0x7D;   // cmd = 0x7D01 LE
    pkt[6] = 0x11; pkt[7] = 0x20;   // tag + flag
    std::copy(data, data + payloadLen, pkt.begin() + 8);
    SendRawEvtPacket(pkt, "InitEcho");
}

// Auto-handshake: send 0x7101 (Query Pen) + 0x7701 (Query MCU)
static void RunHandshake() {
    if (!g_evtTransport || !g_evtTransport->IsOpen()) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // 0x7101 Query Pen Status
    std::vector<uint8_t> q1 = {0x07,0x00,0x02,0x00, 0x01,0x71, 0x11,0x00};
    SendRawEvtPacket(q1, "Handshake");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    // 0x7701 Query MCU Status
    std::vector<uint8_t> q2 = {0x07,0x00,0x02,0x00, 0x01,0x77, 0x11,0x00};
    SendRawEvtPacket(q2, "Handshake");
    g_handshakeComplete.store(true);
}

// ---- Read Threads ----
void EventReadThread() {
    while (g_evtKeepReading.load()) {
        if (!g_evtTransport || !g_evtTransport->IsOpen()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        std::vector<uint8_t> rxBuf;
        auto res = g_evtTransport->ReadPacket(rxBuf, 1000);
        if (res.has_value() && !rxBuf.empty()) {
            // Decode event name for log
            std::string label = "Rx";
            if (rxBuf.size() >= 7) {
                const char* name = GetEventName(rxBuf[5]);
                if (name) label = std::string("Rx ") + name;
            }
            AddLog("Evt", label.c_str(), rxBuf);

            if (g_autoAckEnabled.load() && rxBuf.size() >= 7) {
                uint8_t eventCode = rxBuf[5];
                // 1. Send ACK for this event
                int ackCode = GetAckCodeForEvent(eventCode);
                if (ackCode >= 0) {
                    SendAckDirect(static_cast<uint8_t>(ackCode));
                }
                // 2. If PEN_REP_PARAM (0x7B), echo config via 0x7D01
                if (eventCode == 0x7B && rxBuf.size() > 8) {
                    SendInitParamEcho(rxBuf.data() + 8,
                                     rxBuf.size() - 8);
                }
            }
        }
    }
}

void PressureReadThread() {
    while (g_pressKeepReading.load()) {
        if (!g_pressTransport || !g_pressTransport->IsOpen()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        std::vector<uint8_t> rxBuf;
        auto res = g_pressTransport->ReadPacket(rxBuf, 1000);
        if (res.has_value() && !rxBuf.empty()) {
            AddLog("Press", "Rx", rxBuf);
            // Decode pressure report
            if (rxBuf.size() >= 11 && rxBuf[0] == 0x55) {
                g_pressReportType.store(rxBuf[0]);
                g_pressFreq1.store(rxBuf[1]);
                g_pressFreq2.store(rxBuf[2]);
                for (int k = 0; k < 4; ++k) {
                    uint16_t p = static_cast<uint16_t>(rxBuf[3 + k*2]) |
                                (static_cast<uint16_t>(rxBuf[4 + k*2]) << 8);
                    g_pressLive[k].store(p);
                }
            }
        }
    }
}

void StartThreads() {
    g_evtKeepReading = true;
    g_evtReadThread = std::thread(EventReadThread);
    g_pressKeepReading = true;
    g_pressReadThread = std::thread(PressureReadThread);
}

void StopThreads() {
    g_evtKeepReading = false;
    g_pressKeepReading = false;
    if (g_evtReadThread.joinable()) g_evtReadThread.join();
    if (g_pressReadThread.joinable()) g_pressReadThread.join();
}

// UI Helpers
void DrawHexDump(const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        if (i == 4 || i == 5) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%02X", data[i]); // highlight cmd_id
        } else if (i == 6) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%02X", data[i]); // highlight tag
        } else {
            ImGui::Text("%02X", data[i]);
        }
        if (i < data.size() - 1) ImGui::SameLine();
    }
}

void SendPacket(uint16_t cmdId, const std::vector<uint8_t>& payload) {
    if (!g_evtTransport || !g_evtTransport->IsOpen()) return;

    std::vector<uint8_t> txData(8 + payload.size(), 0);
    txData[0] = 0x07;
    txData[1] = 0x01;
    txData[2] = 0x02;
    txData[3] = 0x00;
    txData[4] = cmdId & 0xFF;
    txData[5] = (cmdId >> 8) & 0xFF;
    txData[6] = 0x11;
    txData[7] = 0x00;

    if (cmdId == 0x7101) txData[1] = 0x00;
    if (cmdId == 0x7701) txData[1] = 0x00;
    if (cmdId == 0x7D01) { txData[1] = 0x01; txData[7] = 0x20; }
    if (cmdId == 0x8001) { txData[1] = 0x01; txData[7] = 0x20; }
    
    std::copy(payload.begin(), payload.end(), txData.begin() + 8);

    auto res = g_evtTransport->WritePacket(txData);
    if (res.has_value()) {
        AddLog("Evt", "Tx", txData);
    } else {
        AddLog("Evt", "Tx ERR", txData);
    }
}

int main(int, char**)
{
    // AllocConsole();
    // FILE* dummy;
    // freopen_s(&dummy, "CONOUT$", "w", stdout);
    // freopen_s(&dummy, "CONOUT$", "w", stderr);

    Common::Logger::Init("BtMcuTestTool");
    LOG_INFO("Tool", __func__, "System", "--- BtMcuTestTool Starts ---");

    g_evtTransport = Himax::Pen::CreatePenUsbTransportWin32();
    g_pressTransport = Himax::Pen::CreatePenUsbTransportWin32();
    StartThreads();

    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"BtMcuTestToolApp", nullptr };
    ::RegisterClassExW(&wc);
    
    // Create application window (Hide Main Window: Create as WS_POPUP and WS_EX_TOOLWINDOW off-screen)
    HWND hwnd = ::CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, L"BT MCU Test Tool (Hidden)", WS_POPUP, 
                                  -10000, -10000, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Hide the main window completely so user just sees the floating ImGui windows
    ::ShowWindow(hwnd, SW_HIDE);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
    
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool done = false;
    char customCmdBuf[16] = "7101";
    char customPayloadBuf[128] = "";
    char ackBuf[16] = "00";

    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Main UI
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        ImGui::Begin("BT MCU Pen USB Protocol Test Tool");

        // 1. Dual-Channel Connection Status
        ImGui::Text("Event Channel:"); ImGui::SameLine();
        if (g_evtTransport && g_evtTransport->IsOpen()) {
            ImGui::TextColored(ImVec4(0,1,0,1), "CONNECTED");
        } else {
            ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "DISCONNECTED");
        }
        ImGui::SameLine(); ImGui::Text("  |  Pressure Channel:"); ImGui::SameLine();
        if (g_pressTransport && g_pressTransport->IsOpen()) {
            ImGui::TextColored(ImVec4(0,1,0,1), "CONNECTED");
        } else {
            ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "DISCONNECTED");
        }

        if (ImGui::Button("Connect Both")) {
            // Event channel
            auto evtPath = FindEventDevicePath();
            if (evtPath.has_value()) {
                auto res = g_evtTransport->Open(evtPath.value());
                if (res.has_value()) {
                    LOG_INFO("Tool", __func__, "Evt", "Event channel connected.");
                } else {
                    LOG_ERROR("Tool", __func__, "Evt", "Failed to open event channel.");
                }
            } else {
                LOG_ERROR("Tool", __func__, "Evt", "Event device not found.");
            }
            // Pressure channel
            auto pressPath = FindPressureDevicePath();
            if (pressPath.has_value()) {
                auto res = g_pressTransport->Open(pressPath.value());
                if (res.has_value()) {
                    LOG_INFO("Tool", __func__, "Press", "Pressure channel connected.");
                } else {
                    LOG_ERROR("Tool", __func__, "Press", "Failed to open pressure channel.");
                }
            } else {
                LOG_ERROR("Tool", __func__, "Press", "Pressure HID device not found.");
            }

            // Auto-handshake in background thread
            g_handshakeComplete.store(false);
            std::thread(RunHandshake).detach();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close Both")) {
            if (g_evtTransport) g_evtTransport->Close();
            if (g_pressTransport) g_pressTransport->Close();
            g_handshakeComplete.store(false);
        }
        ImGui::SameLine();
        if (g_handshakeComplete.load()) {
            ImGui::TextColored(ImVec4(0,1,0,1), "Handshake OK");
        } else if (g_evtTransport && g_evtTransport->IsOpen()) {
            ImGui::TextColored(ImVec4(1,1,0,1), "Handshaking...");
        }

        ImGui::Separator();

        // 2. Quick Commands
        ImGui::Text("Quick Commands");
        if (ImGui::Button("Query Status (0x7101)")) { SendPacket(0x7101, {}); }
        ImGui::SameLine();
        if (ImGui::Button("Query Info (0x7701)")) { SendPacket(0x7701, {}); }
        ImGui::SameLine();
        if (ImGui::Button("Re-Handshake")) {
            g_handshakeComplete.store(false);
            std::thread(RunHandshake).detach();
        }
        ImGui::SameLine();
        if (ImGui::Button("Match Info Reply (0x7E01)")) { SendPacket(0x7E01, {0x01}); }

        ImGui::Separator();

        // 3. Custom Commands
        ImGui::Text("Custom Command");
        ImGui::InputText("Command ID (Hex)", customCmdBuf, sizeof(customCmdBuf), ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::InputText("Payload (Hex Space Separated)", customPayloadBuf, sizeof(customPayloadBuf));
        if (ImGui::Button("Send Custom")) {
            uint32_t cmdId = 0;
            sscanf_s(customCmdBuf, "%x", &cmdId);
            
            std::vector<uint8_t> payload;
            char* next_token = nullptr;
            char* token = strtok_s(customPayloadBuf, " ", &next_token);
            while (token != nullptr) {
                uint32_t val = 0;
                if (sscanf_s(token, "%x", &val) == 1) {
                    payload.push_back(static_cast<uint8_t>(val));
                }
                token = strtok_s(nullptr, " ", &next_token);
            }
            SendPacket(static_cast<uint16_t>(cmdId), payload);
        }

        ImGui::Separator();

        // 4. ACK Responder
        {
            bool autoAck = g_autoAckEnabled.load();
            if (ImGui::Checkbox("Auto-ACK", &autoAck)) {
                g_autoAckEnabled.store(autoAck);
            }
            ImGui::SameLine();
            ImGui::TextColored(
                autoAck ? ImVec4(0,1,0,1) : ImVec4(0.6f,0.6f,0.6f,1),
                autoAck ? "ON (ACKs sent: %u)" : "OFF",
                g_ackSentCount.load());
        }
        ImGui::Text("Manual ACK (0x8001):");
        ImGui::InputText("ACK Code (Hex)", ackBuf, sizeof(ackBuf), ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::SameLine();
        if (ImGui::Button("Send ACK")) {
            uint32_t ackVal = 0;
            sscanf_s(ackBuf, "%x", &ackVal);
            SendPacket(0x8001, {static_cast<uint8_t>(ackVal)});
        }

        ImGui::Separator();

        // 5. Live Pressure Display
        ImGui::Text("Live Pressure Data");
        uint8_t rt = g_pressReportType.load();
        ImGui::Text("Report Type: 0x%02X (%c)", rt, rt >= 0x20 ? rt : '?');
        ImGui::Text("BT Freq: %d / %d", g_pressFreq1.load(), g_pressFreq2.load());
        ImGui::Text("Pressure: [%5d] [%5d] [%5d] [%5d]",
                    g_pressLive[0].load(), g_pressLive[1].load(),
                    g_pressLive[2].load(), g_pressLive[3].load());

        ImGui::Separator();

        // 6. Logs
        ImGui::Text("Packet Log (%zu / %zu)", g_logs.size(), MAX_LOG_COUNT);
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            std::lock_guard<std::mutex> lock(g_logMutex);
            g_logs.clear();
        }
        ImGui::SameLine();
        ImGui::Checkbox("File Log", &g_fileLoggingEnabled);
        ImGui::SameLine();
        if (g_logFile.is_open()) {
            ImGui::TextColored(ImVec4(0.5f,1,0.5f,1), "[%s]", g_logFileName.c_str());
        } else if (g_fileLoggingEnabled) {
            ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1), "(will open on first log)");
        }

        ImGui::BeginChild("LogRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        {
            std::lock_guard<std::mutex> lock(g_logMutex);
            for (const auto& log : g_logs) {
                // Color-code by channel
                ImVec4 chColor = (log.channel == "Evt") ? ImVec4(0.4f,0.8f,1.0f,1.0f)
                                                        : ImVec4(1.0f,0.8f,0.3f,1.0f);
                ImGui::TextColored(chColor, "[%s][%s] %s: ",
                    log.time.c_str(), log.channel.c_str(), log.direction.c_str());
                ImGui::SameLine();
                DrawHexDump(log.data);
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        ImGui::End();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    StopThreads();
    CloseLogFile();
    if (g_evtTransport) { g_evtTransport->Close(); g_evtTransport.reset(); }
    if (g_pressTransport) { g_pressTransport->Close(); g_pressTransport.reset(); }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    Common::Logger::Shutdown();
    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
