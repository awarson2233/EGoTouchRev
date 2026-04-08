#include "penevt/PenEventBridge.h"
#include "Logger.h"

#include <Windows.h>
#include <SetupAPI.h>
#include <hidsdi.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace Himax::Pen {

// ── 静态常量 ──────────────────────────────────────────────────────────────
const GUID PenEventBridge::kEventDeviceGuid =
    {0xdd0ebedb, 0xf1d6, 0x4cfa, {0xac, 0xca, 0x71, 0xe6, 0x6d, 0x31, 0x78, 0xca}};

// ── 回调设置 ───────────────────────────────────────────────────────────────
void PenEventBridge::SetEventCallback(PenEventCallback cb) {
    std::lock_guard<std::mutex> lk(m_cbMutex);
    m_eventCallback = std::move(cb);
}

// ── 设备路径发现 ───────────────────────────────────────────────────────────
std::optional<std::wstring> PenEventBridge::FindDevicePath() {
    HDEVINFO devInfo = SetupDiGetClassDevsW(
        &kEventDeviceGuid, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return std::nullopt;

    std::optional<std::wstring> result;
    for (DWORD i = 0; ; ++i) {
        SP_DEVICE_INTERFACE_DATA ifData{};
        ifData.cbSize = sizeof(ifData);
        if (!SetupDiEnumDeviceInterfaces(devInfo, nullptr,
                                         &kEventDeviceGuid, i, &ifData)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS) break;
            continue;
        }
        DWORD reqSize = 0;
        SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, nullptr, 0, &reqSize, nullptr);
        if (reqSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) continue;
        std::vector<uint8_t> buf(reqSize, 0);
        auto* det = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
        det->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, det, reqSize,
                                             nullptr, nullptr)) {
            result = det->DevicePath;
            break;
        }
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return result;
}

// ── SetScanMode — BT 笔切频命令 ───────────────────────────────────────────
//
// 原厂调用链:
//   ApDaemon::SetScanMode → 构造 IPC {type=1, code=3, "freq1,freq2,mode"}
//     → THP_Service::BtPen_HandleInitParamEvent → 解码 ASCII → binary
//       → BtPen_SendPacket(header, binary_payload, 0x20)
//
// 我们直接操作 col00 USB 设备，跳过 THP_Service 的解码层，
// 因此必须自行实现 HandleInitParamEvent 的 type-3 编码。
//
// Header (汇编验证: THP_Service.dll @ 0x18000fe0a-0x18000fe1d):
//   MOV byte ptr [RSP+0x38], 0x07      → byte[0] = 0x07
//   MOV word ptr [RSP+0x39], 0x0201    → byte[1] = 0x01, byte[2] = 0x02
//   MOV word ptr [RSP+0x3c], 0x7D01    → byte[4] = 0x01, byte[5] = 0x7D
//   MOV byte ptr [RSP+0x3f], 0x20      → byte[7] = 0x20
//   (byte[6] 由 BtPen_SendPacket 强制覆写为 0x11)
//
// Payload: 32-byte binary，由 type-3 解码器从 ASCII 十进制字符串转换而来

