#include "penevt/PenEventBridge.h"
#include "btmcu/PenUsbPacketBuilder.h"
#include "Logger.h"

#include <Windows.h>
#include <SetupAPI.h>
#include <hidsdi.h>

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace Himax::Pen {

namespace {

const GUID kEventDeviceGuid =
    {0xdd0ebedb, 0xf1d6, 0x4cfa, {0xac, 0xca, 0x71, 0xe6, 0x6d, 0x31, 0x78, 0xca}};

} // namespace

PenEventBridge::~PenEventBridge() {
    Stop();
}

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
        if (!SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, det, reqSize,
                                              nullptr, nullptr)) {
            continue;
        }

        HANDLE probe = CreateFileW(det->DevicePath,
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   nullptr,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   nullptr);
        if (probe == INVALID_HANDLE_VALUE) {
            LOG_WARN("PenEvent", __func__, "MCU",
                     "Skipping event device path that failed probe open: error={}",
                     GetLastError());
            continue;
        }

        CloseHandle(probe);
        result = det->DevicePath;
        break;
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

    const auto payload = BuildScanModePayload(freq1, freq2, mode);
    const auto pkt = BuildPenUsbPayloadCommand(
        PenUsbCommandId::InitParamSet,
        std::span<const uint8_t>(payload.data(), payload.size()));

    if (!SendRawPacket(pkt)) {
        LOG_WARN("PenEvent", __func__, "MCU", "SetScanMode send failed.");
        return false;
    }

    LOG_INFO("PenEvent", __func__, "MCU",
             "SetScanMode sent: freq1=0x{:02X}, freq2=0x{:02X}, mode={} payloadLen={}",
             freq1, freq2, mode, payload.size());
    return true;
}

// ── 协议辅助 ───────────────────────────────────────────────────────────────
int PenEventBridge::GetAckCode(uint8_t eventCode) {
    return GetFactoryBtMcuAckCode(eventCode);
}

bool PenEventBridge::SendRawPacket(const std::vector<uint8_t>& pkt) {
    std::lock_guard<std::mutex> txLock(m_txMutex);
    if (!IsTransportOpen()) {
        LOG_WARN("PenEvent", __func__, "MCU", "Transport not open; dropping packet send.");
        return false;
    }

    auto result = GetTransport()->WritePacket(pkt);
    if (!result) {
        LOG_WARN("PenEvent", __func__, "MCU",
                 "WritePacket failed while sending {} bytes (error={}).",
                 pkt.size(), static_cast<int>(result.error()));
        return false;
    }

    return true;
}

void PenEventBridge::SendAck(uint8_t eventCode, uint8_t ackCode) {
    const auto pkt = BuildPenUsbEventAck(ackCode);
    if (SendRawPacket(pkt)) {
        LOG_INFO("PenEvent", __func__, "MCU",
                 "ACK sent: event={}(0x{:02X}) ack=0x{:02X}",
                 PenUsbEventNameFromRaw(eventCode), eventCode, ackCode);
    }
}

void PenEventBridge::ExecuteInitAction(PenUsbInitAction action) {
    switch (action) {
    case PenUsbInitAction::None:
        return;
    case PenUsbInitAction::SendInitialQueries:
        (void)SendQueryPenStatus();
        (void)SendFirstMcuStatusQuery();
        return;
    case PenUsbInitAction::SendSecondMcuStatusQuery:
        (void)SendSecondMcuStatusQuery();
        return;
    case PenUsbInitAction::SendInitProtocolParams:
        (void)SendInitProtocolParams();
        return;
    }
}

bool PenEventBridge::SendQueryHardwareVersion() {
    const auto query = BuildPenUsbFixedSizeCommand(PenUsbCommandId::QueryHardwareVersion);
    if (!SendRawPacket(query)) {
        LOG_WARN("PenEvent", __func__, "MCU", "Failed to send 0x0201 QueryHardwareVersion.");
        return false;
    }

    LOG_INFO("PenEvent", __func__, "MCU", "Sent 0x0201 QueryHardwareVersion.");
    return true;
}

bool PenEventBridge::SendQueryPenStatus() {
    const auto query = BuildPenUsbCommand(PenUsbCommandId::QueryPenStatus);
    if (!SendRawPacket(query)) {
        LOG_WARN("PenEvent", __func__, "MCU", "Failed to send 0x7101 CheckPenStatus.");
        return false;
    }

    LOG_INFO("PenEvent", __func__, "MCU", "Sent 0x7101 CheckPenStatus.");
    return true;
}

