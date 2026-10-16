// ChipHidProbeB —— 方案 B 验证工具。
//
// 与 ChipHidProbe（方案 A：翻 0x10007540=1 + 轮询报告缓冲）不同，本工具验证方案 B：
// 接口模式字 0x10007540 保持 0（SPI 行为、INT 脉冲、帧校验全不变），运行时把 P1–P5 五处
// 补丁热写进 master 的 ISRAM，让芯片在 SPI 模式下也跑完整坐标管线、并把组好的 HID 触摸
// 报告拷进原始帧帧尾。主机照常 GetFrame，直接解析帧尾 back_data[kMasterSuffixOffset+0x80]
// 起的 88 B 报告，不再在用户态解算手指；slave 笔帧一行不动。
//
// 五处补丁都是 4 字节对齐的整字写，运行中原子热写、无需停机；复位即恢复原版固件，退出时
// 还会把原字节写回。依据 docs/artifacts/spi-片上解算可行性.md 第 4 节（P1–P5、P4′、帧尾布局）。
//
//   ChipHidProbeB.exe [--seconds N] [--out FILE] [--isram-base HEX] [--dry-run] [--no-patch]
//     --isram-base HEX  强制 ISRAM 的主机 AHB 窗口基址（默认自动探测，候选 0x20000000 / 0x08000000 …）
//     --dry-run         只探测窗口 + 校验锚点并打印补丁计划，不写任何字节
//     --no-patch        跳过打补丁（固件已刷过方案 B），直接解析帧尾报告

#include <windows.h>

#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <algorithm>

#include "Logger.h"
#include "FrameLayout.h"
#include "SolverTypes.h"
#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "vhf/VhfReporter.h"