bool PenEventBridge::SendScanMode(uint8_t freq1, uint8_t freq2, uint8_t mode) {
    if (!IsTransportOpen()) {
        LOG_WARN("PenEvent", __func__, "MCU", "Transport not open, cannot send SetScanMode.");
        return false;
    }

    // ── Type-3 编码 (完整版, 支持 len=1/2/3/4) ───────────────────────────
    // HandleInitParamEvent 对每个 comma-separated token 的解码逻辑:
    //   len=1: 1 byte  → strtol("0x" + [0])
    //   len=2: 2 bytes → strtol("0x" + [1]),       strtol("0x" + [0])
    //   len=3: 2 bytes → strtol("0x" + [1] + [2]), strtol("0x" + [0])
    //   len=4: 2 bytes → strtol("0x" + [2] + [3]), strtol("0x" + [0] + [1])
    auto encodeType3Token = [](const char* hex_str, uint8_t* out) -> int {
        int n = static_cast<int>(strlen(hex_str));

        if (n <= 1) {
            char h[4] = {'0', 'x', hex_str[0], '\0'};
            out[0] = static_cast<uint8_t>(strtol(h, nullptr, 0));
            return 1;
        }
        if (n == 2) {
            // high byte: "0x" + [1]
            char ha[4] = {'0', 'x', hex_str[1], '\0'};
            out[0] = static_cast<uint8_t>(strtol(ha, nullptr, 0));
            // low byte: "0x" + [0]
            char hb[4] = {'0', 'x', hex_str[0], '\0'};
            out[1] = static_cast<uint8_t>(strtol(hb, nullptr, 0));
            return 2;
        }
        if (n == 3) {
            // high byte: "0x" + [1][2]
            char ha[5] = {'0', 'x', hex_str[1], hex_str[2], '\0'};
            out[0] = static_cast<uint8_t>(strtol(ha, nullptr, 0));
            // low byte: "0x" + [0]
            char hb[4] = {'0', 'x', hex_str[0], '\0'};
            out[1] = static_cast<uint8_t>(strtol(hb, nullptr, 0));
            return 2;
        }
        // len=4
        // high byte: "0x" + [2][3]
        char ha[5] = {'0', 'x', hex_str[2], hex_str[3], '\0'};
        out[0] = static_cast<uint8_t>(strtol(ha, nullptr, 0));
        // low byte: "0x" + [0][1]
        char hb[5] = {'0', 'x', hex_str[0], hex_str[1], '\0'};
        out[1] = static_cast<uint8_t>(strtol(hb, nullptr, 0));
        return 2;
    };

    // 将 uint8_t freq 值转为十进制字符串再编码
    char s_freq1[8], s_freq2[8], s_mode[8];
    snprintf(s_freq1, sizeof(s_freq1), "%u", freq1);
    snprintf(s_freq2, sizeof(s_freq2), "%u", freq2);
    snprintf(s_mode,  sizeof(s_mode),  "%u", mode ? 3u : 0u);

    uint8_t payload[0x20] = {};  // 32-byte binary payload (zero-init)
    int off = 0;
    off += encodeType3Token(s_freq1, payload + off);
    off += encodeType3Token(s_freq2, payload + off);
    off += encodeType3Token(s_mode,  payload + off);
    (void)off;  // 实际总字节数

    // ── 构建 USB 报文: 8-byte header + 32-byte binary payload ────────────
    std::vector<uint8_t> pkt(8 + 0x20, 0);
    pkt[0] = 0x07;   // report type
    pkt[1] = 0x01;   // fixed (payload-present flag)
    pkt[2] = 0x02;   // sub-group
    pkt[3] = 0x00;
    pkt[4] = 0x01;   // cmd_id low  (0x7D01)
    pkt[5] = 0x7D;   // cmd_id high (0x7D01)
    pkt[6] = 0x11;   // MCU protocol marker
    pkt[7] = 0x20;   // payload tag (0x20 for payload-present commands)
    std::copy(payload, payload + 0x20, pkt.begin() + 8);

    SendRawPacket(pkt);
    LOG_INFO("PenEvent", __func__, "MCU",
             "SetScanMode sent: freq1=0x{:02X}, freq2=0x{:02X}, mode={}"
             " → payload=[{:02X} {:02X} {:02X} {:02X} {:02X} {:02X}]",
             freq1, freq2, mode,
             payload[0], payload[1], payload[2],
             payload[3], payload[4], payload[5]);
    return true;
}

// ── 协议辅助 ───────────────────────────────────────────────────────────────
int PenEventBridge::GetAckCode(uint8_t eventCode) {
    switch (eventCode) {
    case 0x6F: return 11;   case 0x70: return 0;    case 0x71: return 1;
    case 0x72: return 2;    case 0x73: return 0xD;  case 0x74: return 3;
    case 0x75: return 4;    case 0x76: return 5;    case 0x77: return 6;
    case 0x78: return 7;    case 0x79: return 8;    case 0x7B: return 0xA;
    case 0x7C: return 0xC;  case 0x7F: return 9;    case 0x2F: return 0xB;
    default:   return -1;
    }
}

void PenEventBridge::SendRawPacket(const std::vector<uint8_t>& pkt) {
    if (!IsTransportOpen()) return;
    (void)GetTransport()->WritePacket(pkt);
}