bool PenEventBridge::SendFirstMcuStatusQuery() {
    const auto query = BuildPenUsbCommand(PenUsbCommandId::QueryPenInfo);
    if (!SendRawPacket(query)) {
        LOG_WARN("PenEvent", __func__, "MCU", "Failed to send first 0x7701 CheckMcuStatus.");
        return false;
    }

    LOG_INFO("PenEvent", __func__, "MCU", "Sent 0x7701 CheckMcuStatus (#1/2).");
    return true;
}

bool PenEventBridge::SendSecondMcuStatusQuery() {
    const auto query = BuildPenUsbCommand(PenUsbCommandId::QueryPenInfo);
    if (!SendRawPacket(query)) {
        LOG_WARN("PenEvent", __func__, "MCU", "Failed to send second 0x7701 CheckMcuStatus.");
        return false;
    }

    LOG_INFO("PenEvent", __func__, "MCU", "Sent 0x7701 CheckMcuStatus (#2/2).");
    return true;
}

void PenEventBridge::AdvanceSessionFromEvent(uint8_t eventCode) {
    PenUsbInitAction action = PenUsbInitAction::None;
    {
        std::lock_guard<std::mutex> sessionLock(m_sessionMutex);
        action = m_initSession.OnEvent(static_cast<PenUsbEventCode>(eventCode));
    }

    switch (action) {
    case PenUsbInitAction::SendSecondMcuStatusQuery:
        LOG_INFO("PenEvent", __func__, "MCU",
                 "0x77 PEN_SCREEN_STATUS received after first 0x7701; issuing second 0x7701 query.");
        break;
    case PenUsbInitAction::SendInitProtocolParams:
        LOG_INFO("PenEvent", __func__, "MCU",
                 "0x7B PEN_REP_PARAM received after handshake; sending 0x7D01 InitParam.");
        break;
    default:
        break;
    }

    ExecuteInitAction(action);
}

// ── BtHidChannel hooks ────────────────────────────────────────────────────
void PenEventBridge::OnConnected() {
    RunHandshake();
    (void)SendQueryHardwareVersion();
}

void PenEventBridge::OnPacketReceived(const std::vector<uint8_t>& packet) {
    auto parsed = TryParsePenUsbEventFrame(std::span<const uint8_t>(packet.data(), packet.size()));
    if (!parsed) {
        LOG_WARN("PenEvent", __func__, "MCU",
                 "Dropping invalid RX packet: size={}B.", packet.size());
        return;
    }

    const uint8_t eventCode = parsed->eventCode;
    const uint8_t payload0 = parsed->payload.empty() ? 0 : parsed->payload[0];
    LOG_INFO("PenEvent", __func__, "MCU",
             "RX event: event={}(0x{:02X}) size={}B payloadLen={} payload0=0x{:02X}",
             PenUsbEventNameFromRaw(eventCode), eventCode, packet.size(),
             parsed->payload.size(), payload0);

    int ackCode = GetAckCode(eventCode);
    if (ackCode >= 0) {
        SendAck(eventCode, static_cast<uint8_t>(ackCode));
    }

    AdvanceSessionFromEvent(eventCode);

    PenEventCallback cbCopy;
    {
        std::lock_guard<std::mutex> lk(m_cbMutex);
        cbCopy = m_eventCallback;
    }
    if (cbCopy) {
        PenEvent ev;
        ev.code = static_cast<PenUsbEventCode>(eventCode);
        ev.payload.assign(parsed->payload.begin(), parsed->payload.end());
        ev.receivedAt = std::chrono::steady_clock::now();

        if (!ev.payload.empty()) {
            switch (ev.code) {
            case PenUsbEventCode::PenModule: {
                const uint8_t payloadLength = packet[7];
                auto modelId = TryParsePenModuleModelId(parsed->payload, payloadLength);
                if (!modelId) {
                    LOG_WARN("PenEvent", __func__, "MCU",
                             "PenModule ignored: invalid payloadLen={} available={}.",
                             payloadLength, parsed->payload.size());
                    break;
                }

                const auto modelInfo = ResolvePenModuleModel(*modelId);
                ev.semantic.hasPenModuleModelId = true;
                ev.semantic.penModuleModelId = *modelId;
                ev.semantic.penModuleModel = modelInfo.model;
                ev.semantic.hasPenModuleProtocolHint =
                    modelInfo.protocolHint != PenModuleProtocolHint::Auto;
                ev.semantic.penModuleProtocolHint = modelInfo.protocolHint;

                LOG_INFO("PenEvent", __func__, "MCU",
                         "PenModule: model={} modelId=0x{:06X} protocol={} payloadLen={}",
                         modelInfo.name, modelInfo.modelId,
                         ToString(modelInfo.protocolHint), payloadLength);
                break;
            }
            case PenUsbEventCode::PenHardwareVersion: {
                auto hardwareVersion = DecodePenUsbUtf8Payload(parsed->payload);
                ev.semantic.hasHardwareVersion = !hardwareVersion.empty();
                ev.semantic.hardwareVersion = std::move(hardwareVersion);
                if (ev.semantic.hasHardwareVersion) {
                    LOG_INFO("PenEvent", __func__, "MCU",
                             "PenHardwareVersion: version=\"{}\" payloadLen={}",
                             ev.semantic.hardwareVersion, parsed->payload.size());
                } else {
                    LOG_WARN("PenEvent", __func__, "MCU",
                             "PenHardwareVersion ignored: empty or invalid UTF-8 payloadLen={}",
                             parsed->payload.size());
                }
                break;
            }
            case PenUsbEventCode::PenConnStatus:
                ev.semantic.hasConnection = true;
                ev.semantic.connected = (ev.payload[0] != 0);
                break;
            case PenUsbEventCode::PenTypeInfo:
                ev.semantic.hasStylusId = true;
                ev.semantic.stylusId = ev.payload[0];
                break;
            case PenUsbEventCode::PenCurStatus:
                ev.semantic.hasCurrentMode = true;
                ev.semantic.currentModeRaw = ev.payload[0];
                ev.semantic.currentMode = PenCurrentModeFromRaw(ev.payload[0]);
                break;
            case PenUsbEventCode::EraserToggle:
                ev.semantic.hasEraserToggle = true;
                ev.semantic.eraserToggle = ev.payload[0];
                break;
            case PenUsbEventCode::PenCurrentFunc:
                ev.semantic.hasCurrentFunc = true;
                ev.semantic.currentFunc = ev.payload[0];
                break;
            default:
                break;
            }
        }

        cbCopy(ev);
    }

    if (m_notifyEvent) {
        SetEvent(static_cast<HANDLE>(m_notifyEvent));
    }
}

