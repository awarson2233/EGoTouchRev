// ChipHidProbe —— 验证 SPI 模式下把 master 的接口模式字 0x10007540 改成 1 后，片上坐标管线与 HID 报告组装是否工作。
//
// 流程：停 EGoTouchService → Chip::Init 建链 → 读基线 → 写 0x10007540 = 1 → 轮询报告缓冲 0x1001B438
// 与坐标输出 0x10002718 → 写回 0 、拉高 INT → Deinit → 恢复服务。不写 flash；复位即恢复原状。
// 依据见 docs/artifacts/spi-片上解算可行性.md 第 3 节。
//
//   ChipHidProbe.exe [--seconds N] [--out FILE] [--keep-raw]（保留 raw_out_sel = 0xF6，默认切模式前清 0）

#include <windows.h>
#include <mmsystem.h>

#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "Logger.h"
#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"

namespace {

using Clock = std::chrono::steady_clock;

const std::wstring kDevicePathMaster = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
const std::wstring kDevicePathSlave = L"\\\\.\\Global\\SPBTESTTOOL_SLAVE";
const wchar_t* kServiceName = L"EGoTouchService";

constexpr uint32_t kIfMode = 0x10007540;       // 接口模式字，1 = I²C 行为
constexpr uint32_t kRawOutSel = 0x100072EC;    // raw_out_sel：原厂 I²C 下由 cfg_load 清 0，raw_frame_build 随之不再出帧
constexpr uint32_t kStageCode = 0x10007F40;    // 阶段码，低字节主循环 0x18..0x28
constexpr uint32_t kReady = 0x100072C0;        // 0x72C0 = 固件就绪
constexpr uint32_t kSortMode = 0x10007F04;     // 检测模式字
constexpr uint32_t kFrameCounter = 0x100068D0; // +2: 0x500068D2 每帧 +1
constexpr uint32_t kModeState = 0x08013738;    // 扫描模式状态 0x20013738（ISRAM .bss 的主机视角）
constexpr uint32_t kReportBuf = 0x1001B438;    // 2 包 × 44 B
constexpr uint32_t kReportState = 0x1001B514;  // [0]=状态 0 已组装/2 空闲, [1]=包数, [2..3]=字节数
constexpr uint32_t kCoordOut = 0x10002718;     // 10 点 × 12 B
constexpr uint32_t kIntLevel = 0x90028068;
constexpr uint32_t kDelta = 0x1000D134;        // 40×60 s16：时域平均后的 delta，阈值比较用它
constexpr uint32_t kDeltaStats = 0x1000D124;   // +0 过阈值格数, +2 正 delta 格数, +4 最大 delta
constexpr uint32_t kNegFlag = 0x1000367C;      // bit0/1：出现负 delta（基线异常）
constexpr uint32_t kBaselinePtr = 0x1001B528;  // 基线数组指针（MCU 地址）
constexpr int kRows = 40, kCols = 60;

/// 状态跟踪：每 20 ms 读一次，值变了才记录
struct Watch {
    const char* name;
    uint32_t addr;
    uint32_t mask;
};
constexpr Watch kWatches[] = {
    {"rek_cmd", 0x1001B520, 0xFFFFFFFF},    // 0x5001B520：A55AA55A 软复位 / A33AA33A / 3AA33AA3，非 0 时主循环自旋等它清零
    {"neg_flags", 0x1000367C, 0x0000FFFF},  // bit0 delta 低于负阈值, bit1 负 delta, bit5 基线漂移
    {"bl_marks", 0x100068FC, 0xFFFFFFFF},   // 0x500068FE = 0x42：基线跟踪发现漂移
    {"panel_7544", 0x10007544, 0xFFFFFFFF}, // 面板侧状态（0x300E2001 高半字节）
    {"bigsig", 0x1001E840, 0xFFFF0000},     // 0x5001E842：大信号保护
    {"b570_574", 0x1001B570, 0xFFFFFFFF},   // 0x5001B570
    {"b574", 0x1001B574, 0xFFFFFFFF},
    {"rek_cnt_7f50", 0x10007F50, 0x0000FFFF}, // 0x20002554 写入的重校准计数
};

void Out(const char* fmt, ...);
bool RdBig(Himax::HalDevice* dev, uint32_t addr, uint8_t* buf, uint32_t len);

/// 基线快照：两个频点各一份（0x50008124 / 0x50009524，各 0x1400 B）
struct BaselineTracker {
    std::array<std::array<uint16_t, 0xA00>, 2> ref{};
    std::array<bool, 2> haveRef{};

