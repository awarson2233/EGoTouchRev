// TsaRawReplay —— 把一段 .dvrbin 录制灌进 TsaCoreSolver（反编译的 TSACore 手指管线），
// 逐帧打印 frame.touch.output.contacts（TSACore 是进程级单例，A/B 只能分进程跑）。
//
//   TsaRawReplay.exe <file.dvrbin> [--panel ID] [--to N] [--dump N] [--charger] [--quiet]
//                    [--afe] [--sos N] [--hold N K]
//                    [--pen X Y A B] [--pen-hover] [--pen-tilt TX TY] [--pen-word W] [--pen-interleave]
//                    [--stylus-solver] [--state-digest FILE] [--state-dump STEP FILE] [--profile]
//   TsaRawReplay.exe --state-diff A B [SYMBOL]
//
//   --panel ID     面板机型（W273AS2700 / 2710 / 1300 / 1310），默认取录制里记的；旧录制没记就是 W273AS2700
//   --afe          把 TSACore 的闭环请求与要下发的 AFE 动作打到 stderr（请求只在变化时打印）
//   --sos N        在第 N 帧之前注入一次 AFE SOS 之后的复位（宿主重启芯片）
//   --hold N K     把第 N 帧额外重复喂 K 次（模拟长时间静止）
//
// 判断两份实现是否等价（重构前后各跑一遍，见 Compare-Reference.ps1）：
//   --state-digest FILE     每喂一帧，把 TSACore 全部算法状态（.bss + 堆缓冲，指针已规范化）的摘要写一行
//   --state-dump STEP FILE  把第 STEP 次喂帧之后的完整状态写成快照文件
//   --state-diff A B [SYM]  比较两份快照，按符号名列出不同的区间（给了 SYM 就只列名字含它的）；有差异时退出码为 1
//   --heap-skew N           自检用：先分配 N 个大小不一的堆块再建解算器，摘要不应因此改变
//
// 性能：
//   --profile      结束时把 TSACore 各步骤的累计耗时打到 stderr（PROF 纳秒 次数 步骤），按耗时降序；第一行是
//                  TsaCoreSolver::Process 整体（含适配层与宿主）。各步骤要求算法库以 EGO_TSACORE_PROFILE=ON 构建，
//                  否则没有数据（Compare-Reference -Benchmark 会这样构建）
//   --profile-frame  同上但只计整帧：各步骤不计时，整帧耗时里没有分段计时自身的开销
//
// 录制里没有笔的时候，可以注入一支模拟笔来走通手指侧依赖笔状态的逻辑（只对 TsaCoreSolver 生效）：
//   --pen X Y A B      第 A..B 帧（闭区间）笔尖落在 (X, Y)，单位与输出 CSV 相同（显示像素）。
//                      这些帧的 slave 段改写成「有笔信号」，stylus.output 填成书写中
//   --pen-hover        注入的笔只悬停，不落笔
//   --pen-tilt TX TY   注入的倾角（度，StylusSolver 的方向），默认 0 0
//   --pen-word W       笔状态字（十六进制），默认 0x0002 = 已连接；0x0102 再打开防误触模式
//   --pen-interleave   注入区间内的奇数帧标成「驱动跳过了 master」
//   注入时 stderr 会在笔模式 / 笔在场变化的帧打一行 PEN。
//
// 录制里有真实笔数据时：
//   --stylus-solver    像 DeviceRuntime 一样先跑 StylusPipeline（用录制里的蓝牙压力采样），再把它的输出
//                      交给 TsaCoreSolver。每个有笔信号的帧在 stderr 打一行 STY：笔帧状态、送进 TSACore 的
//                      笔报告，以及按驱动方式展开的 TX1 整屏矩阵的峰值位置（换成像素，用来核对坐标方向）
//
// 依赖 DVR 读取器回填原始 SPI 帧，只在 Debug 配置下可用。没有保存原始帧的旧录制（rawDataLength = 0）
// 会用 suffix 重新拼一帧：矩阵填驱动的零点，所以只有笔的那一半是真实数据。

#include "DvrBinaryIO.h"
#include "FrameLayout.h"
#include "SolverTypes.h"
#include "StateTrace.h"
#include "StylusSolver/StylusPipeline.h"
#include "TsaCore/TsaCore.h"
#include "TsaCore/TsaCoreSolver.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace {

