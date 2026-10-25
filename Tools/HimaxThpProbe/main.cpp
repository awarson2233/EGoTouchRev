// HimaxThpProbe —— 验证总线端口（Linux 的 himax-thp + TouchBusLinux、Windows 的 SpbThpTool + TouchBusWin32）：
// 跑 Chip::Init，连续取帧，按 Chip 的帧校验规则分别检查 master / slave 两路，并打印 heatmap 峰值所在的格子
// （手指移动时应跟着走）。
//
//   HimaxThpProbe [--dev PATH] [--frames N] [--probe-only] [--raw] [--with-slave] [--dump FILE] [--poll] [--late-arm]
//
//   --dev PATH     设备节点，默认 /dev/himax_thp；Windows 上固定用 SPBTESTTOOL_MASTER / _SLAVE，忽略此项
//   --frames N     取 N 次帧后退出，默认 600；Ctrl+C 随时退出
//   --late-arm     （Windows）Init 之前把两路的读帧关掉（SET_BLOCK 0），由 Init 的读策略重新打开：
//                  SpbThpTool 默认从上电起每个 INT 边沿都读帧，Init 期间也不例外；关掉之后 Init 期间芯片吐的帧
//                  没人取，与 Linux 驱动在 INT_ENABLE 之前不读帧的时序相同
//   --probe-only   只读面板电源与总线探测，不初始化芯片
//   --with-slave   同时初始化、读取 slave（笔）。默认只用 master：本机 spi0.0 上的 slave 功能码没有应答
//   --dump FILE    每次取帧后把 master 帧原样追加到 FILE（每条记录：1 字节校验结果 + 5063 字节帧），
//                  好帧坏帧都存，便于离线对比坏帧坏在哪里
//   --raw          不经 Chip，直接对 master / slave 两路做同一组最基本的寄存器读写，对照两路的应答
//   --poll         Init 之后不等 INT：关掉两路读帧，每 4 ms 直接读一次两路的事件栈，统计帧头，
//                  看芯片到底有没有在出帧（与 INT 有没有来分开）
//
// 需要能读写设备节点（默认只有 root）。

#include "FrameLayout.h"
#include "Logger.h"
#include "himax/HimaxChip.h"
#include "himax/HimaxProtocol.h"
#include "Port/Time.h"
#include "Port/TouchBus.h"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