// ── 握手 ──────────────────────────────────────────────────────────────────
// API Monitor 抓包验证的原厂初始化序列:
//   0x7101 CheckPenStatus
//   0x7701 CheckMcuStatus
//        ← 0x77 PEN_SCREEN_STATUS, ACK 0x06
//   0x7701 CheckMcuStatus (重发)
//        ← 等待 0x7B PEN_REP_PARAM
//   0x7D01 InitProtocolParams (40B = 8 header + 32 payload, USB HID)
void PenEventBridge::RunHandshake() {
    if (!IsRunning() || !IsTransportOpen()) {
        LOG_INFO("PenEvent", __func__, "MCU",
                 "Handshake skipped because channel is not running/open.");
        return;
    }

    LOG_INFO("PenEvent", __func__, "MCU",
             "Starting event-driven init sequence: 0x7101 → 0x7701 → 0x7701, with 0x7D01 deferred until MCU 0x7B.");

    PenUsbInitAction action = PenUsbInitAction::None;
    {
        std::lock_guard<std::mutex> sessionLock(m_sessionMutex);
        action = m_initSession.OnConnected();
    }
    ExecuteInitAction(action);
}

// ── 初始协议参数 ──────────────────────────────────────────────────────────
// 原厂 ApDaemon::GetProtocolPrmtMode1/2 → GetProtocolInfo → ReportBluetoothPenInfo
// 输出: "3333,3333,2e7,412,258,411a,10f,1,"
// 这些参数通过 BtPen_GetReportInfo(event_class=2) → HandleInitParamEvent
// 被编码为 0x7D01 二进制包发送给 MCU。
//
// 我们这里直接用 Type-3 编码器处理这些 hex token。
bool PenEventBridge::SendInitProtocolParams() {
    if (!IsTransportOpen()) {
        LOG_WARN("PenEvent", __func__, "MCU", "Transport not open, cannot send 0x7D01 InitProtocolParams.");
        return false;
    }

    const auto pkt = BuildFactoryInitProtocolParamsCommand();

    if (!SendRawPacket(pkt)) {
        LOG_WARN("PenEvent", __func__, "MCU", "Failed to send 0x7D01 InitProtocolParams.");
        return false;
    }

    LOG_INFO("PenEvent", __func__, "MCU",
             "0x7D01 InitProtocolParams sent (40B exact factory capture).");
    return true;
}

} // namespace Himax::Pen