    void Sample(Himax::HalDevice* dev, double t, uint32_t blPtrMcu) {
        int idx = -1;
        if (blPtrMcu == 0x50008124) idx = 0;
        else if (blPtrMcu == 0x50009524) idx = 1;
        if (idx < 0) {
            Out("[%7.3f] BASELINE 指针异常 0x%08X\n", t, blPtrMcu);
            return;
        }
        std::array<uint8_t, 0x1400> raw{};
        if (!RdBig(dev, blPtrMcu - 0x40000000, raw.data(), static_cast<uint32_t>(raw.size()))) return;
        std::array<uint16_t, 0xA00> cur{};
        for (size_t i = 0; i < cur.size(); ++i) {
            cur[i] = raw[i * 2] | (raw[i * 2 + 1] << 8);
            if (cur[i] == 0xFEED || cur[i] == 0xDEAD) return;  // 读毛刺，整帧丢弃
        }
        if (!haveRef[idx]) {
            ref[idx] = cur;
            haveRef[idx] = true;
            Out("[%7.3f] BASELINE[%d] 记录参考\n", t, idx);
            return;
        }
        int up500 = 0, up1000 = 0, down500 = 0, maxUp = 0, maxDown = 0;
        for (size_t i = 0; i < cur.size(); ++i) {
            const int d = static_cast<int>(cur[i]) - static_cast<int>(ref[idx][i]);
            if (d > 500) ++up500;
            if (d > 1000) ++up1000;
            if (d < -500) ++down500;
            if (d > maxUp) maxUp = d;
            if (d < maxDown) maxDown = d;
        }
        Out("[%7.3f] BASELINE[%d] vs 参考: >+500:%d >+1000:%d <-500:%d 最大+%d 最大%d\n", t, idx, up500, up1000, down500, maxUp,
            maxDown);
    }
};

std::FILE* g_out = nullptr;

void Out(const char* fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    std::fputs(buf, stdout);
    std::fflush(stdout);
    if (g_out) {
        std::fputs(buf, g_out);
        std::fflush(g_out);
    }
}

bool Rd(Himax::HalDevice* dev, uint32_t addr, uint8_t* buf, uint32_t len) {
    return static_cast<bool>(Himax::HimaxProtocol::register_read(dev, addr, buf, len));
}

bool RdU32(Himax::HalDevice* dev, uint32_t addr, uint32_t& v) {
    uint8_t b[4]{};
    if (!Rd(dev, addr, b, 4)) return false;
    v = b[0] | (b[1] << 8) | (b[2] << 16) | (static_cast<uint32_t>(b[3]) << 24);
    return true;
}

bool WrU32(Himax::HalDevice* dev, uint32_t addr, uint32_t v) {
    const uint8_t b[4] = {static_cast<uint8_t>(v), static_cast<uint8_t>(v >> 8), static_cast<uint8_t>(v >> 16),
                          static_cast<uint8_t>(v >> 24)};
    return static_cast<bool>(Himax::HimaxProtocol::register_write(dev, addr, b, 4));
}

void DumpWord(Himax::HalDevice* dev, const char* name, uint32_t addr) {
    uint32_t v = 0;
    if (RdU32(dev, addr, v)) {
        Out("  %-14s 0x%08X = 0x%08X\n", name, addr, v);
    } else {
        Out("  %-14s 0x%08X = <读失败>\n", name, addr);
    }
}

class ServiceGuard {
public:
    ~ServiceGuard() { Restore(); }