namespace {

volatile std::sig_atomic_t g_stop = 0;

void OnSignal(int) { g_stop = 1; }

// 与 HimaxChipFrame.cpp 的 CheckFrame 同一套规则：从第 5 字节起按 16 位字求和
enum class Check { Valid, AllZero, ChecksumError, BusFiller };

Check CheckFrame(const uint8_t* frame, uint32_t sizeBytes) {
    constexpr uint32_t kChecksumOffset = 5;
    constexpr uint8_t kBusFiller = 0xEC;
    const uint8_t* body = frame + kChecksumOffset;
    const uint32_t bodyBytes = sizeBytes - kChecksumOffset;
    uint32_t sum = 0;
    for (uint32_t i = 0; i + 1 < bodyBytes; i += 2) sum += static_cast<uint32_t>(body[i] | (body[i + 1] << 8));
    if (sum == 0) return Check::AllZero;
    if ((sum & 0xFFFFu) == 0) return Check::Valid;
    for (uint32_t i = 0; i < bodyBytes; ++i) {
        if (body[i] != kBusFiller) return Check::ChecksumError;
    }
    return Check::BusFiller;
}

struct CheckStats {
    uint64_t valid = 0, allZero = 0, checksum = 0, filler = 0;
    void Add(Check c) {
        switch (c) {
        case Check::Valid: ++valid; break;
        case Check::AllZero: ++allZero; break;
        case Check::ChecksumError: ++checksum; break;
        case Check::BusFiller: ++filler; break;
        }
    }
};

void DumpHex(const char* name, const uint8_t* p, int n) {
    std::printf("  %s:", name);
    for (int i = 0; i < n; ++i) std::printf(" %02X", p[i]);
    std::printf("\n");
}

// heatmap 里偏离中位基线最多的格子（原始值是无符号的，触摸让读数朝一个方向偏）
struct Peak {
    int row = 0, col = 0;
    int delta = 0;
};

Peak FindPeak(const uint8_t* master) {
    const uint8_t* m = master + Frame::kMatrixOffset;
    long sum = 0;
    for (int i = 0; i < Frame::kMatrixCells; ++i) sum += m[2 * i] | (m[2 * i + 1] << 8);
    const int mean = static_cast<int>(sum / Frame::kMatrixCells);
    Peak best;
    for (int i = 0; i < Frame::kMatrixCells; ++i) {
        const int v = m[2 * i] | (m[2 * i + 1] << 8);
        const int d = std::abs(v - mean);
        if (d > best.delta) best = {i / Frame::kRxCount, i % Frame::kRxCount, d};
    }
    return best;
}

void PrintStats(const char* name, const CheckStats& s) {
    std::printf("%s valid=%llu zero=%llu bad=%llu filler=%llu", name,
                static_cast<unsigned long long>(s.valid), static_cast<unsigned long long>(s.allZero),
                static_cast<unsigned long long>(s.checksum), static_cast<unsigned long long>(s.filler));
}

// ── --raw：两路对照的最小寄存器读写 ────────────────────────────────────────

int ReadReg(Port::ITouchBus& bus, uint8_t reg) {
    uint8_t v[4]{};
    if (!bus.ReadBus(reg, v, 1)) return -1;
    return v[0];
}

void PrintRegs(const char* tag, Port::ITouchBus& m, Port::ITouchBus& s) {
    for (const uint8_t reg : {0x13, 0x0D, 0x31}) {
        std::printf("  %-28s reg 0x%02X: master=%4d slave=%4d\n", tag, reg, ReadReg(m, reg), ReadReg(s, reg));
    }
}

// interface_on 的做法：假读 0x08 唤醒总线，再配连续模式 / 地址自增，读回
void InterfaceOn(const char* name, Port::ITouchBus& bus) {
    uint8_t dummy[4]{};
    const bool readOk = static_cast<bool>(bus.ReadBus(0x08, dummy, 4));
    uint8_t conti = 0x31, incr4 = 0x12;
    const bool w1 = static_cast<bool>(bus.WriteBus(0x13, nullptr, &conti, 1));
    const bool w2 = static_cast<bool>(bus.WriteBus(0x0D, nullptr, &incr4, 1));
    std::printf("  %-6s dummy 0x08 read=%d [%02X %02X %02X %02X], write 0x13/0x0D ok=%d/%d -> 0x13=%d 0x0D=%d\n",
                name, readOk ? 1 : 0, dummy[0], dummy[1], dummy[2], dummy[3], w1 ? 1 : 0, w2 ? 1 : 0,
                ReadReg(bus, 0x13), ReadReg(bus, 0x0D));
}

void ReadIcId(const char* name, Port::ITouchBus& bus) {
    uint8_t id[4]{};
    const auto res = Himax::HimaxProtocol::register_read(&bus, 0x900000D0, id, 4);
    std::printf("  %-6s IC ID @0x900000D0: ok=%d %02X %02X %02X %02X\n", name, res ? 1 : 0, id[0], id[1], id[2], id[3]);
}

void ReadEventStack(const char* name, Port::ITouchBus& bus) {
    uint8_t buf[16]{};
    const bool ok = static_cast<bool>(bus.ReadBus(0x30, buf, sizeof(buf)));
    std::printf("  %-6s event stack 0x30 ok=%d:", name, ok ? 1 : 0);
    for (const uint8_t b : buf) std::printf(" %02X", b);
    std::printf("\n");
}

int RunRaw(const std::filesystem::path& masterPath, const std::filesystem::path& slavePath) {
    auto m = Port::CreateTouchBus(masterPath, Port::TouchChannel::Master);
    auto s = Port::CreateTouchBus(slavePath, Port::TouchChannel::Slave);
    if (!m->IsValid() || !s->IsValid()) {
        std::fprintf(stderr, "cannot open %s (err %u / %u)\n", masterPath.string().c_str(), m->GetError(), s->GetError());
        return 1;
    }
    for (int round = 0; round < 2; ++round) {
        if (round == 1) {
            std::printf("[hw reset via master, wait 100 ms]\n");
            (void)m->SetReset(false);
            Port::SleepMs(5);
            (void)m->SetReset(true);
            Port::SleepMs(100);
        }
        std::printf("[round %d]\n", round);
        PrintRegs("before interface_on", *m, *s);
        InterfaceOn("master", *m);
        InterfaceOn("slave", *s);
        ReadIcId("master", *m);
        ReadIcId("slave", *s);
        ReadEventStack("master", *m);
        ReadEventStack("slave", *s);
    }
    return 0;
}

// ── --poll：Init 之后绕开 INT 直接轮询事件栈 ─────────────────────────────────

int RunPoll(const std::filesystem::path& masterPath, const std::filesystem::path& slavePath, long rounds) {
    auto m = Port::CreateTouchBus(masterPath, Port::TouchChannel::Master);
    auto s = Port::CreateTouchBus(slavePath, Port::TouchChannel::Slave);
    if (!m->IsValid() || !s->IsValid()) return 1;
    // 驱动里的读帧会和这里抢事件栈。SpbThpTool 的 INT_CLOSE 什么也不做，要靠 SET_BLOCK 0 才能让它的 ISR 不读帧
    (void)m->IntClose();
    (void)s->IntClose();
    (void)m->SetBlock(false);
    (void)s->SetBlock(false);

    struct Seen {
        uint64_t total = 0, hdr = 0, zero = 0, other = 0, changed = 0;
        uint64_t lastHash = 0;
        uint8_t firstHdr[8]{};
        bool printed = false;
    };
    auto classify = [](Seen& st, const uint8_t* d, size_t n) {
        ++st.total;
        if (d[0] == 0x5A && d[1] == 0xA5) {
            ++st.hdr;
            // 帧内容和上一个带帧头的读数不同 = 芯片确实在扫描出新帧，而不是同一帧被反复读到
            uint64_t h = 1469598103934665603ull;
            for (size_t i = 0; i < n; ++i) h = (h ^ d[i]) * 1099511628211ull;
            if (h != st.lastHash) ++st.changed;
            st.lastHash = h;
            if (!st.printed) {
                std::memcpy(st.firstHdr, d, sizeof(st.firstHdr));
                st.printed = true;
            }
        } else if (std::all_of(d, d + n, [](uint8_t b) { return b == 0; })) {
            ++st.zero;
        } else {
            ++st.other;
        }
    };
    // 两颗芯片的固件状态与刚写的几个寄存器，对照 Slave 的固件是否在跑
    for (const uint32_t addr : {0x900000A8u, 0x100072ECu, 0x10000000u, 0x900000D0u}) {
        uint8_t mv[4]{}, sv[4]{};
        const bool mo = static_cast<bool>(Himax::HimaxProtocol::register_read(m.get(), addr, mv, 4));
        const bool so = static_cast<bool>(Himax::HimaxProtocol::register_read(s.get(), addr, sv, 4));
        std::printf("reg 0x%08X: master%s %02X %02X %02X %02X | slave%s %02X %02X %02X %02X\n", addr, mo ? "" : "(err)",
                    mv[0], mv[1], mv[2], mv[3], so ? "" : "(err)", sv[0], sv[1], sv[2], sv[3]);
    }
    static uint8_t mb[Frame::kMasterFrameSize], sb[Frame::kSlaveFrameSize];
    Seen ms, ss;
    for (long i = 0; i < rounds && !g_stop; ++i) {
        if (s->ReadBus(0x30, sb, Frame::kSlaveFrameSize - 3)) classify(ss, sb, Frame::kSlaveFrameSize - 3);
        if (m->ReadBus(0x30, mb, Frame::kMasterFrameSize - 3)) classify(ms, mb, Frame::kMasterFrameSize - 3);
        Port::SleepMs(4);
    }
    // 最后一次读到的 slave 帧（总线头之后的数据）整帧打出来，看校验不过是长度、格式还是内容的问题
    std::printf("last slave frame (%d bytes after bus header):\n", Frame::kSlaveFrameSize - 3);
    for (int i = 0; i < Frame::kSlaveFrameSize - 3; ++i) std::printf("%02X%s", sb[i], (i % 32 == 31) ? "\n" : " ");
    std::printf("\n");
    for (auto [name, st] : {std::pair{"master", &ms}, std::pair{"slave", &ss}}) {
        std::printf("poll %-6s reads=%llu 5AA5=%llu (new %llu) zero=%llu other=%llu first hdr:", name,
                    static_cast<unsigned long long>(st->total), static_cast<unsigned long long>(st->hdr),
                    static_cast<unsigned long long>(st->changed),
                    static_cast<unsigned long long>(st->zero), static_cast<unsigned long long>(st->other));
        for (const uint8_t b : st->firstHdr) std::printf(" %02X", b);
        std::printf("\n");
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    std::string dev = "/dev/himax_thp";
    long frames = 600;
    bool probeOnly = false;
    bool raw = false;
    bool withSlave = false;
    bool poll = false;
    bool lateArm = false;
    std::string dumpPath;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--dev" && i + 1 < argc) dev = argv[++i];
        else if (a == "--frames" && i + 1 < argc) frames = std::strtol(argv[++i], nullptr, 10);
        else if (a == "--probe-only") probeOnly = true;
        else if (a == "--raw") raw = true;
        else if (a == "--with-slave") withSlave = true;
        else if (a == "--poll") poll = true;
        else if (a == "--late-arm") lateArm = true;
        else if (a == "--dump" && i + 1 < argc) dumpPath = argv[++i];
        else {
            std::fprintf(stderr, "usage: %s [--dev PATH] [--frames N] [--probe-only] [--raw] [--with-slave] [--dump FILE] [--poll] [--late-arm]\n", argv[0]);
            return 2;
        }
    }
#ifdef _WIN32
    const std::filesystem::path masterPath = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
    const std::filesystem::path slavePath = L"\\\\.\\Global\\SPBTESTTOOL_SLAVE";
#else
    const std::filesystem::path masterPath = dev;
    const std::filesystem::path slavePath = dev;
#endif

    if (raw) return RunRaw(masterPath, slavePath);

    std::signal(SIGINT, OnSignal);
    std::signal(SIGTERM, OnSignal);
    Common::Logger::Init("HimaxThpProbe", std::filesystem::temp_directory_path() / "egotouch-logs", nullptr);

    if (lateArm) {
        // SET_BLOCK 是驱动里整个设备的开关，另开的句柄关掉，Chip 自己的句柄同样生效
        for (const auto& [path, ch] : {std::pair{masterPath, Port::TouchChannel::Master}, std::pair{slavePath, Port::TouchChannel::Slave}}) {
            auto bus = Port::CreateTouchBus(path, ch);
            const bool ok = bus->IsValid() && static_cast<bool>(bus->SetBlock(false));
            std::printf("late-arm: %s SET_BLOCK 0 %s\n", ch == Port::TouchChannel::Master ? "master" : "slave", ok ? "ok" : "FAILED");
        }
    }

    Himax::Chip chip(masterPath, slavePath);
    chip.SetSlaveEnabled(withSlave);
    if (!chip.IsReady(Himax::DeviceType::Master) || !chip.IsReady(Himax::DeviceType::Slave)) {
        std::fprintf(stderr, "cannot open %s (need the bus driver bound, root / the service stopped)\n", masterPath.string().c_str());
        Common::Logger::Shutdown();
        return 1;
    }

    const auto panel = chip.ReadPanelPower();
    std::printf("panel power: %s\n", panel ? (*panel ? "on" : "off") : "unreadable");
    const auto probe = chip.ProbeBus();
    std::printf("bus probe: io=%d master(0x13)=0x%02X slave(0x13)=0x%02X err=%u -> %s\n", probe.ioOk ? 1 : 0,
                probe.master, probe.slave, probe.nativeError, probe.Ready() ? "ready" : "NOT ready");
    if (probeOnly) {
        Common::Logger::Shutdown();
        return probe.Ready() ? 0 : 1;
    }

    std::printf("initializing chip...\n");
    if (auto res = chip.Init(); !res) {
        std::fprintf(stderr, "Init failed at '%s': %s\n", chip.LastInitFailStep(), ToString(res.error()));
        Common::Logger::Shutdown();
        return 1;
    }
    // 只看取帧，不让 Chip 自己在静止时把 IC 切进 IDLE
    chip.SetAutoIdleEnabled(false);
    if (poll) {
        const int rc = RunPoll(masterPath, slavePath, frames);
        (void)chip.Deinit(true);
        Common::Logger::Shutdown();
        return rc;
    }
    std::printf("init ok, reading %ld frames (touch the screen; Ctrl+C to stop)\n", frames);

    std::FILE* dump = dumpPath.empty() ? nullptr : std::fopen(dumpPath.c_str(), "wb");
    if (!dumpPath.empty() && !dump) std::fprintf(stderr, "cannot write %s\n", dumpPath.c_str());
    // 记录一帧：Chip 判定为无效的帧（NoData）也还留在 back_data 里
    auto dumpFrame = [&](const uint8_t* m) {
        if (!dump) return;
        const uint8_t check = static_cast<uint8_t>(CheckFrame(m, Frame::kMasterFrameSize));
        std::fwrite(&check, 1, 1, dump);
        std::fwrite(m, 1, Frame::kMasterFrameSize, dump);
    };

    CheckStats master, slave;
    uint64_t gotFrame = 0, noData = 0;
    bool dumped = false;
    using clock = std::chrono::steady_clock;
    auto windowStart = clock::now();
    uint64_t windowFrames = 0;
    int rc = 0;

    for (long n = 0; n < frames && !g_stop; ++n) {
        const auto r = chip.GetFrame();
        if (r == Himax::Chip::FrameRead::Sos) {
            std::fprintf(stderr, "SOS: %s\n", chip.LastSosReason());
            rc = 1;
            break;
        }
        if (r == Himax::Chip::FrameRead::NoData) {
            ++noData;
            dumpFrame(chip.GetFrameBuffer().data());
            continue;
        }
        ++gotFrame;
        ++windowFrames;

        const auto& buf = chip.GetFrameBuffer();
        const uint8_t* m = buf.data();
        const uint8_t* s = buf.data() + Frame::kSlaveHeaderOffset;
        const bool masterRead = chip.GetLastMasterWasRead();
        if (masterRead) dumpFrame(m);
        if (masterRead) master.Add(CheckFrame(m, Frame::kMasterFrameSize));
        if (withSlave) slave.Add(CheckFrame(s, Frame::kSlaveFrameSize));

        if (!dumped && masterRead) {
            dumped = true;
            std::printf("first frame:\n");
            DumpHex("master[0..15]", m, 16);
            DumpHex("slave [0..15]", s, 16);
        }

        const auto now = clock::now();
        if (now - windowStart >= std::chrono::seconds(1)) {
            const double secs = std::chrono::duration<double>(now - windowStart).count();
            const Peak p = FindPeak(m);
            std::printf("%5.1f fps | ", windowFrames / secs);
            PrintStats("master", master);
            std::printf(" | ");
            PrintStats("slave", slave);
            std::printf(" | peak tx=%2d rx=%2d delta=%d\n", p.row, p.col, p.delta);
            std::fflush(stdout);
            windowStart = now;
            windowFrames = 0;
        }
    }

    std::printf("done: frames=%llu nodata=%llu | ", static_cast<unsigned long long>(gotFrame),
                static_cast<unsigned long long>(noData));
    PrintStats("master", master);
    std::printf(" | ");
    PrintStats("slave", slave);
    std::printf("\n");

    if (dump) std::fclose(dump);
    (void)chip.Deinit(true);
    Common::Logger::Shutdown();
    return rc;
}