const char* EventLabel(int reportEvent) {
    switch (reportEvent) {
    case Solvers::TouchReportDown: return "down";
    case Solvers::TouchReportMove: return "move";
    case Solvers::TouchReportUp:   return "up";
    default:                       return "idle";
    }
}

const char* ActionLabel(Solvers::Tsa::AfeActionType type) {
    using T = Solvers::Tsa::AfeActionType;
    switch (type) {
    case T::ClearStatus:      return "ClearStatus";
    case T::ForceToFreqPoint: return "ForceToFreqPoint";
    case T::ForceToScanRate:  return "ForceToScanRate";
    case T::EnterIdle:        return "EnterIdle";
    case T::StartCalibration: return "StartCalibration";
    }
    return "?";
}

// 把一帧的 slave 段改写成「有笔信号」：两个 9×9 块的中心放到给定节点，再调整校验字。
// 网格内容不动（桥接模式下 TSACore 只看笔帧状态，坐标来自 stylus.output）。
// 帧自带原始缓冲只在诊断构建里有；发布构建的读取器本来就不保留原始帧。
void MarkStylusSignal(Solvers::HeatmapFrame& f, int row, int col) {
#if EGOTOUCH_DIAG
    if (f.rawData.size() < static_cast<size_t>(Frame::kTotalFrameSize)) return;
    uint8_t* slave = f.rawData.data() + Frame::kSlaveHeaderOffset;
    for (const int word : {0, Frame::kBlockWords}) {
        uint8_t* p = slave + Frame::kHeaderBytes + word * 2;
        p[0] = static_cast<uint8_t>(row);
        p[1] = 0;
        p[2] = static_cast<uint8_t>(col);
        p[3] = 0;
    }
    // 驱动的校验：slave + 5 起的 167 个字求和，低 16 位为 0。第 0 个字是头部的最后两个字节。
    slave[5] = slave[6] = 0;
    uint32_t sum = 0;
    for (int i = 0; i < (Frame::kSlaveFrameSize - 5) / 2; ++i) {
        sum += static_cast<uint32_t>(slave[5 + i * 2] | (slave[6 + i * 2] << 8));
    }
    const uint16_t fix = static_cast<uint16_t>(0x10000u - (sum & 0xffffu));
    slave[5] = static_cast<uint8_t>(fix & 0xff);
    slave[6] = static_cast<uint8_t>(fix >> 8);
#else
    (void)f;
    (void)row;
    (void)col;
#endif
}

// 录制没有保存原始 SPI 帧时，用 master / slave suffix 拼一帧：矩阵填 0x8000（驱动增益阶段的零点，
// 相当于没有手指），两个帧头按驱动的校验补齐。返回是否拼了。
bool RebuildRawFrame(Solvers::HeatmapFrame& f) {
#if EGOTOUCH_DIAG
    if (f.rawLen != 0 || !f.masterSuffixValid || !f.slaveSuffixValid) return false;
    f.rawData.assign(static_cast<size_t>(Frame::kTotalFrameSize), 0);
    uint8_t* raw = f.rawData.data();
    const auto putWords = [](uint8_t* dst, const uint16_t* words, int count) {
        for (int i = 0; i < count; ++i) {
            dst[i * 2] = static_cast<uint8_t>(words[i] & 0xff);
            dst[i * 2 + 1] = static_cast<uint8_t>(words[i] >> 8);
        }
    };
    // 校验：帧起始 + 5 起按小端字求和，低 16 位为 0。补偿字放在头部的最后两个字节。
    const auto fixChecksum = [](uint8_t* frame, int frameSize) {
        frame[5] = frame[6] = 0;
        uint32_t sum = 0;
        for (int i = 0; i < (frameSize - 5) / 2; ++i) {
            sum += static_cast<uint32_t>(frame[5 + i * 2] | (frame[6 + i * 2] << 8));
        }
        const uint16_t fix = static_cast<uint16_t>(0x10000u - (sum & 0xffffu));
        frame[5] = static_cast<uint8_t>(fix & 0xff);
        frame[6] = static_cast<uint8_t>(fix >> 8);
    };
    for (uint8_t* head : {raw, raw + Frame::kSlaveHeaderOffset}) {
        head[2] = 0x5a;
        head[3] = 0x5a;
        head[4] = 0xa5;
    }
    for (int i = 0; i < Frame::kMatrixCells; ++i) {
        raw[Frame::kMatrixOffset + i * 2] = 0x00;
        raw[Frame::kMatrixOffset + i * 2 + 1] = 0x80;
    }
    putWords(raw + Frame::kMasterSuffixOffset, f.masterSuffix.words, Frame::kMasterSuffixWords);
    putWords(raw + Frame::kSlaveSuffixOffset, f.slaveSuffix.words, Frame::kSlaveSuffixWords);
    fixChecksum(raw, Frame::kMasterFrameSize);
    fixChecksum(raw + Frame::kSlaveHeaderOffset, Frame::kSlaveFrameSize);
    f.rawPtr = raw;
    f.rawLen = f.rawData.size();
    return true;
#else
    (void)f;
    return false;
#endif
}