namespace {

using Clock = std::chrono::steady_clock;

const std::wstring kDevicePathMaster = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
const std::wstring kDevicePathSlave = L"\\\\.\\Global\\SPBTESTTOOL_SLAVE";
const wchar_t* kServiceName = L"EGoTouchService";

// ── 帧尾报告位置：back_data 里 master 帧的帧尾 + 0x80（= 固件 0x500012C4 + 0x80），2 包 × 44 B ──
constexpr int kReportOffset = Frame::kMasterSuffixOffset + 0x80;  // 4807 + 128 = 4935
constexpr int kReportBytes = 88;

// ── ISRAM 补丁（MCU 视角地址；host 地址 = mcu - 0x20000000 + isramBase）─────────────
constexpr uint32_t kIsramMcuBase = 0x20000000;

// P5 扩展子程序 66 B（Ghidra NDS32 规格逐行汇编回验，见文档 4.2），末尾补 2 B nop16 凑成 68 B：
// AHB 写按 32 位字工作，写入长度须 4 对齐；补的 2 B 落在 pop25 之后，永不执行。
constexpr uint8_t kP5[] = {
    0xfc, 0x00, 0x46, 0x05, 0x00, 0x02, 0x58, 0x00, 0x07, 0x18, 0x49, 0xff, 0xfe, 0x29, 0x46, 0x05,
    0x00, 0x1b, 0x58, 0x00, 0x04, 0x38, 0x46, 0x15, 0x00, 0x01, 0x58, 0x10, 0x83, 0x44, 0x44, 0x20,
    0x00, 0x16, 0x0c, 0x30, 0x00, 0x01, 0x1c, 0x30, 0x80, 0x01, 0x50, 0x21, 0x7f, 0xff, 0x4e, 0x23,
    0xff, 0xfa, 0x84, 0x62, 0x46, 0x05, 0x00, 0x1b, 0x10, 0x30, 0x05, 0x14, 0x49, 0x00, 0x17, 0x31,
    0xfc, 0x80, 0x92, 0x00};

struct Patch {
    const char* name;
    uint32_t mcuAddr;
    std::vector<uint8_t> expectOrig;  // 打补丁前应读到的原字节（锚点校验，仅 4B 补丁用）
    std::vector<uint8_t> patch;       // 要写入的新字节
    std::vector<uint8_t> savedOrig;   // 运行时读到的原字节，退出时写回
};

// 自动探测用的候选 ISRAM 主机窗口基址（P1/P3/P4′ 原字节当锚点；DSRAM 是 0x1000_xxxx 不在此列）
constexpr uint32_t kBaseCandidates[] = {0x20000000, 0x08000000, 0x00000000, 0x28000000, 0x0C000000, 0x10100000};

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

bool WrVerify(Himax::HalDevice* dev, uint32_t addr, const uint8_t* buf, uint32_t len) {
    // 传完整 len 作为 verify_len：默认 verify_len=0 只比对前 2 字节，而 P4′ 新旧字节前两位都是 49 FF，会漏判。
    // 重试几次：write_and_verify 的回读可能撞上 AHB 读毛刺（0xDEADFEED）而误判失败，重写重校即可。
    for (int attempt = 0; attempt < 4; ++attempt) {
        if (Himax::HimaxProtocol::write_and_verify(dev, addr, buf, len, len)) return true;
    }
    return false;
}

// 按 4 字节字逐个写入+回校（全程非 burst、每字原子）。len 须 4 对齐。
bool WritePatch(Himax::HalDevice* dev, uint32_t host, const uint8_t* buf, uint32_t len) {
    for (uint32_t off = 0; off < len; off += 4) {
        if (!WrVerify(dev, host + off, buf + off, 4)) return false;
    }
    return true;
}

uint32_t HostAddr(uint32_t mcuAddr, uint32_t isramBase) { return mcuAddr - kIsramMcuBase + isramBase; }

std::vector<Patch> MakePatches() {
    std::vector<Patch> v;
    v.push_back({"P1", 0x20006e90, {0x5A, 0x08, 0x01, 0x05}, {0x92, 0x00, 0x92, 0x00}, {}});
    v.push_back({"P2", 0x20006f48, {0x48, 0x00, 0x00, 0x90}, {0x92, 0x00, 0x92, 0x00}, {}});
    v.push_back({"P3", 0x20008294, {0x84, 0x00, 0xF8, 0x59}, {0x84, 0x00, 0x92, 0x00}, {}});
    v.push_back({"P4'", 0x2000d234, {0x49, 0xFF, 0xF0, 0x2B}, {0x49, 0xFF, 0xD8, 0xDC}, {}});
    v.push_back({"P5", 0x200083ec, {}, std::vector<uint8_t>(kP5, kP5 + sizeof kP5), {}});
    return v;
}

Patch* ByName(std::vector<Patch>& ps, const char* name) {
    for (auto& p : ps)
        if (std::strcmp(p.name, name) == 0) return &p;
    return nullptr;
}

/// 探测 ISRAM 主机窗口：对每个候选基址，读 P1/P3/P4′ 三处原字节与期望比对，全中才算命中。
uint32_t DetectIsramBase(Himax::HalDevice* m, std::vector<Patch>& patches) {
    const char* anchorNames[] = {"P1", "P3", "P4'"};
    for (const uint32_t base : kBaseCandidates) {
        bool allOk = true;
        for (const char* nm : anchorNames) {
            Patch* a = ByName(patches, nm);
            std::array<uint8_t, 4> got{};
            const uint32_t host = HostAddr(a->mcuAddr, base);
            if (!Rd(m, host, got.data(), 4) || std::memcmp(got.data(), a->expectOrig.data(), 4) != 0) {
                allOk = false;
                break;
            }
        }
        Out("  候选基址 0x%08X: %s\n", base, allOk ? "命中（三处锚点全部匹配）" : "不匹配");
        if (allOk) return base;
    }
    return 0;
}

/// 解析并打印帧尾报告：每包 2C 00 01 n + 5 × 8 B（flag, size×3, X u16, Y u16）
/// flag = (id<<2)|3 接触 / (id<<2) 抬起；id = flag>>2（1..10）；flag==0 为空槽
void PrintReport(const uint8_t* rep, double tSec) {
    Out("[%7.3f] HID 报告 条目数 %u:", tSec, rep[3]);
    for (int p = 0; p < 2; ++p) {
        const uint8_t* pk = rep + p * 44;
        if (pk[0] != 0x2C || pk[1] != 0x00 || pk[2] != 0x01) {
            Out(" [包%d 头异常 %02X %02X %02X %02X]", p, pk[0], pk[1], pk[2], pk[3]);
            continue;
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

// 首帧允许空：仅要求第一包头是 2C 00 01（条目数可为 0）
bool ReportHeaderOk(const uint8_t* rep) { return rep[0] == 0x2C && rep[1] == 0x00 && rep[2] == 0x01; }

// 注入坐标映射参数（可由命令行调）。芯片报告 X/Y 是 u16，缩放到显示像素 2560×1600。
struct InjectCfg {
    float chipMaxX = 4096.0f;  // 芯片报告坐标满量程（描述符声明 0–4096；实测可用四角标定修正）
    float chipMaxY = 4096.0f;
    // 实测标定（本机横屏 2560×1600）：交换 X/Y + 水平翻转，方向跟手正确。--identity 可清零重标。
    bool swapXY = true;        // 交换 X/Y
    bool invertX = true;       // 水平翻转
    bool invertY = false;      // 垂直翻转
    bool transpose = false;    // 传给 VhfTouchPacket::Build（它内部 invertX=!transpose、invertY=transpose）
};
constexpr float kPixelMaxX = 2560.0f;
constexpr float kPixelMaxY = 1600.0f;

// 解析帧尾报告 → 填 frame.touch.output.contacts（供 VhfReporter::DispatchTouch 组包注入）
void ParseReportToContacts(const uint8_t* rep, Solvers::HeatmapFrame& frame, const InjectCfg& cfg) {
    auto& contacts = frame.touch.output.contacts;
    contacts.clear();
    for (int p = 0; p < 2; ++p) {
        const uint8_t* pk = rep + p * 44;
        if (pk[0] != 0x2C || pk[1] != 0x00 || pk[2] != 0x01) continue;
        for (int k = 0; k < 5; ++k) {
            const uint8_t* e = pk + 4 + k * 8;
            if (e[0] == 0) continue;  // 空槽
            Solvers::TouchContact c;
            c.id = e[0] >> 2;  // 1..10
            const bool down = (e[0] & 3) == 3;
            c.reportEvent = down ? Solvers::TouchReportDown : Solvers::TouchReportUp;
            c.isReported = true;
            const unsigned rx = e[4] | (e[5] << 8);
            const unsigned ry = e[6] | (e[7] << 8);
            float nx = std::clamp(static_cast<float>(cfg.swapXY ? ry : rx) / cfg.chipMaxX, 0.0f, 1.0f);
            float ny = std::clamp(static_cast<float>(cfg.swapXY ? rx : ry) / cfg.chipMaxY, 0.0f, 1.0f);
            if (cfg.invertX) nx = 1.0f - nx;
            if (cfg.invertY) ny = 1.0f - ny;
            c.x = nx * kPixelMaxX;
            c.y = ny * kPixelMaxY;
            contacts.push_back(c);
        }
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
            if (StartServiceW(m_service, 0, nullptr)) Out("EGoTouchService 已重新启动\n");
            else Out("重新启动 EGoTouchService 失败（错误 %lu）\n", GetLastError());
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

/// 退出时把原字节写回（P4′ 先回去激活，再其余）。复位本来也能恢复，这里只是尽早还原。
class PatchRestorer {
public:
    PatchRestorer(Himax::HalDevice* m, std::vector<Patch>& patches) : m_(m), ps_(patches) {}
    void Arm(uint32_t base) {
        base_ = base;
        armed_ = true;
    }
    void SetDevice(Himax::HalDevice* m) { m_ = m; }  // 重新 Init 后 master 句柄可能变
    ~PatchRestorer() {
        if (!armed_) return;
        Out("── 回写原字节 ──\n");
        const char* order[] = {"P4'", "P3", "P2", "P1", "P5"};
        for (const char* nm : order) {
            Patch* p = ByName(ps_, nm);
            if (!p || p->savedOrig.empty()) continue;
            const uint32_t host = HostAddr(p->mcuAddr, base_);
            const bool ok = WritePatch(m_, host, p->savedOrig.data(), static_cast<uint32_t>(p->savedOrig.size()));
            Out("  %-3s %s\n", p->name, ok ? "已回写" : "回写失败（复位可恢复）");
        }
    }

private:
    Himax::HalDevice* m_;
    std::vector<Patch>& ps_;
    uint32_t base_ = 0;
    bool armed_ = false;
};

/// 打补丁（探测窗口 → 校验原值 → 按序写入）。成功返回 true；一旦读到原值即武装 restorer，
/// 即使写入中途失败也会回写已改动的字节。
bool ApplyPatches(Himax::HalDevice* m, std::vector<Patch>& patches, uint32_t forcedBase, bool dryRun,
                  uint32_t& baseOut, bool& dryStop, PatchRestorer& restorer) {
    dryStop = false;
    Out("── 探测 ISRAM 主机窗口 ──\n");
    uint32_t base = forcedBase;
    if (base != 0) Out("  使用命令行指定基址 0x%08X\n", base);
    else base = DetectIsramBase(m, patches);
    if (base == 0) {
        Out("✗ 未能确定 ISRAM 窗口，三处锚点都没匹配上；不打补丁。\n");
        Out("  可用 --isram-base 手动指定，或先核对驱动 BusRead 的地址映射。\n");
        return false;
    }
    baseOut = base;
    Out("✓ ISRAM 窗口基址 = 0x%08X\n", base);

    Out("── 读原值并校验锚点 ──\n");
    for (auto& p : patches) {
        p.savedOrig.resize(p.patch.size());
        const uint32_t host = HostAddr(p.mcuAddr, base);
        const bool rdOk = Rd(m, host, p.savedOrig.data(), static_cast<uint32_t>(p.savedOrig.size()));
        bool origOk = rdOk;
        if (rdOk && p.expectOrig.size() == 4)  // 仅 4B 补丁有锚点可比（P5 原值是 ISR 代码，只读不比）
            origOk = std::memcmp(p.savedOrig.data(), p.expectOrig.data(), 4) == 0;
        Out("  %-3s mcu=0x%08X host=0x%08X len=%zu 原值%s\n", p.name, p.mcuAddr, host, p.patch.size(),
            !rdOk ? "<读失败>" : (origOk ? "符合" : "✗与预期不符"));
        if (!rdOk || !origOk) {
            Out("✗ %s 原值校验失败，中止打补丁。\n", p.name);
            return false;
        }
    }

    if (dryRun) {
        Out("── dry-run：只探测与校验，不写任何字节 ──\n");
        dryStop = true;
        return false;
    }

    restorer.Arm(base);  // 原值已读到；从这里起任何已改动的字节都会在退出时回写
    Out("── 写入补丁（顺序 P5→P1→P2→P3→P4′，逐 4 字节字写入+回校）──\n");
    const char* order[] = {"P5", "P1", "P2", "P3", "P4'"};
    for (const char* nm : order) {
        Patch* p = ByName(patches, nm);
        const uint32_t host = HostAddr(p->mcuAddr, base);
        const bool ok = WritePatch(m, host, p->patch.data(), static_cast<uint32_t>(p->patch.size()));
        Out("  %-3s host=0x%08X %s\n", p->name, host, ok ? "OK" : "✗ 写入/回读失败");
        if (!ok) return false;  // 已写入的部分由 restorer 回写
    }
    Out("✓ P1–P5 已写入并回读一致，方案 B 生效（if_mode 仍为 0）\n");
    return true;
}

/// 读帧循环：解析帧尾报告并打印；若 vhf 非空，则把触点经 VHF 注入系统（H1 端到端）。
void ReadLoop(Himax::Chip& chip, int seconds, VhfReporter* vhf, const InjectCfg& cfg) {
    chip.SetAutoIdleEnabled(false);  // 短测期间保持快速扫描，便于观察
    Out("── 读帧 %d 秒：听到高音后按屏幕手指%s ──\n", seconds, vhf ? "（会驱动系统光标）" : "，核对报告里的触点");
    Beep(1047, 250);  // 开始提示：现在按下手指
    Beep(1319, 250);
    const auto t0 = Clock::now();
    auto lastStat = t0;
    uint32_t frames = 0, masterReads = 0, reports = 0, badHeader = 0, sos = 0;
    std::array<uint8_t, kReportBytes> lastRep{};
    bool haveLast = false;
    Solvers::HeatmapFrame frame;  // 复用，只用到 touch.output.contacts
    while (Clock::now() - t0 < std::chrono::seconds(seconds)) {
        const auto fr = chip.GetFrame();
        const double t = std::chrono::duration<double>(Clock::now() - t0).count();
        if (fr == Himax::Chip::FrameRead::Sos) {
            ++sos;
            Out("[%7.3f] SOS: %s\n", t, chip.LastSosReason());
            continue;
        }
        if (fr == Himax::Chip::FrameRead::NoData) continue;
        ++frames;
        if (!chip.GetLastMasterWasRead()) continue;  // 本帧没读 master（2:1 交错），帧尾无新报告
        ++masterReads;

        const uint8_t* rep = chip.back_data.data() + kReportOffset;
        if (!ReportHeaderOk(rep)) {
            ++badHeader;
            if (badHeader <= 5 || badHeader % 50 == 0)
                Out("[%7.3f] 帧尾报告头异常 %02X %02X %02X %02X（扩展未生效？第 %u 次）\n", t, rep[0], rep[1], rep[2],
                    rep[3], badHeader);
            continue;
        }
        ++reports;
        if (vhf) {
            ParseReportToContacts(rep, frame, cfg);
            vhf->DispatchTouch(frame);  // 组 VHF 触摸包并写入虚拟 HID 设备
        }
        if (!haveLast || std::memcmp(rep, lastRep.data(), kReportBytes) != 0) {
            PrintReport(rep, t);
            std::memcpy(lastRep.data(), rep, kReportBytes);
            haveLast = true;
        }
        if (Clock::now() - lastStat >= std::chrono::seconds(2)) {
            lastStat = Clock::now();
            Out("[%7.3f] 统计 frames=%u masterReads=%u reports=%u badHeader=%u sos=%u inject=%s\n", t, frames,
                masterReads, reports, badHeader, sos, vhf ? (vhf->IsDeviceOpen() ? "设备已开" : "设备未开") : "否");
        }
    }
    if (vhf) vhf->FlushTouchAllUp();  // 收尾：全部抬起
    Out("── 读帧结束：frames=%u masterReads=%u reports=%u badHeader=%u sos=%u ──\n", frames, masterReads, reports,
        badHeader, sos);
    Beep(440, 500);  // 结束提示：可以松手
}

/// 读约 1.5 s，统计 master 帧里帧尾报告头是否为 2C 00 01（空触点也带这头），判断补丁在不在。
/// 不需要触屏。返回 true 表示报告存在（补丁生效）。
bool QuickCheckHeader(Himax::Chip& chip, const char* label) {
    const auto t0 = Clock::now();
    uint32_t valid = 0, bad = 0, skip = 0;
    while (Clock::now() - t0 < std::chrono::milliseconds(1500)) {
        const auto fr = chip.GetFrame();
        if (fr == Himax::Chip::FrameRead::Sos) continue;
        if (fr == Himax::Chip::FrameRead::NoData) continue;
        if (!chip.GetLastMasterWasRead()) continue;
        if (skip < 5) {  // 跳过开头几帧：刚打补丁时 0x5001b514 组装节拍未就位（首帧特例）
            ++skip;
            continue;
        }
        const uint8_t* rep = chip.back_data.data() + kReportOffset;
        if (ReportHeaderOk(rep)) ++valid;
        else ++bad;
    }
    const bool present = valid > 0 && bad == 0;
    Out("  [%s] 2C0001 头=%u 异常头=%u → %s\n", label, valid, bad,
        present ? "报告存在（补丁生效）" : (valid == 0 ? "报告消失（补丁未生效）" : "混合"));
    return present;
}

/// 第 7 步：验证 ISRAM 补丁的易失性——复位（这里用 Deinit+重新 Init 代理 sense_off/灭屏/SOS）
/// 后补丁被抹掉、必须重打。期望：①打补丁后有报告 → ②重新 Init 后报告消失 → ③重打后报告回来。
int RunReinitTest(Himax::Chip& chip, Himax::HalDevice* m, uint32_t forcedBase) {
    std::vector<Patch> patches = MakePatches();
    uint32_t base = 0;
    bool dryStop = false;
    int rc = 1;
    {
        PatchRestorer restorer(m, patches);
        if (!ApplyPatches(m, patches, forcedBase, false, base, dryStop, restorer)) {
            (void)chip.Deinit(true);
            return 1;
        }
        const bool c1 = QuickCheckHeader(chip, "① 首次打补丁后");

        Out("── Deinit + 重新 Init（模拟复位事件）──\n");
        (void)chip.Deinit(true);
        if (auto res = chip.Init(); !res) {
            Out("重新 Init 失败于 %s\n", chip.LastInitFailStep());
            return 1;
        }
        m = chip.GetMasterDevice();
        restorer.SetDevice(m);
        const bool c2 = QuickCheckHeader(chip, "② 重新 Init 后、未重打补丁");

        if (!ApplyPatches(m, patches, forcedBase, false, base, dryStop, restorer)) {
            (void)chip.Deinit(true);
            return 1;
        }
        const bool c3 = QuickCheckHeader(chip, "③ 重打补丁后");

        Out("── 第 7 步结论：%s ──\n",
            (c1 && !c2 && c3) ? "✓ 复位抹掉补丁、重打即恢复：H1 须在每次 Init 后重打（符合预期）"
            : (c1 && c2 && c3) ? "补丁在 Deinit+Init 后仍存活（该路径未复位 ISRAM；灭屏/SOS 仍需实测）"
                               : "✗ 异常，见上面三步");
    }
    (void)chip.Deinit(true);
    rc = 0;
    return rc;
}

// 读一段并按 hex + ASCII 打印（ASCII 不可见字节显示为 '.'）
void DumpRegion(Himax::HalDevice* m, const char* label, uint32_t addr, uint32_t len) {
    std::vector<uint8_t> buf(len, 0);
    bool ok = true;
    for (uint32_t off = 0; off < len && ok; off += 256) {
        const uint32_t n = std::min<uint32_t>(256, len - off);
        ok = Rd(m, addr + off, buf.data() + off, n);
    }
    Out("── %s @0x%08X (%u B) %s──\n", label, addr, len, ok ? "" : "[读失败] ");
    if (!ok) return;
    for (uint32_t i = 0; i < len; i += 16) {
        char hex[64] = {}, asc[20] = {};
        int hp = 0;
        for (uint32_t j = 0; j < 16 && i + j < len; ++j) {
            const uint8_t b = buf[i + j];
            hp += std::snprintf(hex + hp, sizeof hex - hp, "%02x ", b);
            asc[j] = (b >= 0x20 && b < 0x7f) ? static_cast<char>(b) : '.';
        }
        Out("  %04x: %-48s %s\n", i, hex, asc);
    }
}

// 读出固件版本信息：flash ROM 头（版本串/编译时间）+ DSRAM 参数块（型号/参数版本）
int RunFwInfo() {
    Himax::Chip chip(kDevicePathMaster, kDevicePathSlave);
    if (!chip.IsReady(Himax::DeviceType::Master)) {
        Out("打开设备失败\n");
        return 1;
    }
    if (auto res = chip.Init(); !res) {
        Out("Chip::Init 失败于 %s\n", chip.LastInitFailStep());
        return 1;
    }
    Himax::HalDevice* m = chip.GetMasterDevice();
    // flash 主机地址：ISRAM 实测恒等，flash 在 MCU 0x0.. 推测主机亦恒等（0x0..）
    DumpRegion(m, "flash ROM 头（版本串）", 0x00000360, 0xA0);   // 期望 ROM Vx.x / mapcode / Chk-Tl / HX83121-A
    DumpRegion(m, "flash 编译时间(BCD)", 0x000003E0, 0x10);
    DumpRegion(m, "flash 参数区头", 0x00021400, 0x40);           // 期望 A5 5A ... HX83121-A ... 210115
    DumpRegion(m, "DSRAM 参数块(cfg_load 拷贝)", 0x10007000, 0x80);
    (void)chip.Deinit(true);
    return 0;
}

int RunSession(int seconds, bool dryRun, bool noPatch, uint32_t forcedBase, bool reinitTest, bool inject,
               const InjectCfg& cfg) {
    Himax::Chip chip(kDevicePathMaster, kDevicePathSlave);
    if (!chip.IsReady(Himax::DeviceType::Master)) {
        Out("打开设备失败\n");
        return 1;
    }
    if (auto res = chip.Init(); !res) {
        Out("Chip::Init 失败于 %s\n", chip.LastInitFailStep());
        return 1;
    }
    Out("建链完成（if_mode 保持 0，不翻 I²C）\n");
    Himax::HalDevice* m = chip.GetMasterDevice();

    if (reinitTest) return RunReinitTest(chip, m, forcedBase);

    // VHF 注入器（可选）：仅在 --inject 时创建并打开，把解析出的触点写进虚拟 HID 设备。
    VhfReporter vhf;
    VhfReporter* vhfPtr = nullptr;
    if (inject) {
        vhf.SetEnabled(true);
        vhf.SetTransposeEnabled(cfg.transpose);
        vhfPtr = &vhf;
        Out("── 注入模式：解析帧尾报告 → VHF 触摸（chipMax=%.0f×%.0f swapXY=%d invX=%d invY=%d transpose=%d）──\n",
            cfg.chipMaxX, cfg.chipMaxY, cfg.swapXY ? 1 : 0, cfg.invertX ? 1 : 0, cfg.invertY ? 1 : 0,
            cfg.transpose ? 1 : 0);
        vhf.FlushTouchAllUp();  // 强制尝试打开 VHF 设备（即便此刻还没触点），以便上来就知道设备在不在
        Out("  VHF 设备：%s\n", vhf.IsDeviceOpen() ? "已打开" : "打开失败（设备不在 / 被占用 / 驱动未加载）");
    }

    std::vector<Patch> patches = MakePatches();

    if (noPatch) {
        Out("── --no-patch：假定固件已是方案 B，直接读帧尾报告 ──\n");
        ReadLoop(chip, seconds, vhfPtr, cfg);
        if (vhfPtr) vhf.Close();
        (void)chip.Deinit(true);
        return 0;
    }

    uint32_t base = 0;
    bool dryStop = false;
    bool patched = false;
    {
        // restorer 在作用域结束（ReadLoop 之后、Deinit 之前）回写原值；ApplyPatches 读到原值即武装它
        PatchRestorer restorer(m, patches);
        patched = ApplyPatches(m, patches, forcedBase, dryRun, base, dryStop, restorer);
        if (patched) ReadLoop(chip, seconds, vhfPtr, cfg);
    }
    if (vhfPtr) vhf.Close();
    (void)chip.Deinit(true);
    return (patched || dryStop) ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    int seconds = 25;
    bool dryRun = false;
    bool noPatch = false;
    bool reinitTest = false;
    bool fwInfo = false;
    bool inject = false;
    uint32_t forcedBase = 0;
    InjectCfg cfg;
    std::string outPath;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--seconds") && i + 1 < argc) seconds = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--out") && i + 1 < argc) outPath = argv[++i];
        else if (!std::strcmp(argv[i], "--isram-base") && i + 1 < argc) forcedBase = std::strtoul(argv[++i], nullptr, 16);
        else if (!std::strcmp(argv[i], "--dry-run")) dryRun = true;
        else if (!std::strcmp(argv[i], "--no-patch")) noPatch = true;
        else if (!std::strcmp(argv[i], "--reinit-test")) reinitTest = true;
        else if (!std::strcmp(argv[i], "--fwinfo")) fwInfo = true;
        else if (!std::strcmp(argv[i], "--inject")) inject = true;
        else if (!std::strcmp(argv[i], "--chip-max-x") && i + 1 < argc) cfg.chipMaxX = static_cast<float>(std::atof(argv[++i]));
        else if (!std::strcmp(argv[i], "--chip-max-y") && i + 1 < argc) cfg.chipMaxY = static_cast<float>(std::atof(argv[++i]));
        else if (!std::strcmp(argv[i], "--identity")) { cfg.swapXY = cfg.invertX = cfg.invertY = false; }
        else if (!std::strcmp(argv[i], "--swap-xy")) cfg.swapXY = true;
        else if (!std::strcmp(argv[i], "--no-swap-xy")) cfg.swapXY = false;
        else if (!std::strcmp(argv[i], "--invert-x")) cfg.invertX = true;
        else if (!std::strcmp(argv[i], "--no-invert-x")) cfg.invertX = false;
        else if (!std::strcmp(argv[i], "--invert-y")) cfg.invertY = true;
        else if (!std::strcmp(argv[i], "--transpose")) cfg.transpose = true;
    }
    SetConsoleOutputCP(CP_UTF8);
    if (!outPath.empty()) g_out = std::fopen(outPath.c_str(), "w");
    Common::Logger::Init("ChipHidProbeB", "C:/ProgramData/EGoTouchRev/logs/", nullptr);

    int exitCode = 1;
    {
        ServiceGuard svc;
        if (svc.StopIfRunning()) {
            Sleep(300);
            exitCode = fwInfo ? RunFwInfo()
                              : RunSession(seconds, dryRun, noPatch, forcedBase, reinitTest, inject, cfg);
        }
    }
    Common::Logger::Shutdown();
    if (g_out) std::fclose(g_out);
    return exitCode;
}
