// TouchBusWin32.cpp — ITouchBus 的 Windows 实现：SPBTESTTOOL 驱动的设备对象。
//
// Master / Slave 是两个设备对象（\\.\Global\SPBTESTTOOL_MASTER / _SLAVE），各开一个句柄；
// 所有 I/O 都是 overlapped 发出、同步等完成，这样 RequestCancel 可以从别的线程用 CancelIoEx 打断。
#include "Port/TouchBus.h"

#include <cstring>
#include <vector>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace {

constexpr uint8_t OP_WRITE_MASTER = 0xF2;
constexpr uint8_t OP_READ_MASTER  = 0xF3;
constexpr uint8_t OP_WRITE_SLAVE  = 0xF4;
constexpr uint8_t OP_READ_SLAVE   = 0xF5;

const DWORD SPI_IOCTL_INT_OPEN    = 0x4001c00; // 打开中断/初始化
const DWORD SPI_IOCTL_INT_CLOSE   = 0x4001c04; // 关闭中断
const DWORD SPI_IOCTL_WRITEREAD   = 0x4001c10; // [不常用] 偶见于特定初始化流
const DWORD SPI_IOCTL_WAIT_INT    = 0x4001c20; // 等待中断触发
const DWORD SPI_IOCTL_FULL_DUPLEX = 0x4001c24; // [核心] BusRead / 全双工读
const DWORD SPI_IOCTL_GET_FRAME   = 0x4001c28; // 获取帧数据
const DWORD SPI_IOCTL_SET_TIMEOUT = 0x4001c2c; // 设置超时
const DWORD SPI_IOCTL_SET_BLOCK   = 0x4001c30; // 设置阻塞模式
const DWORD SPI_IOCTL_SET_RESET   = 0x4001c34; // 复位设备
const DWORD SPI_IOCTL_READ_ACPI   = 0x4001c38; // 读取 ACPI 配置

/**
 * @brief 执行同步 I/O 操作（封装 Overlapped 异步操作为同步）
 * @param handle 文件句柄
 * @param hEvent overlapped 用的事件
 * @param lastError 错误码输出
 * @param ioFunc I/O 函数 lambda
 * @param expectedLen 预期长度
 * @param checkLen 是否检查长度
 * @param outBytes 实际传输字节数输出
 * @return bool 是否成功
 */
template<typename Func>
ChipResult<> PerformSyncIo(HANDLE handle, HANDLE hEvent, DWORD& lastError, Func ioFunc, ChipError errType, DWORD expectedLen = 0, bool checkLen = false, DWORD* outBytes = NULL) {
    if (handle == INVALID_HANDLE_VALUE) {
        lastError = ERROR_INVALID_HANDLE;
        return std::unexpected(ChipError::CommunicationError);
    }
    if (hEvent == NULL) {
        lastError = ERROR_INVALID_PARAMETER;
        return std::unexpected(ChipError::InternalError);
    }
    ResetEvent(hEvent);
    OVERLAPPED ov = {0};
    ov.hEvent = hEvent;

    DWORD bytesTransferred = 0;
    BOOL res = ioFunc(&ov, &bytesTransferred);

    if (!res) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            if (WaitForSingleObject(ov.hEvent, INFINITE) == WAIT_OBJECT_0) {
                res = GetOverlappedResult(handle, &ov, &bytesTransferred, TRUE);
                if (!res) lastError = GetLastError();
            } else {
                lastError = GetLastError();
            }
        } else {
            lastError = err;
        }
    }

    if (!res) return std::unexpected(errType);
    if (checkLen && bytesTransferred != expectedLen) {
        lastError = ERROR_WRITE_FAULT;
        return std::unexpected(ChipError::CommunicationError);
    }

    if (outBytes) *outBytes = bytesTransferred;
    lastError = 0;
    return {};
}

class TouchBusWin32 final : public Port::ITouchBus {
public:
    TouchBusWin32(const wchar_t* path, Port::TouchChannel channel);
    ~TouchBusWin32() override;

    bool IsValid() const override { return m_handle != INVALID_HANDLE_VALUE; }
    ChipResult<> ReadBus(uint8_t cmd, uint8_t* data, uint32_t len) override;
    ChipResult<> WriteBus(uint8_t cmd, const uint8_t* addr, const uint8_t* data, uint32_t len) override;
    ChipResult<> GetFrame(void* buffer, uint32_t outLen, uint32_t* retLen) override;
    ChipResult<> SetTimeOut(uint32_t millisecond) override;
    ChipResult<> SetBlock(bool status) override;
    ChipResult<> SetReset(bool state) override;
    ChipResult<> IntOpen() override;
    ChipResult<> IntClose() override;
    ChipResult<> ReadPanelStatus(uint8_t* data, uint32_t len) override;
    uint32_t GetError() const override { return m_lastError; }
    bool IsTimeoutError() const override;
    bool IsCancelledError() const override;
    void RequestCancel() noexcept override;

private:
    ChipResult<> Ioctl(DWORD code, const void* in, uint32_t inLen, void* out,
                       uint32_t outLen, uint32_t* retLen);
    ChipResult<> IoctlOnce(DWORD code, const void* in, uint32_t inLen, void* out,
                           uint32_t outLen, uint32_t* retLen);

