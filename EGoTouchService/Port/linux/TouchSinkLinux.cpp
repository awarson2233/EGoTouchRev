// TouchSinkLinux.cpp — ITouchSink 的 Linux 实现：uhid 虚拟 HID 触摸屏。
//
// 描述符按 TouchSink.h 的报告布局写，内核的 hid-multitouch 认出 Contact Identifier 后接管，生成触摸屏的
// 输入设备（相当于 Windows 上的 VHF 注入驱动）。描述符里没有 Win8 认证的 feature 报告，hid-multitouch 按
// 默认类处理：报告里没出现的触点不会自动抬起，所以这里记下还按着的 ID，ReleaseAll 时逐个发抬起，
// 而不是像 VhfReporter 那样只发一个触点数为 0 的报告。
#include "Port/TouchSink.h"

#include <array>
#include <bitset>
#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <linux/uhid.h>
#include <unistd.h>

namespace {

constexpr size_t kReportLen = 32;
constexpr size_t kContactsPerReport = 5;
constexpr size_t kContactStride = 6;
constexpr size_t kContactCountOffset = 31;
constexpr uint8_t kReportId = 0x01;
constexpr uint8_t kStateTip = 0x01;
constexpr uint8_t kStateConfidence = 0x02;

// 一个触点：Tip Switch + Confidence（1 bit 各）、6 bit 填充、Contact Identifier（8 bit）、X、Y（16 bit 各）。
// 物理尺寸取自 DSDT 面板配置（166 × 265 mm），单位厘米、指数 -2。
#define EGO_FINGER(xmax_lo, xmax_hi, ymax_lo, ymax_hi) \
    0x05, 0x0D,                          /*   Usage Page (Digitizer)          */ \
    0x09, 0x22,                          /*   Usage (Finger)                  */ \
    0xA1, 0x02,                          /*   Collection (Logical)            */ \
    0x35, 0x00, 0x45, 0x00,              /*     Physical Min / Max 0          */ \
    0x55, 0x00, 0x65, 0x00,              /*     Unit Exponent 0, Unit None    */ \
    0x09, 0x42, 0x09, 0x47,              /*     Usage (Tip Switch, Confidence)*/ \
    0x15, 0x00, 0x25, 0x01,              /*     Logical Min 0, Max 1          */ \
    0x75, 0x01, 0x95, 0x02, 0x81, 0x02,  /*     2 x 1 bit, Input (Var)        */ \
    0x75, 0x06, 0x95, 0x01, 0x81, 0x03,  /*     6 bit padding, Input (Const)  */ \
    0x09, 0x51,                          /*     Usage (Contact Identifier)    */ \
    0x26, 0xFF, 0x00,                    /*     Logical Max 255               */ \
    0x75, 0x08, 0x95, 0x01, 0x81, 0x02,  /*     8 bit, Input (Var)            */ \
    0x05, 0x01,                          /*     Usage Page (Generic Desktop)  */ \
    0x55, 0x0E, 0x65, 0x11,              /*     Unit Exponent -2, Unit cm     */ \
    0x75, 0x10, 0x95, 0x01,              /*     16 bit                        */ \
    0x09, 0x30,                          /*     Usage (X)                     */ \
    0x26, xmax_lo, xmax_hi,              /*     Logical Max                   */ \
    0x46, 0x7C, 0x06,                    /*     Physical Max 1660 (16.60 cm)  */ \
    0x81, 0x02,                          /*     Input (Var)                   */ \
    0x09, 0x31,                          /*     Usage (Y)                     */ \
    0x26, ymax_lo, ymax_hi,              /*     Logical Max                   */ \
    0x46, 0x5A, 0x0A,                    /*     Physical Max 2650 (26.50 cm)  */ \
    0x81, 0x02,                          /*     Input (Var)                   */ \
    0xC0                                 /*   End Collection                  */

// X 0..16000（0x3E80）、Y 0..25600（0x6400），与 VhfTouchPacket 的逻辑量程一致
#define EGO_FINGER_XY EGO_FINGER(0x80, 0x3E, 0x00, 0x64)

constexpr uint8_t kDescriptor[] = {
    0x05, 0x0D,        // Usage Page (Digitizer)
    0x09, 0x04,        // Usage (Touch Screen)
    0xA1, 0x01,        // Collection (Application)
    0x85, kReportId,   //   Report ID (1)
    EGO_FINGER_XY,
    EGO_FINGER_XY,
    EGO_FINGER_XY,
    EGO_FINGER_XY,
    EGO_FINGER_XY,
    0x05, 0x0D,        //   Usage Page (Digitizer)
    0x35, 0x00, 0x45, 0x00, 0x55, 0x00, 0x65, 0x00,
    0x09, 0x54,        //   Usage (Contact Count)
    0x15, 0x00, 0x25, 0x7F,
    0x75, 0x08, 0x95, 0x01, 0x81, 0x02,
    0xC0,              // End Collection
};

#undef EGO_FINGER_XY
#undef EGO_FINGER

class TouchSinkLinux final : public Port::ITouchSink {
public:
    TouchSinkLinux();
    ~TouchSinkLinux() override;