void PenEventBridge::SendAck(uint8_t ackCode) {
    // Header 验证: THP_Service BtPen_SendEventAck @ 0x180011d9d-0x180011dc3
    //   MOV byte [RSP+0x20], 0x07    MOV word [RSP+0x21], 0x0201
    //   MOV word [RSP+0x24], 0x8001  MOV byte [RSP+0x27], 0x20
    const std::vector<uint8_t> pkt = {
        0x07, 0x01, 0x02, 0x00,
        0x01, 0x80, 0x11, 0x20, ackCode
    };
    SendRawPacket(pkt);
    LOG_INFO("PenEvent", __func__, "MCU", "ACK sent: 0x{:02X}", ackCode);
}

void PenEventBridge::SendInitParamEcho(const uint8_t* data, size_t len) {
    // Header 与 HandleInitParamEvent 相同: cmd_id=0x7D01
    size_t payloadLen = std::min(len, size_t(0x20));
    std::vector<uint8_t> pkt(8 + payloadLen, 0);
    pkt[0] = 0x07; pkt[1] = 0x01; pkt[2] = 0x02; pkt[3] = 0x00;
    pkt[4] = 0x01; pkt[5] = 0x7D;
    pkt[6] = 0x11; pkt[7] = 0x20;
    std::copy(data, data + payloadLen, pkt.begin() + 8);
    SendRawPacket(pkt);
    LOG_INFO("PenEvent", __func__, "MCU", "0x7D01 InitParam echo sent ({} bytes payload).", payloadLen);
}

// ── BtHidChannel hooks ────────────────────────────────────────────────────
void PenEventBridge::OnConnected() {
    RunHandshake();  // 同步执行握手，避免 detach 的生命周期风险
}

void PenEventBridge::OnPacketReceived(const std::vector<uint8_t>& packet) {
    if (packet.size() < 8) return;

    // ── 诊断: 完整报文 hex dump (前 16 字节) ──
    {
        std::string hexDump;
        size_t dumpLen = std::min(packet.size(), size_t(16));
        for (size_t i = 0; i < dumpLen; ++i) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%02X ", packet[i]);
            hexDump += buf;
        }
        LOG_INFO("PenEvent", __func__, "MCU",
                 "RX [{}B]: {}", packet.size(), hexDump);
    }

    const uint8_t eventCode = packet[5];

    // 1. 自动 ACK
    int ackCode = GetAckCode(eventCode);
    if (ackCode >= 0) {
        SendAck(static_cast<uint8_t>(ackCode));
    }

    // 2. PEN_REP_PARAM (0x7B) → 额外发送 0x7D01 InitParam 回显
    if (eventCode == 0x7B && packet.size() > 8) {
        SendInitParamEcho(packet.data() + 8, packet.size() - 8);
    }

    // 3. NewPenConnectRequest (0x15) → 发送 0x2E01 确认配对握手
    // ⚠ 未验证: 0x15 事件和 0x2E01 命令在 THP_Service.dll 中无代码引用。
    //   此行为基于早期抓包推测。如果 MCU 不识别此命令则无效但无害。
    if (eventCode == 0x15) {
        const std::vector<uint8_t> pkt2e01 = {
            0x07, 0x00, 0x02, 0x00,
            0x01, 0x2E, 0x11, 0x00
        };
        SendRawPacket(pkt2e01);
        LOG_WARN("PenEvent", __func__, "MCU",
                 "Sent 0x2E01 pairing confirmation (UNVERIFIED command).");
    }

    // 4. 触发上层回调
    PenEventCallback cbCopy;
    {
        std::lock_guard<std::mutex> lk(m_cbMutex);
        cbCopy = m_eventCallback;
    }
    if (cbCopy) {
        PenEvent ev;
        ev.code     = static_cast<PenUsbEventCode>(eventCode);
        ev.payload  = std::vector<uint8_t>(packet.begin() + 8, packet.end());
        ev.receivedAt = std::chrono::steady_clock::now();
        cbCopy(ev);
    }

    if (m_notifyEvent) {
        SetEvent(m_notifyEvent);
    }
}