    bool StopIfRunning() {
        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (scm == nullptr) return true;
        m_service = OpenServiceW(scm, kServiceName, SERVICE_QUERY_STATUS | SERVICE_STOP | SERVICE_START);
        CloseServiceHandle(scm);
        if (m_service == nullptr) return true;
        if (State() != SERVICE_RUNNING && State() != SERVICE_START_PENDING) return true;

        SERVICE_STATUS status{};
        if (!ControlService(m_service, SERVICE_CONTROL_STOP, &status) && GetLastError() != ERROR_SERVICE_NOT_ACTIVE) {
            Out("停止服务失败（错误 %lu）\n", GetLastError());
            return false;
        }
        m_wasRunning = true;
        for (int i = 0; i < 200 && State() != SERVICE_STOPPED; ++i) Sleep(100);
        if (State() != SERVICE_STOPPED) {
            Out("服务没有在 20 秒内停下\n");
            return false;
        }
        Out("服务已停止\n");
        return true;
    }

    void Restore() {
        if (m_service == nullptr) return;
        if (m_wasRunning) {
            if (StartServiceW(m_service, 0, nullptr)) {
                Out("EGoTouchService 已重新启动\n");
            } else {
                Out("重新启动 EGoTouchService 失败（错误 %lu）\n", GetLastError());
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

/// 解析报告缓冲：每包 2C 00 01 n + 5 × 8 B（flag, size×3, X u16, Y u16）
void PrintReport(const uint8_t* buf, uint8_t packets, double tSec) {
    Out("[%7.3f] HID 报告 %u 包, 条目数 %u:", tSec, packets, buf[3]);
    for (int p = 0; p < packets && p < 2; ++p) {
        const uint8_t* pk = buf + p * 44;
        if (pk[0] != 0x2C || pk[1] != 0x00 || pk[2] != 0x01) {
            Out(" [包%d 头异常 %02X %02X %02X %02X]", p, pk[0], pk[1], pk[2], pk[3]);
        }
        for (int k = 0; k < 5; ++k) {
            const uint8_t* e = pk + 4 + k * 8;
            if (e[0] == 0) continue;
            const unsigned x = e[4] | (e[5] << 8);
            const unsigned y = e[6] | (e[7] << 8);
            Out(" {id%u %s s%u x%u y%u}", e[0] >> 2, (e[0] & 3) == 3 ? "down" : "up", e[1], x, y);
        }
    }
    Out("\n");
}

/// 分块读一块大于 256 B 的区域
bool RdBig(Himax::HalDevice* dev, uint32_t addr, uint8_t* buf, uint32_t len) {
    for (uint32_t off = 0; off < len; off += 256) {
        const uint32_t n = (len - off) < 256 ? (len - off) : 256;
        if (!Rd(dev, addr + off, buf + off, n)) return false;
    }
    return true;
}

bool ReportHeaderOk(const uint8_t* buf, uint8_t packets) {
    for (int p = 0; p < packets && p < 2; ++p) {
        const uint8_t* pk = buf + p * 44;
        if (pk[0] != 0x2C || pk[1] != 0x00 || pk[2] != 0x01) return false;
    }
    return true;
}

/// delta 图摘要：最大 / 最小、过正负阈值的格数、负 delta 的质心
void PrintDelta(Himax::HalDevice* dev, double tSec, std::FILE* bin) {
    std::array<uint8_t, kRows * kCols * 2> raw{};
    if (!RdBig(dev, kDelta, raw.data(), static_cast<uint32_t>(raw.size()))) {
        Out("[%7.3f] delta 读失败\n", tSec);
        return;
    }
    int mx = -32768, mn = 32767, pos = 0, neg = 0, neg300 = 0;
    for (int i = 0; i < kRows * kCols; ++i) {
        const int v = static_cast<int16_t>(raw[i * 2] | (raw[i * 2 + 1] << 8));
        if (v > mx) mx = v;
        if (v < mn) mn = v;
        if (v > 1500) ++pos;
        if (v < -1500) ++neg;
        if (v < -300) ++neg300;
    }
    uint8_t st[8]{};
    uint32_t negFlag = 0;
    (void)Rd(dev, kDeltaStats, st, 8);
    (void)RdU32(dev, kNegFlag, negFlag);
    Out("[%7.3f] delta max=%d min=%d >1500:%d <-1500:%d <-300:%d | fw过阈值=%u 正格=%u 最大=%u negFlag=0x%X\n", tSec, mx, mn,
        pos, neg, neg300, st[0] | (st[1] << 8), st[2] | (st[3] << 8), st[4] | (st[5] << 8), negFlag);
    if (bin) {
        const float t = static_cast<float>(tSec);
        std::fwrite(&t, sizeof t, 1, bin);
        std::fwrite(raw.data(), 1, raw.size(), bin);
        std::fflush(bin);
    }
}

void PrintCoords(const uint8_t* c, double tSec) {
    Out("[%7.3f] 坐标输出:", tSec);
    int n = 0;
    for (int i = 0; i < 10; ++i) {
        const uint8_t* p = c + i * 12;
        const uint32_t x = p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<uint32_t>(p[3]) << 24);
        const uint32_t y = p[4] | (p[5] << 8) | (p[6] << 16) | (static_cast<uint32_t>(p[7]) << 24);
        if (x == 0xFFFF) continue;
        Out(" {#%d x%u y%u s%u}", i, x, y, p[8]);
        ++n;
    }
    if (n == 0) Out(" (无)");
    Out("\n");
}

}  // namespace

int main(int argc, char** argv) {
    int seconds = 25;
    bool keepRaw = false;
    int forcePanel = -1;  // >= 0：强制 0x50007544（软复位会从面板侧重读覆盖，循环里持续写回）
    bool noHop = false;   // ISRAM 热补丁：freq_hop_request (0x20007874) 进门即返回，不再跳频
    bool peekIsram = false;
    bool hopGuard = false;  // ISRAM 热补丁：有格子过阈值（0x5000D124 != 0）时 freq_hop_request 直接返回
    std::string outPath;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--seconds") && i + 1 < argc) seconds = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--keep-raw")) keepRaw = true;
        else if (!std::strcmp(argv[i], "--panel7544") && i + 1 < argc) forcePanel = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--no-hop")) noHop = true;
        else if (!std::strcmp(argv[i], "--peek-isram")) peekIsram = true;
        else if (!std::strcmp(argv[i], "--hop-guard")) hopGuard = true;
        else if (!std::strcmp(argv[i], "--out") && i + 1 < argc) outPath = argv[++i];
    }
    SetConsoleOutputCP(CP_UTF8);
    timeBeginPeriod(1);  // Sleep(1) 真的睡 1 ms，否则轮询被 15.6 ms 的时钟粒度卡在约 60 Hz
    if (!outPath.empty()) g_out = std::fopen(outPath.c_str(), "w");