    bool IsValid() const override { return m_fd >= 0; }
    bool WriteReport(const uint8_t* report, size_t len) override;
    void ReleaseAll() override;
    uint32_t GetError() const override { return static_cast<uint32_t>(m_lastError); }

private:
    bool Send(const uhid_event& ev);
    void DrainEvents();

    int m_fd = -1;
    int m_lastError = 0;
    std::bitset<256> m_down;   // 最近一次报告里按着的触点 ID
};

TouchSinkLinux::TouchSinkLinux() {
    m_fd = open("/dev/uhid", O_RDWR | O_CLOEXEC | O_NONBLOCK);
    if (m_fd < 0) {
        m_lastError = errno;
        return;
    }
    uhid_event ev{};
    ev.type = UHID_CREATE2;
    std::strncpy(reinterpret_cast<char*>(ev.u.create2.name), "EGoTouch Touchscreen",
                 sizeof(ev.u.create2.name) - 1);
    std::strncpy(reinterpret_cast<char*>(ev.u.create2.phys), "egotouch/himax-thp",
                 sizeof(ev.u.create2.phys) - 1);
    static_assert(sizeof(kDescriptor) <= sizeof(ev.u.create2.rd_data));
    std::memcpy(ev.u.create2.rd_data, kDescriptor, sizeof(kDescriptor));
    ev.u.create2.rd_size = sizeof(kDescriptor);
    ev.u.create2.bus = BUS_VIRTUAL;
    ev.u.create2.vendor = 0x1209;    // pid.codes 的测试 ID，不冒用任何厂商
    ev.u.create2.product = 0x0001;
    ev.u.create2.version = 1;
    if (!Send(ev)) {
        close(m_fd);
        m_fd = -1;
    }
}

TouchSinkLinux::~TouchSinkLinux() {
    if (m_fd < 0) return;
    ReleaseAll();
    uhid_event ev{};
    ev.type = UHID_DESTROY;
    Send(ev);
    close(m_fd);
}

bool TouchSinkLinux::Send(const uhid_event& ev) {
    const ssize_t n = write(m_fd, &ev, sizeof(ev));
    if (n != static_cast<ssize_t>(sizeof(ev))) {
        m_lastError = n < 0 ? errno : EIO;
        return false;
    }
    return true;
}

// 内核发来的 START / OPEN / CLOSE 等事件不需要处理；描述符里没有 feature 报告，不会有 GET_REPORT。
// 读掉免得队列满了丢事件时内核打警告。
void TouchSinkLinux::DrainEvents() {
    uhid_event ev;
    while (read(m_fd, &ev, sizeof(ev)) > 0) {
    }
}

bool TouchSinkLinux::WriteReport(const uint8_t* report, size_t len) {
    if (m_fd < 0 || len != kReportLen || report[0] != kReportId) return false;
    DrainEvents();

    // 第一包带触点总数，开始新的一帧：之前按着的 ID 以本帧为准重新记
    if (report[kContactCountOffset] != 0) m_down.reset();
    for (size_t i = 0; i < kContactsPerReport; ++i) {
        const uint8_t* c = report + 1 + i * kContactStride;
        if (c[0] == 0) continue;   // 空位
        m_down.set(c[1], (c[0] & kStateTip) != 0);
    }

    uhid_event ev{};
    ev.type = UHID_INPUT2;
    std::memcpy(ev.u.input2.data, report, len);
    ev.u.input2.size = static_cast<uint16_t>(len);
    return Send(ev);
}

void TouchSinkLinux::ReleaseAll() {
    if (m_fd < 0 || m_down.none()) return;
    std::array<uint8_t, kReportLen> report{};
    size_t total = m_down.count(), slot = 0;
    bool first = true;
    for (size_t id = 0; id < m_down.size(); ++id) {
        if (!m_down.test(id)) continue;
        uint8_t* c = report.data() + 1 + slot * kContactStride;
        c[0] = kStateConfidence;   // 不按下 = 抬起
        c[1] = static_cast<uint8_t>(id);
        // 抬起时的坐标不影响 hid-multitouch，留 0
        if (++slot == kContactsPerReport || --total == 0) {
            report[0] = kReportId;
            report[kContactCountOffset] = first ? static_cast<uint8_t>(m_down.count()) : 0;
            uhid_event ev{};
            ev.type = UHID_INPUT2;
            std::memcpy(ev.u.input2.data, report.data(), report.size());
            ev.u.input2.size = static_cast<uint16_t>(report.size());
            Send(ev);
            report.fill(0);
            slot = 0;
            first = false;
        }
    }
    m_down.reset();
}

} // namespace

namespace Port {

std::unique_ptr<ITouchSink> CreateTouchSink() {
    return std::make_unique<TouchSinkLinux>();
}

} // namespace Port