// TX1 整屏矩阵（驱动倒序后的方向，与 TSACore 的触点同向）的峰值格子，以及峰值 3×3 邻域的
// 重心（格子下标，带小数），用来和笔报告比对方向与半格约定。
struct StylusPeak {
    int row = 0;
    int col = 0;
    int value = 0;
    double centerRow = 0.0;
    double centerCol = 0.0;
};

bool FindStylusPeak(const uint16_t* matrix, StylusPeak& peak) {
    if (matrix == nullptr) return false;
    for (int i = 0; i < Frame::kMatrixCells; ++i) {
        const int v = static_cast<int16_t>(matrix[i]);
        if (v > peak.value) {
            peak.value = v;
            peak.row = i / Frame::kRxCount;
            peak.col = i % Frame::kRxCount;
        }
    }
    if (peak.value <= 0) return false;
    double sum = 0.0, sumRow = 0.0, sumCol = 0.0;
    for (int r = peak.row - 1; r <= peak.row + 1; ++r) {
        for (int q = peak.col - 1; q <= peak.col + 1; ++q) {
            if (r < 0 || r >= Frame::kTxCount || q < 0 || q >= Frame::kRxCount) continue;
            const int v = static_cast<int16_t>(matrix[r * Frame::kRxCount + q]);
            if (v <= 0) continue;
            sum += v;
            sumRow += static_cast<double>(v) * r;
            sumCol += static_cast<double>(v) * q;
        }
    }
    peak.centerRow = sumRow / sum;
    peak.centerCol = sumCol / sum;
    return true;
}

bool SameRequests(const Solvers::Tsa::AfeRequests& a, const Solvers::Tsa::AfeRequests& b) {
    return a.scanFreqIndex == b.scanFreqIndex && a.scanRateIndex == b.scanRateIndex &&
           a.okToEnterIdle == b.okToEnterIdle && a.reCalibNeeded == b.reCalibNeeded;
}

} // namespace

// solverNs / solverCalls：TsaCoreSolver::Process 整体（适配层、宿主与 TSACore）的耗时，由回放自己计
void PrintProfile(uint64_t solverNs, uint64_t solverCalls) {
    std::fprintf(stderr, "PROF %12llu %8llu TsaCoreSolver::Process\n", static_cast<unsigned long long>(solverNs),
                 static_cast<unsigned long long>(solverCalls));
    std::vector<Solvers::Tsa::Core::ProfileEntry> entries(Solvers::Tsa::Core::ProfileEntries(nullptr, 0));
    entries.resize(std::min(entries.size(), Solvers::Tsa::Core::ProfileEntries(entries.data(), entries.size())));
    if (entries.empty()) {
        std::fprintf(stderr, "no profile data: build with -DEGO_TSACORE_PROFILE=ON\n");
        return;
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.ns > b.ns; });
    for (const auto& e : entries) {
        std::fprintf(stderr, "PROF %12llu %8llu %s\n", static_cast<unsigned long long>(e.ns),
                     static_cast<unsigned long long>(e.count), e.name);
    }
}