    HANDLE m_handle;
    HANDLE m_syncEvent = nullptr;
    DWORD m_lastError;

    uint8_t m_readOp;
    uint8_t m_writeOp;
    const size_t dummy_size = 1;
    const size_t header_size = 2;
    const size_t data_offset = header_size + dummy_size;

    std::vector<uint8_t> m_xfer_buffer;
};

/**
 * @brief 构造函数，打开设备句柄并初始化
 * @param path 设备路径
 * @param channel 通道 (Master/Slave)
 */
TouchBusWin32::TouchBusWin32(const wchar_t* path, Port::TouchChannel channel) : m_handle(INVALID_HANDLE_VALUE), m_lastError(0) {
    m_xfer_buffer.reserve(0x4000 + 32);

    m_handle = CreateFileW(
        path,
        GENERIC_ALL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );
    if (m_handle == INVALID_HANDLE_VALUE) {
        m_lastError = GetLastError();
    }

    m_syncEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (m_syncEvent == nullptr) {
        m_lastError = GetLastError();
        if (m_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
        return;
    }

    if (channel == Port::TouchChannel::Slave) {
        m_readOp = OP_READ_SLAVE;
        m_writeOp = OP_WRITE_SLAVE;
    } else {
        m_readOp = OP_READ_MASTER;
        m_writeOp = OP_WRITE_MASTER;
    }
}

/**
 * @brief 析构函数，关闭句柄
 */
TouchBusWin32::~TouchBusWin32() {
    if (m_syncEvent) {
        CloseHandle(m_syncEvent);
        m_syncEvent = nullptr;
    }

    if (IsValid()) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

/**
 * @brief 执行 DeviceIoControl 操作
 * @param code 控制码
 * @param in 输入缓冲区
 * @param inLen 输入长度
 * @param out 输出缓冲区
 * @param outLen 输出长度
 * @param retLen 实际返回长度
 * @return bool 是否成功
 */
ChipResult<> TouchBusWin32::IoctlOnce(DWORD code, const void* in, uint32_t inLen, void* out, uint32_t outLen, uint32_t* retLen) {
    return PerformSyncIo(m_handle, m_syncEvent, m_lastError, [&](OVERLAPPED* pov, LPDWORD pBytes) {
        return DeviceIoControl(m_handle, code, (LPVOID)in, inLen, out, outLen, pBytes, pov);
    }, ChipError::CommunicationError, 0, false, (DWORD*)retLen);
}

ChipResult<> TouchBusWin32::Ioctl(DWORD code, const void* in, uint32_t inLen, void* out, uint32_t outLen, uint32_t* retLen) {
    ChipResult<> result = std::unexpected(ChipError::InternalError);
    for (int cnt = 0; cnt < 10; cnt++) {
        result = IoctlOnce(code, in, inLen, out, outLen, retLen);
        if (result) break;
        // If the I/O was cancelled via CancelIoEx, do not retry.
        if (m_lastError == ERROR_OPERATION_ABORTED) break;
    }

    return result;
}

/**
 * @brief 通过总线读取数据
 * @param cmd 命令码
 * @param data 接收缓冲区
 * @param len 读取长度
 * @return bool 是否成功
 */
ChipResult<> TouchBusWin32::ReadBus(uint8_t cmd, uint8_t* data, uint32_t len) {
    size_t total_size = data_offset + len;

    m_xfer_buffer.clear();

    m_xfer_buffer.push_back(m_readOp);
    m_xfer_buffer.push_back(cmd);
    m_xfer_buffer.push_back(0x00);      //Dummy Byte

    m_xfer_buffer.resize(total_size, 0);

    uint32_t retLen = 0;
    auto res = Ioctl(SPI_IOCTL_FULL_DUPLEX,
                        m_xfer_buffer.data(), m_xfer_buffer.size(),
                        m_xfer_buffer.data(), m_xfer_buffer.size(),
                        &retLen);

    if (!res) return std::unexpected(ChipError::CommunicationError);

    if (retLen < total_size) {
        return std::unexpected(ChipError::CommunicationError);
    }

    memcpy(data, m_xfer_buffer.data() + data_offset, len);

    return {};
}

/**
 * @brief 通过总线写入数据
 * @param cmd 命令码
 * @param addr 地址 (可选)
 * @param data 数据缓冲区
 * @param len 数据长度
 * @return bool 是否成功
 */
ChipResult<> TouchBusWin32::WriteBus(const uint8_t cmd, const uint8_t* addr, const uint8_t* data, const uint32_t len) {
    m_xfer_buffer.clear();
    m_xfer_buffer.push_back(m_writeOp);
    m_xfer_buffer.push_back(cmd);

    if (addr != NULL) {
        m_xfer_buffer.insert(m_xfer_buffer.end(), addr, addr + 4);
    }
    if (data != NULL) {
        m_xfer_buffer.insert(m_xfer_buffer.end(), data, data + len);
    }

    return PerformSyncIo(m_handle, m_syncEvent, m_lastError, [&](OVERLAPPED* pov, LPDWORD pBytes){
        return WriteFile(m_handle, m_xfer_buffer.data(), (DWORD)m_xfer_buffer.size(), NULL, pov);
    }, ChipError::CommunicationError);
}

/**
 * @brief 读取 ACPI 配置数据（面板电源状态字）
 * @param data 接收缓冲区
 * @param len 读取长度
 * @return bool 是否成功
 */
ChipResult<> TouchBusWin32::ReadPanelStatus(uint8_t* data, uint32_t len) {
    m_xfer_buffer.assign(len, 0);

    uint32_t retLen = 0;

    auto res = Ioctl(SPI_IOCTL_READ_ACPI, nullptr, 0, m_xfer_buffer.data(), len, &retLen);
    if (!res) return std::unexpected(ChipError::CommunicationError);

    if (retLen < len) {
        return std::unexpected(ChipError::CommunicationError);
    }

    memcpy(data, m_xfer_buffer.data(), len);
    return {};
}

/**
 * @brief 获取一帧完整的触摸数据
 * @param buffer 接收缓冲区
 * @param outLen 缓冲区长度
 * @param retLen 实际返回长度
 * @return bool 是否成功
 */
ChipResult<> TouchBusWin32::GetFrame(void* buffer, uint32_t outLen, uint32_t *retLen){
    // 只发一次（与 SpiModule!SpiGetFrame 一致）：等多久由驱动的 SetTimeOut 决定，
    // 读不到帧之后怎么重试是 Chip::GetFrame 的事，这里再重试会把超时放大十倍
    return IoctlOnce(SPI_IOCTL_GET_FRAME, NULL, 0, buffer, outLen, retLen);
}

/**
 * @brief 设置 I/O 超时时间
 * @param milisecond 毫秒数
 * @return bool 是否成功
 */
ChipResult<> TouchBusWin32::SetTimeOut(uint32_t milisecond) {
    return Ioctl(SPI_IOCTL_SET_TIMEOUT, &milisecond, sizeof(milisecond), NULL, 0, NULL);
}

/**
 * @brief 设置阻塞或非阻塞模式
 * @param state true 为阻塞, false 为非阻塞
 * @return bool 是否成功
 */
ChipResult<> TouchBusWin32::SetBlock(bool state) {
    m_xfer_buffer.clear();
    m_xfer_buffer.push_back(uint8_t(state));
    m_xfer_buffer.resize(4, 0);

    return Ioctl(SPI_IOCTL_SET_BLOCK, m_xfer_buffer.data(), 4, NULL, 0, NULL);
}

/**
 * @brief 设置设备复位状态
 * @param state true 为拉高复位, false 为拉低复位
 * @return bool 是否成功
 */
ChipResult<> TouchBusWin32::SetReset(bool state) {
    uint32_t val = state ? 1 : 0;
    return Ioctl(SPI_IOCTL_SET_RESET, &val, sizeof(val), NULL, 0, NULL);
}

/**
 * @brief 打开中断监听
 * @return bool 是否成功
 */
ChipResult<> TouchBusWin32::IntOpen(void) {
    return Ioctl(SPI_IOCTL_INT_OPEN, NULL, 0, NULL, 0, NULL);
}

/**
 * @brief 关闭中断监听
 * @return bool 是否成功
 */
ChipResult<> TouchBusWin32::IntClose(void) {
    return Ioctl(SPI_IOCTL_INT_CLOSE, NULL, 0, NULL, 0, NULL);
}

bool TouchBusWin32::IsTimeoutError() const {
    return m_lastError == WAIT_TIMEOUT ||
           m_lastError == ERROR_TIMEOUT ||
           m_lastError == ERROR_SEM_TIMEOUT;
}

bool TouchBusWin32::IsCancelledError() const {
    return m_lastError == ERROR_OPERATION_ABORTED;
}

void TouchBusWin32::RequestCancel() noexcept {
    if (m_handle != INVALID_HANDLE_VALUE) {
        CancelIoEx(m_handle, NULL);
    }
}

} // anonymous namespace

namespace Port {

std::unique_ptr<ITouchBus> CreateTouchBus(const std::filesystem::path& path, TouchChannel channel) {
    return std::make_unique<TouchBusWin32>(path.c_str(), channel);
}

} // namespace Port
