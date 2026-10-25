// TouchBusLinux.cpp — ITouchBus 的 Linux 实现：himax-thp 内核驱动的字符设备（/dev/himax_thp）。
//
// ABI 见 Kernel/himax-thp/himax_thp.h。与 Windows 实现的对应：
//   Master / Slave  同一个设备节点各开一个文件，SET_CHANNEL 绑定通道；功能码（F2..F5）仍由这里填
//   GetFrame        poll 等帧（超时 = SetTimeOut，非阻塞 = 0）后 read；驱动在帧中断里按帧长读好，
//                   帧长取调用方给的 outLen，变了才下发 SET_FRAME_LEN
//   RequestCancel   eventfd 唤醒 poll。与 CancelIoEx 一样只打断进行中的读帧：每次 GetFrame 开始先清掉
//                   之前残留的取消请求
//   ReadBus         与 Windows 一样失败重试最多 10 次；WriteBus 不重试（Windows 的 WriteFile 也不重试）
#include "Port/TouchBus.h"

#include "himax_thp.h"

#include <cerrno>
#include <cstring>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

constexpr uint8_t OP_WRITE_MASTER = 0xF2;
constexpr uint8_t OP_READ_MASTER  = 0xF3;
constexpr uint8_t OP_WRITE_SLAVE  = 0xF4;
constexpr uint8_t OP_READ_SLAVE   = 0xF5;

constexpr size_t kBusHeader = 3;        // [功能码, cmd, dummy]
constexpr int kReadBusRetries = 10;     // 与 Windows 侧 Ioctl 的重试次数一致

class TouchBusLinux final : public Port::ITouchBus {
public:
    TouchBusLinux(const std::filesystem::path& path, Port::TouchChannel channel);
    ~TouchBusLinux() override;

    bool IsValid() const override { return m_fd >= 0 && m_cancelFd >= 0; }
    ChipResult<> ReadBus(uint8_t cmd, uint8_t* data, uint32_t len) override;
    ChipResult<> WriteBus(uint8_t cmd, const uint8_t* addr, const uint8_t* data, uint32_t len) override;
    ChipResult<> GetFrame(void* buffer, uint32_t outLen, uint32_t* retLen) override;
    ChipResult<> SetTimeOut(uint32_t millisecond) override;
    ChipResult<> SetBlock(bool status) override;
    ChipResult<> SetReset(bool state) override;
    ChipResult<> IntOpen() override { return SetInt(true); }
    ChipResult<> IntClose() override { return SetInt(false); }
    ChipResult<> ReadPanelStatus(uint8_t* data, uint32_t len) override;
    uint32_t GetError() const override { return static_cast<uint32_t>(m_lastError); }
    bool IsTimeoutError() const override { return m_lastError == ETIMEDOUT; }
    bool IsCancelledError() const override { return m_lastError == ECANCELED; }
    void RequestCancel() noexcept override;

private:
    /// 发 m_tx 的前 len 字节；wantRx 时收回到 m_rx
    ChipResult<> Xfer(uint32_t len, bool wantRx);
    ChipResult<> IoctlU32(unsigned long request, uint32_t value);
    ChipResult<> SetInt(bool enable);
    ChipResult<> Fail(int err, ChipError kind = ChipError::CommunicationError) {
        m_lastError = err;
        return std::unexpected(kind);
    }
    void DrainCancel() noexcept;

    int m_fd = -1;
    int m_cancelFd = -1;
    int m_lastError = 0;
    uint8_t m_readOp;
    uint8_t m_writeOp;
    uint32_t m_frameLen = 0;            // 已下发给驱动的帧长，0 = 还没下发
    uint32_t m_timeoutMs = 0;           // 0 = 一直等
    bool m_block = true;

    std::vector<uint8_t> m_tx;
    std::vector<uint8_t> m_rx;
};

TouchBusLinux::TouchBusLinux(const std::filesystem::path& path, Port::TouchChannel channel) {
    const bool slave = channel == Port::TouchChannel::Slave;
    m_readOp = slave ? OP_READ_SLAVE : OP_READ_MASTER;
    m_writeOp = slave ? OP_WRITE_SLAVE : OP_WRITE_MASTER;
    m_tx.reserve(HIMAX_THP_MAX_XFER);
    m_rx.reserve(HIMAX_THP_MAX_XFER);

    // 非阻塞打开：等帧统一在 poll 里做，read 只取已经到的帧
    m_fd = ::open(path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (m_fd < 0) {
        m_lastError = errno;
        return;
    }
    const uint32_t ch = slave ? HIMAX_THP_CH_SLAVE : HIMAX_THP_CH_MASTER;
    if (::ioctl(m_fd, HIMAX_THP_IOC_SET_CHANNEL, &ch) < 0) {
        m_lastError = errno;
        ::close(m_fd);
        m_fd = -1;
        return;
    }
    m_cancelFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_cancelFd < 0) {
        m_lastError = errno;
        ::close(m_fd);
        m_fd = -1;
    }
}

TouchBusLinux::~TouchBusLinux() {
    if (m_cancelFd >= 0) ::close(m_cancelFd);
    if (m_fd >= 0) ::close(m_fd);
}