    Common::Logger::Init("ChipHidProbe", "C:/ProgramData/EGoTouchRev/logs/", nullptr);

    int exitCode = 1;
    {
        ServiceGuard svc;
        if (!svc.StopIfRunning()) goto done;
        Sleep(300);

        {
            Himax::Chip chip(kDevicePathMaster, kDevicePathSlave);
            if (!chip.IsReady(Himax::DeviceType::Master)) {
                Out("打开设备失败\n");
                goto done;
            }
            if (auto res = chip.Init(); !res) {
                Out("Chip::Init 失败于 %s\n", chip.LastInitFailStep());
                goto done;
            }
            Out("建链完成\n");
            Himax::HalDevice* m = chip.GetMasterDevice();

            if (peekIsram) {
                // 找 ISRAM 的主机窗口：镜像里 0x20007874 处是 FC 00 84 01，0x20000328 (_start) 处是 46 F2 00 17
                static constexpr uint32_t kCand[] = {0x08007874, 0x20007874, 0x00007874, 0x10107874, 0x28007874, 0x0C007874};
                for (int pass = 0; pass < 2; ++pass) {
                    if (pass == 1) {
                        bool on = false;
                        for (int i = 0; i < 6 && !on; ++i) on = static_cast<bool>(Himax::HimaxProtocol::safeModeSetRaw(m, true));
                        Out("── safe mode: %s ──\n", on ? "OK" : "失败");
                        Sleep(20);
                    }
                    for (const uint32_t a : kCand) {
                        uint8_t b[8]{};
                        const bool ok = Rd(m, a, b, 8);
                        Out("  [%s] 0x%08X: %s %02X %02X %02X %02X %02X %02X %02X %02X\n", pass ? "safe" : "run ", a,
                            ok ? "ok " : "ERR", b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
                    }
                }
                exitCode = 0;
                goto done;
            }

            // 让固件跑几帧，再看基线
            for (int i = 0; i < 30; ++i) (void)chip.GetFrame();
            Out("── 基线（SPI 模式）──\n");
            DumpWord(m, "if_mode", kIfMode);
            DumpWord(m, "ready", kReady);
            DumpWord(m, "stage", kStageCode);
            DumpWord(m, "sort_mode", kSortMode);
            DumpWord(m, "frame_cnt", kFrameCounter);
            DumpWord(m, "mode_state", kModeState);
            DumpWord(m, "report_state", kReportState);

            DumpWord(m, "raw_out_sel", kRawOutSel);
            if (!keepRaw) {
                Out("── 写 raw_out_sel = 0（停原始帧，对齐原厂 I²C 配置）──\n");
                if (!WrU32(m, kRawOutSel, 0)) Out("写入失败\n");
                DumpWord(m, "raw_out_sel", kRawOutSel);
            }

            // 切模式后不再读原始帧
            if (noHop) {
                // 0x20007874: push25 $s0,0 (FC 00) | movi55 $a0,1 (84 01) → push25 | pop25 $s0,0 (FC 80)
                // ISRAM 在主机 AHB 上的地址与 MCU 视角相同（0x2000xxxx），运行中可直接读写；
                // 退出 safe mode 会让 MCU 重启，所以不进 safe mode
                constexpr uint32_t kHopReqHost = 0x20007874;
                uint32_t fc0 = 0;
                RdU32(m, 0x100068D0, fc0);
                uint8_t cur[4]{};
                const bool okRd = Rd(m, kHopReqHost, cur, 4);
                Out("── ISRAM 0x%08X 原值 %02X %02X %02X %02X ──\n", kHopReqHost, cur[0], cur[1], cur[2], cur[3]);
                if (okRd && cur[0] == 0xFC && cur[1] == 0x00 && cur[2] == 0x84 && cur[3] == 0x01) {
                    const uint8_t patch[4] = {0xFC, 0x00, 0xFC, 0x80};
                    (void)Himax::HimaxProtocol::register_write(m, kHopReqHost, patch, 4);
                    uint8_t back[4]{};
                    (void)Rd(m, kHopReqHost, back, 4);
                    Out("── 写入补丁后回读 %02X %02X %02X %02X ──\n", back[0], back[1], back[2], back[3]);
                } else {
                    Out("── 原值与镜像不符，不打补丁 ──\n");
                }
                Sleep(200);
                uint32_t fc1 = 0;
                RdU32(m, 0x100068D0, fc1);
                Out("── 补丁后帧计数 0x%08X → 0x%08X（应继续增长）──\n", fc0, fc1);
            }
            if (hopGuard) {
                // 代码洞 0x20001100（镜像 0x20001046 起全 0、无引用）：
                //   push25 $s0,0 ; sethi $a0,0x5000d ; lhi $a0,[$a0+0x124] ; beqz38 $a0,+4 ; pop25 $s0,0
                //   movi55 $a0,1 ; j 0x20007878
                // 入口 0x20007874：push25 + movi55（4 B）→ j 0x20001100
                constexpr uint32_t kCave = 0x20001100, kEntry = 0x20007874;
                static constexpr uint8_t kCaveCode[20] = {0xFC, 0x00, 0x46, 0x05, 0x00, 0x0D, 0x02, 0x00, 0x00, 0x92,
                                                          0xC0, 0x02, 0xFC, 0x80, 0x84, 0x01, 0x48, 0x00, 0x33, 0xB4};
                static constexpr uint8_t kEntryCode[4] = {0x48, 0xFF, 0xCC, 0x46};
                uint8_t cave[32]{}, entry[4]{};
                const bool okCave = Rd(m, kCave, cave, 32);
                const bool okEntry = Rd(m, kEntry, entry, 4);
                bool caveZero = okCave;
                for (const uint8_t b : cave) caveZero = caveZero && b == 0;
                Out("── 代码洞 0x%08X %s；入口 %02X %02X %02X %02X ──\n", kCave, caveZero ? "全 0" : "非空", entry[0], entry[1],
                    entry[2], entry[3]);
                if (caveZero && okEntry && entry[0] == 0xFC && entry[1] == 0x00 && entry[2] == 0x84 && entry[3] == 0x01) {
                    (void)Himax::HimaxProtocol::register_write(m, kCave, kCaveCode, sizeof kCaveCode);
                    uint8_t back[20]{};
                    (void)Rd(m, kCave, back, 20);
                    if (std::memcmp(back, kCaveCode, sizeof back) == 0) {
                        (void)Himax::HimaxProtocol::register_write(m, kEntry, kEntryCode, 4);
                        (void)Rd(m, kEntry, entry, 4);
                        Out("── 补丁已写入，入口回读 %02X %02X %02X %02X ──\n", entry[0], entry[1], entry[2], entry[3]);
                    } else {
                        Out("── 代码洞回读不一致，不改入口 ──\n");
                    }
                } else {
                    Out("── 条件不满足，不打补丁 ──\n");
                }
                // neg_baseline_watchdog：有触摸时把无触摸帧计数清零（原来是直接返回、计数跨触摸累积）
                // 0x20007b88: bnez38 $a1,0x20007b98 (C9 08) → bnez38 $a1,0x20007b8e (C9 03)；同字后半 8C 01 不变
                constexpr uint32_t kWd = 0x20007b88;
                uint8_t wd[4]{};
                if (Rd(m, kWd, wd, 4) && wd[0] == 0xC9 && wd[1] == 0x08 && wd[2] == 0x8C && wd[3] == 0x01) {
                    static constexpr uint8_t kWdCode[4] = {0xC9, 0x03, 0x8C, 0x01};
                    (void)Himax::HimaxProtocol::register_write(m, kWd, kWdCode, 4);
                    (void)Rd(m, kWd, wd, 4);
                    Out("── 看门狗补丁回读 %02X %02X %02X %02X ──\n", wd[0], wd[1], wd[2], wd[3]);
                } else {
                    Out("── 看门狗原值不符 %02X %02X %02X %02X，不打补丁 ──\n", wd[0], wd[1], wd[2], wd[3]);
                }
                uint32_t fc0 = 0, fc1 = 0;
                RdU32(m, 0x100068D0, fc0);
                Sleep(200);
                RdU32(m, 0x100068D0, fc1);
                Out("── 补丁后帧计数 0x%08X → 0x%08X ──\n", fc0, fc1);
            }
            if (forcePanel >= 0) {
                Out("── 写 0x10007544 = %d ──\n", forcePanel);
                WrU32(m, 0x10007544, static_cast<uint32_t>(forcePanel));
            }
            Out("── 写 0x10007540 = 1 ──\n");
            if (!WrU32(m, kIfMode, 1)) Out("写入失败\n");
            DumpWord(m, "if_mode", kIfMode);
            Sleep(50);
            DumpWord(m, "stage", kStageCode);
            DumpWord(m, "frame_cnt", kFrameCounter);
            DumpWord(m, "report_state", kReportState);

            uint32_t blPtr = 0;
            if (RdU32(m, kBaselinePtr, blPtr)) Out("  baseline_ptr   = 0x%08X\n", blPtr);
            PrintDelta(m, 0.0, nullptr);

            struct Cue {
                double at;
                const char* text;
            };
            static constexpr Cue kCues[] = {
                {0.0, "第 1 步：先别碰屏幕"},
                {4.0, "第 2 步：十指按在屏幕上，完全不动，一直保持"},
                {20.0, "第 3 步：全部抬起，手离开屏幕"},
                {24.0, "第 4 步：十指在【刚才的位置】再按下并保持"},
                {30.0, "第 5 步：全部抬起，等待结束"},
            };
            BaselineTracker blTrack;
            auto lastBl = Clock::now() - std::chrono::seconds(1);
            size_t nextCue = 0;
            FILE* deltaBin = outPath.empty() ? nullptr : std::fopen((outPath + ".delta.bin").c_str(), "wb");

            Out("── 轮询 %d 秒，按屏幕提示操作 ──\n", seconds);
            const auto t0 = Clock::now();
            auto lastStat = t0;
            auto lastCoord = t0;
            auto lastDelta = t0;
            auto lastWatch = t0;
            uint32_t reports = 0, readErrors = 0, badReads = 0;
            std::array<uint32_t, std::size(kWatches)> watchVal{};
            std::array<bool, std::size(kWatches)> watchInit{};
            uint32_t lastStage = 0, lastActiveCnt = 0, lastIdleCnt = 0;
            std::array<uint8_t, 120> coords{};
            std::array<uint8_t, 120> lastCoords{};
            while (Clock::now() - t0 < std::chrono::seconds(seconds)) {
                const auto now = Clock::now();
                const double t = std::chrono::duration<double>(now - t0).count();

                while (nextCue < std::size(kCues) && t >= kCues[nextCue].at) {
                    Out("\n>>>>>> [%5.1fs] %s <<<<<<\n\n", t, kCues[nextCue].text);
                    Beep(880, 120);
                    ++nextCue;
                }

                uint8_t st[8]{};
                if (!Rd(m, kReportState, st, 8)) {
                    ++readErrors;
                    Out("[%7.3f] 状态读失败 lastErr=%lu\n", t, m->GetError());
                    Sleep(5);
                    continue;
                }
                if (st[0] == 0) {
                    std::array<uint8_t, 88> rep{};
                    const uint8_t packets = st[1] ? st[1] : 1;
                    if (Rd(m, kReportBuf, rep.data(), 88)) {
                        if (ReportHeaderOk(rep.data(), packets)) {
                            PrintReport(rep.data(), packets, t);
                            ++reports;
                        } else {
                            ++badReads;
                            Out("[%7.3f] 报告头异常（读毛刺）%02X %02X %02X %02X, lastErr=%lu\n", t, rep[0], rep[1], rep[2], rep[3],
                                m->GetError());
                            continue;  // 不确认，下一轮重读
                        }
                    }
                    WrU32(m, kReportState, 0x00000002);  // 确认：状态回 2，包数 / 字节数清零
                    WrU32(m, kIntLevel, 1);             // INT 拉回高电平
                }

                if (now - lastCoord >= std::chrono::milliseconds(100)) {
                    lastCoord = now;
                    if (Rd(m, kCoordOut, coords.data(), 120) && coords != lastCoords) {
                        PrintCoords(coords.data(), t);
                        lastCoords = coords;
                    }
                }
                if (now - lastBl >= std::chrono::milliseconds(500)) {
                    lastBl = now;
                    uint32_t ptr = 0;
                    if (RdU32(m, kBaselinePtr, ptr) && ptr != 0xDEADFEED) blTrack.Sample(m, t, ptr);
                }
                if (now - lastWatch >= std::chrono::milliseconds(20)) {
                    lastWatch = now;
                    if (forcePanel >= 0) {
                        uint32_t pv = 0;
                        if (RdU32(m, 0x10007544, pv) && pv != 0xDEADFEED && pv != static_cast<uint32_t>(forcePanel)) {
                            WrU32(m, 0x10007544, static_cast<uint32_t>(forcePanel));
                            Out("[%7.3f] 写回 0x10007544 = %d（被改成 0x%X）\n", t, forcePanel, pv);
                        }
                    }
                    for (size_t w = 0; w < std::size(kWatches); ++w) {
                        uint32_t v = 0;
                        if (!RdU32(m, kWatches[w].addr, v) || v == 0xDEADFEED) continue;
                        v &= kWatches[w].mask;
                        if (!watchInit[w] || v != watchVal[w]) {
                            Out("[%7.3f] WATCH %-10s 0x%08X -> 0x%08X\n", t, kWatches[w].name, watchInit[w] ? watchVal[w] : 0, v);
                            watchVal[w] = v;
                            watchInit[w] = true;
                        }
                    }
                    uint32_t stage = 0;
                    if (RdU32(m, kStageCode, stage) && stage != 0xDEADFEED && (stage & 0xFF) != (lastStage & 0xFF)) {
                        lastStage = stage;
                    }
                }
                if (now - lastDelta >= std::chrono::milliseconds(250)) {
                    // 帧计数：0x500068D2 在 0x200138D7 == 1（常规扫描）时 +1，0x500068D4 在其他状态 +1
                    uint8_t fc[8]{};
                    if (Rd(m, 0x100068D0, fc, 8) && !(fc[0] == 0xED && fc[1] == 0xFE)) {
                        const uint32_t act = fc[2] | (fc[3] << 8), idl = fc[4] | (fc[5] << 8);
                        Out("[%7.3f] FRAMES active+%u other+%u stage=0x%02X\n", t, (act - lastActiveCnt) & 0xFFFF,
                            (idl - lastIdleCnt) & 0xFFFF, lastStage & 0xFF);
                        lastActiveCnt = act;
                        lastIdleCnt = idl;
                    }
                }
                if (now - lastDelta >= std::chrono::milliseconds(250)) {
                    lastDelta = now;
                    PrintDelta(m, t, deltaBin);
                }
                if (now - lastStat >= std::chrono::seconds(2)) {
                    lastStat = now;
                    uint32_t stage = 0, fc = 0;
                    RdU32(m, kStageCode, stage);
                    RdU32(m, kFrameCounter, fc);
                    Out("[%7.3f] stage=0x%08X frame_cnt=0x%08X report_state=%02X %02X %02X %02X reports=%u readErr=%u bad=%u\n",
                        t, stage, fc, st[0], st[1], st[2], st[3], reports, readErrors, badReads);
                }
                Sleep(1);
            }

            if (deltaBin) std::fclose(deltaBin);
            Out("── 还原 ──\n");
            WrU32(m, kIfMode, 0);
            WrU32(m, kIntLevel, 1);
            WrU32(m, kRawOutSel, 0xF6);
            DumpWord(m, "if_mode", kIfMode);
            DumpWord(m, "stage", kStageCode);
            (void)chip.Deinit(true);
            Out("共收到 %u 份报告\n", reports);
            exitCode = 0;
        }
    done:;
    }
    Common::Logger::Shutdown();
    if (g_out) std::fclose(g_out);
    return exitCode;
}