// ── 握手 ──────────────────────────────────────────────────────────────────
void PenEventBridge::RunHandshake() {
    if (!IsTransportOpen()) return;
    LOG_INFO("PenEvent", __func__, "MCU", "Sending 0x7101 + 0x7701 handshake.");

    const std::vector<uint8_t> q1 = {0x07,0x00,0x02,0x00, 0x01,0x71, 0x11,0x00};
    SendRawPacket(q1);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    const std::vector<uint8_t> q2 = {0x07,0x00,0x02,0x00, 0x01,0x77, 0x11,0x00};
    SendRawPacket(q2);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // ── 发送初始协议参数 (原厂 ReportBluetoothPenInfo → 0x7D01) ──
    SendInitProtocolParams();

    LOG_INFO("PenEvent", __func__, "MCU", "Handshake + InitProtocol complete.");
}

// ── 初始协议参数 ──────────────────────────────────────────────────────────
// 原厂 ApDaemon::GetProtocolPrmtMode1/2 → GetProtocolInfo → ReportBluetoothPenInfo
// 输出: "3333,3333,2e7,412,258,411a,0f,1,"
// 这些参数通过 BtPen_GetReportInfo(event_class=2) → HandleInitParamEvent
// 被编码为 0x7D01 二进制包发送给 MCU。
//
// 我们这里直接用 Type-3 编码器处理这些 hex token。
void PenEventBridge::SendInitProtocolParams() {
    if (!IsTransportOpen()) return;

    // Factory CSV: "3333,3333,2e7,412,258,411a,0f,1,"
    const char* tokens[] = {
        "3333", "3333", "2e7", "412", "258", "411a", "0f", "1"
    };

    // 使用与 SendScanMode 相同的 Type-3 编码器
    auto encodeType3Token = [](const char* hex_str, uint8_t* out) -> int {
        int n = static_cast<int>(strlen(hex_str));
        if (n <= 1) {
            char h[4] = {'0', 'x', hex_str[0], '\0'};
            out[0] = static_cast<uint8_t>(strtol(h, nullptr, 0));
            return 1;
        }
        if (n == 2) {
            char ha[4] = {'0', 'x', hex_str[1], '\0'};
            out[0] = static_cast<uint8_t>(strtol(ha, nullptr, 0));
            char hb[4] = {'0', 'x', hex_str[0], '\0'};
            out[1] = static_cast<uint8_t>(strtol(hb, nullptr, 0));
            return 2;
        }
        if (n == 3) {
            char ha[5] = {'0', 'x', hex_str[1], hex_str[2], '\0'};
            out[0] = static_cast<uint8_t>(strtol(ha, nullptr, 0));
            char hb[4] = {'0', 'x', hex_str[0], '\0'};
            out[1] = static_cast<uint8_t>(strtol(hb, nullptr, 0));
            return 2;
        }
        // len=4
        char ha[5] = {'0', 'x', hex_str[2], hex_str[3], '\0'};
        out[0] = static_cast<uint8_t>(strtol(ha, nullptr, 0));
        char hb[5] = {'0', 'x', hex_str[0], hex_str[1], '\0'};
        out[1] = static_cast<uint8_t>(strtol(hb, nullptr, 0));
        return 2;
    };

    uint8_t payload[0x20] = {};
    int off = 0;
    for (const char* tok : tokens) {
        off += encodeType3Token(tok, payload + off);
    }

    // 构建 0x7D01 InitParam 报文
    std::vector<uint8_t> pkt(8 + 0x20, 0);
    pkt[0] = 0x07; pkt[1] = 0x01; pkt[2] = 0x02; pkt[3] = 0x00;
    pkt[4] = 0x01; pkt[5] = 0x7D;
    pkt[6] = 0x11; pkt[7] = 0x20;
    std::copy(payload, payload + 0x20, pkt.begin() + 8);

    SendRawPacket(pkt);

    // 日志: dump 前 16 字节 payload
    std::string hexDump;
    for (int i = 0; i < off && i < 16; ++i) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%02X ", payload[i]);
        hexDump += buf;
    }
    LOG_INFO("PenEvent", __func__, "MCU",
             "0x7D01 InitProtocolParams sent ({} bytes payload): {}", off, hexDump);
}

} // namespace Himax::Pen