ChipResult<> TouchBusLinux::Xfer(uint32_t len, bool wantRx) {
    if (!IsValid()) return Fail(EBADF);
    if (len > HIMAX_THP_MAX_XFER) return Fail(EMSGSIZE, ChipError::InvalidOperation);
    if (wantRx) m_rx.assign(len, 0);

    himax_thp_xfer x{};
    x.tx_buf = reinterpret_cast<uintptr_t>(m_tx.data());
    x.rx_buf = wantRx ? reinterpret_cast<uintptr_t>(m_rx.data()) : 0;
    x.len = len;
    if (::ioctl(m_fd, HIMAX_THP_IOC_XFER, &x) < 0) return Fail(errno);
    m_lastError = 0;
    return {};
}

ChipResult<> TouchBusLinux::ReadBus(uint8_t cmd, uint8_t* data, uint32_t len) {
    const uint32_t total = static_cast<uint32_t>(kBusHeader) + len;
    m_tx.assign(total, 0);
    m_tx[0] = m_readOp;
    m_tx[1] = cmd;

    ChipResult<> res = std::unexpected(ChipError::InternalError);
    for (int i = 0; i < kReadBusRetries; ++i) {
        res = Xfer(total, true);
        if (res) break;
    }
    if (!res) return res;

    std::memcpy(data, m_rx.data() + kBusHeader, len);
    return {};
}

ChipResult<> TouchBusLinux::WriteBus(uint8_t cmd, const uint8_t* addr, const uint8_t* data, uint32_t len) {
    m_tx.clear();
    m_tx.push_back(m_writeOp);
    m_tx.push_back(cmd);
    if (addr != nullptr) m_tx.insert(m_tx.end(), addr, addr + 4);
    if (data != nullptr) m_tx.insert(m_tx.end(), data, data + len);
    return Xfer(static_cast<uint32_t>(m_tx.size()), false);
}

void TouchBusLinux::DrainCancel() noexcept {
    uint64_t v;
    while (::read(m_cancelFd, &v, sizeof(v)) == static_cast<ssize_t>(sizeof(v))) {
    }
}

ChipResult<> TouchBusLinux::GetFrame(void* buffer, uint32_t outLen, uint32_t* retLen) {
    if (!IsValid()) return Fail(EBADF);
    if (outLen != m_frameLen) {
        if (auto res = IoctlU32(HIMAX_THP_IOC_SET_FRAME_LEN, outLen); !res) return res;
        m_frameLen = outLen;
    }
    DrainCancel();

    pollfd fds[2] = {
        {m_fd, POLLIN, 0},
        {m_cancelFd, POLLIN, 0},
    };
    const int timeout = !m_block ? 0 : (m_timeoutMs == 0 ? -1 : static_cast<int>(m_timeoutMs));
    int n;
    do {
        n = ::poll(fds, 2, timeout);
    } while (n < 0 && errno == EINTR);
    if (n < 0) return Fail(errno);
    if (n == 0) return Fail(ETIMEDOUT, ChipError::Timeout);
    if (fds[1].revents & POLLIN) {
        DrainCancel();
        return Fail(ECANCELED);
    }
    if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) return Fail(ENODEV);

    const ssize_t got = ::read(m_fd, buffer, outLen);
    if (got < 0) {
        // poll 报了可读、read 却没帧：被别的读者抢先取走，按超时处理
        if (errno == EAGAIN) return Fail(ETIMEDOUT, ChipError::Timeout);
        return Fail(errno);
    }
    if (retLen) *retLen = static_cast<uint32_t>(got);
    m_lastError = 0;
    return {};
}

ChipResult<> TouchBusLinux::SetTimeOut(uint32_t millisecond) {
    m_timeoutMs = millisecond;
    return {};
}

ChipResult<> TouchBusLinux::SetBlock(bool status) {
    m_block = status;
    return {};
}

ChipResult<> TouchBusLinux::IoctlU32(unsigned long request, uint32_t value) {
    if (!IsValid()) return Fail(EBADF);
    if (::ioctl(m_fd, request, &value) < 0) return Fail(errno);
    m_lastError = 0;
    return {};
}

ChipResult<> TouchBusLinux::SetReset(bool state) {
    return IoctlU32(HIMAX_THP_IOC_SET_RESET, state ? 1u : 0u);
}

ChipResult<> TouchBusLinux::SetInt(bool enable) {
    return IoctlU32(HIMAX_THP_IOC_INT_ENABLE, enable ? 1u : 0u);
}

ChipResult<> TouchBusLinux::ReadPanelStatus(uint8_t* data, uint32_t len) {
    if (!IsValid()) return Fail(EBADF);
    uint32_t state = 0;
    if (::ioctl(m_fd, HIMAX_THP_IOC_PANEL_STATE, &state) < 0) return Fail(errno);
    // 与 Windows 的 ACPI 读取同形：小端状态字，bit0 = 屏幕已上电
    std::memset(data, 0, len);
    for (uint32_t i = 0; i < len && i < sizeof(state); ++i) {
        data[i] = static_cast<uint8_t>(state >> (8 * i));
    }
    m_lastError = 0;
    return {};
}

void TouchBusLinux::RequestCancel() noexcept {
    if (m_cancelFd < 0) return;
    const uint64_t one = 1;
    (void)!::write(m_cancelFd, &one, sizeof(one));
}

} // anonymous namespace

namespace Port {

std::unique_ptr<ITouchBus> CreateTouchBus(const std::filesystem::path& path, TouchChannel channel) {
    return std::make_unique<TouchBusLinux>(path, channel);
}

} // namespace Port