int main(int argc, char** argv) {
    if ((argc == 4 || argc == 5) && std::strcmp(argv[1], "--state-diff") == 0) {
        TsaReplay::StateSnapshot a, b;
        if (!a.Read(argv[2]) || !b.Read(argv[3])) {
            std::fprintf(stderr, "cannot read snapshot\n");
            return 2;
        }
        return TsaReplay::DiffSnapshots(a, b, TSA_RAW_ROOT, argc == 5 ? argv[4] : "") == 0 ? 0 : 1;
    }
    if (argc < 2) {
        std::fprintf(stderr, "usage: TsaRawReplay <file.dvrbin> [--panel ID] [--to N] [--dump N] [--charger] [--quiet] "
                             "[--afe] [--sos N] [--hold N K] [--pen X Y A B] [--pen-hover] "
                             "[--pen-tilt TX TY] [--pen-word W] [--pen-interleave] [--stylus-solver] "
                             "[--state-digest FILE] [--state-dump STEP FILE] [--profile | --profile-frame]\n"
                             "       TsaRawReplay --state-diff A B [SYMBOL]\n");
        return 2;
    }
    int to = -1;
    int dump = -1;   // 该帧处理后把 TSACore 的 diff 网格（40 行 x 60 列）打到 stderr
    int sosFrame = -1;
    int holdFrame = -1;
    int holdCount = 0;
    bool charger = false;
    bool quiet = false;
    bool profile = false;
    bool afe = false;
    bool pen = false;
    bool penHover = false;
    bool penInterleave = false;
    bool stylusSolver = false;
    float penX = 0.0f, penY = 0.0f;
    int penFrom = 0, penTo = -1;
    int penTiltX = 0, penTiltY = 0;
    unsigned penWord = 0x0002;
    const char* digestPath = nullptr;
    const char* dumpPath = nullptr;
    int dumpStep = -1;
    int heapSkew = 0;
    const char* panelArg = nullptr;
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--to") == 0 && i + 1 < argc) to = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--panel") == 0 && i + 1 < argc) panelArg = argv[++i];
        else if (std::strcmp(argv[i], "--dump") == 0 && i + 1 < argc) dump = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--sos") == 0 && i + 1 < argc) sosFrame = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--hold") == 0 && i + 2 < argc) {
            holdFrame = std::atoi(argv[++i]);
            holdCount = std::atoi(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--charger") == 0) charger = true;
        else if (std::strcmp(argv[i], "--quiet") == 0) quiet = true;
        else if (std::strcmp(argv[i], "--profile") == 0) profile = true;
        else if (std::strcmp(argv[i], "--profile-frame") == 0) {
            profile = true;
            Solvers::Tsa::Core::ProfileSteps(false);
        }
        else if (std::strcmp(argv[i], "--afe") == 0) afe = true;
        else if (std::strcmp(argv[i], "--pen") == 0 && i + 4 < argc) {
            pen = true;
            penX = static_cast<float>(std::atof(argv[++i]));
            penY = static_cast<float>(std::atof(argv[++i]));
            penFrom = std::atoi(argv[++i]);
            penTo = std::atoi(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--pen-hover") == 0) penHover = true;
        else if (std::strcmp(argv[i], "--pen-interleave") == 0) penInterleave = true;
        else if (std::strcmp(argv[i], "--stylus-solver") == 0) stylusSolver = true;
        else if (std::strcmp(argv[i], "--pen-tilt") == 0 && i + 2 < argc) {
            penTiltX = std::atoi(argv[++i]);
            penTiltY = std::atoi(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--state-digest") == 0 && i + 1 < argc) digestPath = argv[++i];
        else if (std::strcmp(argv[i], "--heap-skew") == 0 && i + 1 < argc) heapSkew = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--state-dump") == 0 && i + 2 < argc) {
            dumpStep = std::atoi(argv[++i]);
            dumpPath = argv[++i];
        }
        else if (std::strcmp(argv[i], "--pen-word") == 0 && i + 1 < argc) {
            penWord = static_cast<unsigned>(std::strtoul(argv[++i], nullptr, 16));
        }
    }

    std::vector<Solvers::HeatmapFrame> frames;
    int version = 0;
    std::string err;
    Dvr::RuntimeConfigSnapshot config;
    if (!Dvr::ReadBinaryFile(argv[1], frames, version, nullptr, &err, nullptr, nullptr, &config)) {
        std::fprintf(stderr, "read failed: %s\n", err.c_str());
        return 1;
    }
    if (to < 0 || to > static_cast<int>(frames.size())) to = static_cast<int>(frames.size());
    const std::string recordedPanel = config.PanelProjectId();
    const std::string panel(Solvers::Tsa::ResolvePanelProjectId(panelArg != nullptr ? panelArg : recordedPanel));
    std::fprintf(stderr, "loaded %zu frames (dvr v%d), solver=%s, panel=%s (recorded: %s)\n", frames.size(), version,
                 "TsaCoreSolver", panel.c_str(), recordedPanel.empty() ? "none" : recordedPanel.c_str());

    // 比对工具自检：先占掉一些大小不一的堆块，让后面所有对象换个地址。状态摘要不应随之改变
    std::vector<std::unique_ptr<char[]>> skew;
    for (int i = 0; i < heapSkew; ++i) skew.push_back(std::make_unique<char[]>(static_cast<size_t>(64 + i * 37 % 4096)));

    // 解算器带大块状态，放堆上
    auto tsa = std::make_unique<Solvers::Tsa::TsaCoreSolver>();
    auto stylus = std::make_unique<Solvers::StylusPipeline>();
    uint32_t btSeqSeen = 0;
    tsa->SetPanelProjectId(panel);
    stylus->SetPanelProjectId(panel);
    tsa->SetChargerPlugged(charger);
    if (pen || stylusSolver) {
        tsa->SetStylusConnected(true);
        tsa->SetPenStatusWord(static_cast<uint16_t>(penWord));
    }
    bool penMode = false, penInRange = false;

    std::FILE* digestFile = nullptr;
    if (digestPath != nullptr) {
        digestFile = std::fopen(digestPath, "w");
        if (digestFile == nullptr) {
            std::fprintf(stderr, "cannot write %s\n", digestPath);
            return 1;
        }
        // 规范化方式变了就要换版本号：两边的摘要只有同一版本才能比
        std::fprintf(digestFile, "# tsa-state %d: step frame digest blocks bytes orphans\n", TsaReplay::kStateFormatVersion);
    }
    TsaReplay::StateSnapshot snapshot;
    // 喂完一帧之后的状态；step = -1 是喂第一帧之前
    const auto traceState = [&](int traceStep, int frameIndex) {
        if (digestFile == nullptr && dumpPath == nullptr) return true;
        if (!snapshot.Capture()) {
            std::fprintf(stderr, "state capture failed at step %d\n", traceStep);
            return false;
        }
        if (digestFile != nullptr) {
            std::fprintf(digestFile, "%d %d %016llx %zu %zu %d\n", traceStep, frameIndex,
                         static_cast<unsigned long long>(snapshot.Digest()), snapshot.blocks.size(), snapshot.TotalBytes(),
                         snapshot.orphanHeapBlocks);
        }
        if (dumpPath != nullptr && traceStep == dumpStep && !snapshot.Write(dumpPath)) {
            std::fprintf(stderr, "cannot write %s\n", dumpPath);
            return false;
        }
        return true;
    };
    if (!traceState(-1, -1)) return 1;

    int fed = 0, withTouch = 0, dispatched = 0;
    int step = 0;   // 喂进解算器的帧序号，--hold 重复的帧也计数
    uint64_t holdShiftMs = 0;
    uint64_t solverNs = 0, solverCalls = 0;
    size_t maxTouch = 0;
    unsigned statusSeen = 0;
    Solvers::Tsa::AfeRequests lastReq{};
    if (!quiet) std::printf("frame,num,id,event,x,y\n");

    int recSlaveOnly = 0, recPenInRange = 0;
    for (const auto& sf : frames) {
        if (!sf.masterWasRead) ++recSlaveOnly;
        if (sf.stylus.output.inRange) ++recPenInRange;
    }
    int rebuilt = 0;
    for (auto& sf : frames) {
        if (RebuildRawFrame(sf)) ++rebuilt;
    }
    std::fprintf(stderr, "recording: %d frames without master, %d with the pen in range\n", recSlaveOnly,
                 recPenInRange);
    if (rebuilt != 0) {
        std::fprintf(stderr, "recording has no raw SPI data: %d frames rebuilt from suffix words with an empty matrix\n",
                     rebuilt);
    }

    for (int fi = 0; fi < to; ++fi) {
        Solvers::HeatmapFrame& f = frames[static_cast<size_t>(fi)];
        if (pen && fi >= penFrom && fi <= penTo) {
            // CSV 的像素 → StylusSolver 的坐标（面板原始行列方向，每节点 1024 单位）
            const float dim1 = penX * static_cast<float>(Frame::kRxCount) * 1024.0f / 2560.0f;
            const float dim2 = penY * static_cast<float>(Frame::kTxCount) * 1024.0f / 1600.0f;
            MarkStylusSignal(f, static_cast<int>(dim2 / 1024.0f), static_cast<int>(dim1 / 1024.0f));
            auto& out = f.stylus.output;
            out = {};
            out.valid = true;
            out.inRange = true;
            out.tipDown = !penHover;
            out.pressure = penHover ? 0 : 1024;
            out.point.valid = true;
            out.point.x = dim1;
            out.point.y = dim2;
            out.point.tiltX = static_cast<int16_t>(penTiltX);
            out.point.tiltY = static_cast<int16_t>(penTiltY);
            if (!penHover) tsa->OnBtPressure();
            if (penInterleave && (fi & 1) != 0) f.masterWasRead = false;
        }
        decltype(f.stylus.input.btSample) recordedBt{};
        if (stylusSolver) {
            // 录制里的蓝牙采样是当时管线看到的最新一包，序号变了才算新包
            const auto bt = f.stylus.input.btSample;
            recordedBt = bt;
            if (bt.hasSample && bt.seq != btSeqSeen) {
                btSeqSeen = bt.seq;
                stylus->SetBtMcuPressurePacket(bt.pressure, bt.rawPressure, bt.freq1, bt.freq2);
                tsa->OnBtPressure();
            }
            stylus->Process(f);
        }

        const int repeats = (fi == holdFrame) ? holdCount + 1 : 1;
        bool dispatch = false;
        for (int rp = 0; rp < repeats; ++rp, ++step) {
            if (fi == sosFrame && rp == 0) tsa->OnAfeSos();
            // 时间戳取录制里的接收时间（没有时按 120 Hz 合成），回放结果只取决于输入；
            // --hold 多喂的帧每帧顺延 8 ms
            if (rp != 0) holdShiftMs += 8;
            const uint64_t baseMs = f.receiveSystemEpochUs != 0
                ? f.receiveSystemEpochUs / 1000
                : static_cast<uint64_t>(fed) * 25 / 3;
            const auto t0 = profile ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
            dispatch = tsa->Process(f, baseMs + holdShiftMs);
            if (profile) {
                solverNs += static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count());
                ++solverCalls;
            }
            if (!traceState(step, fi)) return 1;
            statusSeen |= tsa->LastAfeStatus();
            if (stylusSolver) {
                const auto& info = tsa->LastStylusInfo();
                const auto& report = tsa->LastStylusReport();
                StylusPeak peak;
                const bool hasPeak = FindStylusPeak(info.tx1, peak);
                const bool reportInRange = report.status != 0;
                const bool reportTipDown = (report.status & Solvers::Tsa::StylusReportFlag::kTipDown) != 0;
                if (hasPeak || reportInRange || f.stylus.output.valid) {
                    std::fprintf(stderr, "STY frame=%d master=%d status=0x%02x report(inRange=%d tip=%d x=%u y=%u tilt=%d,%d p=%u)",
                                 fi, f.masterWasRead ? 1 : 0, info.status, reportInRange ? 1 : 0, reportTipDown ? 1 : 0,
                                 report.x, report.y, report.tiltX, report.tiltY, report.pressure);
                    if (hasPeak) {
                        // 重心换成像素，格子中心记在半格处
                        std::fprintf(stderr, " tx1peak(row=%d col=%d v=%d -> x=%.0f y=%.0f)", peak.row, peak.col, peak.value,
                                     (peak.centerCol + 0.5) * 2560.0 / Frame::kRxCount,
                                     (peak.centerRow + 0.5) * 1600.0 / Frame::kTxCount);
                    }
                    {
                        // 倾角的中间量：TX2/TX1 信号比、坐标差上限、TX1 − TX2 坐标差（1024 单位 / 节点）
                        const auto& tilt = f.stylus.runtime.hpp3.tilt;
                        const auto& post = f.stylus.runtime.hpp3.post;
                        std::fprintf(stderr, " tilt(valid=%d ratio=%u lim=%u dif=%d,%d", tilt.valid ? 1 : 0, tilt.signalRatio,
                                     tilt.lenLimit, tilt.diffDim1, tilt.diffDim2);
#if EGOTOUCH_DIAG
                        std::fprintf(stderr, " raw=%d,%d", tilt.rawDiffDim1, tilt.rawDiffDim2);
#endif
                        std::fprintf(stderr, ") post(speed=%d coef=%d)", static_cast<int>(post.speedValue),
                                     static_cast<int>(post.iirCoef));
                    }
                    std::fprintf(stderr, " bt(seq=%u p=%u,%u,%u,%u) stylusMode=%d asaInRange=%d\n", recordedBt.seq,
                                 recordedBt.pressure[0], recordedBt.pressure[1], recordedBt.pressure[2],
                                 recordedBt.pressure[3], tsa->InStylusMode() ? 1 : 0, tsa->StylusInRange() ? 1 : 0);
                }
            }
            if (pen && (tsa->InStylusMode() != penMode || tsa->StylusInRange() != penInRange)) {
                penMode = tsa->InStylusMode();
                penInRange = tsa->StylusInRange();
                std::fprintf(stderr, "PEN frame=%d step=%d stylusMode=%d inRange=%d\n", fi, step, penMode ? 1 : 0,
                             penInRange ? 1 : 0);
            }
            if (!afe) continue;

            const auto& req = tsa->LastAfeRequests();
            if (!SameRequests(req, lastReq)) {
                std::fprintf(stderr, "REQ frame=%d step=%d freq=%d rate=%d idle=%d recalib=%d (status=0x%02x ctrl=0x%08x)\n",
                             fi, step, req.scanFreqIndex, req.scanRateIndex, req.okToEnterIdle ? 1 : 0,
                             req.reCalibNeeded ? 1 : 0, tsa->LastAfeStatus(), tsa->LastCtrlFlags());
                lastReq = req;
            }
            for (const auto& action : tsa->PendingAfeActions()) {
                std::fprintf(stderr, "ACT frame=%d step=%d %s(%u)\n", fi, step, ActionLabel(action.type),
                             static_cast<unsigned>(action.param));
                // 回放里没有 IC：命令一律按成功回报，握手状态照常推进
                tsa->OnAfeActionExecuted(action, true);
            }
        }
        if (fi == dump) {
            const int16_t* dif = tsa->DiffGrid();
            for (int r = 0; dif != nullptr && r < Frame::kTxCount; ++r) {
                for (int c = 0; c < Frame::kRxCount; ++c) {
                    std::fprintf(stderr, "%6d", dif[r * Frame::kRxCount + c]);
                }
                std::fputc('\n', stderr);
            }
        }
        ++fed;
        if (dispatch) ++dispatched;

        const auto& contacts = f.touch.output.contacts;
        size_t reported = 0;
        for (const auto& c : contacts) {
            if (c.id > 0 && c.isReported) ++reported;
        }
        if (reported > maxTouch) maxTouch = reported;
        if (reported != 0) ++withTouch;
        if (quiet) continue;
        if (reported == 0) {
            std::printf("%d,0,,,,\n", fi);
            continue;
        }
        for (const auto& c : contacts) {
            if (c.id <= 0 || !c.isReported) continue;
            std::printf("%d,%zu,%d,%s,%.1f,%.1f\n", fi, reported, c.id, EventLabel(c.reportEvent),
                        static_cast<double>(c.x), static_cast<double>(c.y));
        }
    }
    std::fprintf(stderr, "fed %d frames, %d dispatched, %d with touches, max %zu touches", fed, dispatched,
                 withTouch, maxTouch);
    std::fprintf(stderr, ", tsa available=%d, afe status bits seen 0x%02x", tsa->IsAvailable() ? 1 : 0, statusSeen);
    std::fputc('\n', stderr);
    if (profile) PrintProfile(solverNs, solverCalls);
    return 0;
}
