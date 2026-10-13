// TsaRawReplay —— 把一段 .dvrbin 录制灌进 TsaCoreSolver（反编译的 TSACore 手指管线），
// 逐帧打印 frame.touch.output.contacts。加 --legacy 时改用 TouchPipeline 并输出同一格式，
// 两次运行的 CSV 可以直接对比（TSACore 是进程级单例，A/B 只能分进程跑）。
//
//   TsaRawReplay.exe <file.dvrbin> [--legacy] [--to N] [--dump N] [--charger] [--quiet]
//                    [--afe] [--idle-exit N] [--hold N K]
//
//   --afe          把 TSACore 的闭环请求与要下发的 AFE 动作打到 stderr（请求只在变化时打印）
//   --idle-exit N  在第 N 帧之前注入一次「宿主观察到 IDLE 退出」
//   --hold N K     把第 N 帧额外重复喂 K 次（模拟长时间静止）
//
// 依赖 DVR 读取器回填原始 SPI 帧，只在 Debug 配置下可用。

#include "DvrBinaryIO.h"
#include "FrameLayout.h"
#include "SolverTypes.h"
#include "TouchSolver/TouchPipeline.h"
#include "TsaCore/TsaCoreSolver.h"

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

bool SameRequests(const Solvers::Tsa::AfeRequests& a, const Solvers::Tsa::AfeRequests& b) {
    return a.scanFreqIndex == b.scanFreqIndex && a.scanRateIndex == b.scanRateIndex &&
           a.okToEnterIdle == b.okToEnterIdle && a.reCalibNeeded == b.reCalibNeeded;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: TsaRawReplay <file.dvrbin> [--legacy] [--to N] [--dump N] [--charger] [--quiet] "
                             "[--afe] [--idle-exit N] [--hold N K]\n");
        return 2;
    }
    int to = -1;
    int dump = -1;   // 该帧处理后把 TSACore 的 diff 网格（40 行 x 60 列）打到 stderr
    int idleExit = -1;
    int holdFrame = -1;
    int holdCount = 0;
    bool legacy = false;
    bool charger = false;
    bool quiet = false;
    bool afe = false;
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--to") == 0 && i + 1 < argc) to = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--dump") == 0 && i + 1 < argc) dump = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--idle-exit") == 0 && i + 1 < argc) idleExit = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--hold") == 0 && i + 2 < argc) {
            holdFrame = std::atoi(argv[++i]);
            holdCount = std::atoi(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--legacy") == 0) legacy = true;
        else if (std::strcmp(argv[i], "--charger") == 0) charger = true;
        else if (std::strcmp(argv[i], "--quiet") == 0) quiet = true;
        else if (std::strcmp(argv[i], "--afe") == 0) afe = true;
    }

    std::vector<Solvers::HeatmapFrame> frames;
    int version = 0;
    std::string err;
    if (!Dvr::ReadBinaryFile(argv[1], frames, version, nullptr, &err)) {
        std::fprintf(stderr, "read failed: %s\n", err.c_str());
        return 1;
    }
    if (to < 0 || to > static_cast<int>(frames.size())) to = static_cast<int>(frames.size());
    std::fprintf(stderr, "loaded %zu frames (dvr v%d), solver=%s\n", frames.size(), version,
                 legacy ? "TouchPipeline" : "TsaCoreSolver");

    // 两个解算器都带大块状态，放堆上
    auto tsa = std::make_unique<Solvers::Tsa::TsaCoreSolver>();
    auto pipeline = std::make_unique<Solvers::TouchPipeline>();
    tsa->SetChargerPlugged(charger);

    int fed = 0, withTouch = 0, dispatched = 0;
    int step = 0;   // 喂进解算器的帧序号，--hold 重复的帧也计数
    uint64_t holdShiftMs = 0;
    size_t maxTouch = 0;
    unsigned statusSeen = 0;
    Solvers::Tsa::AfeRequests lastReq{};
    if (!quiet) std::printf("frame,num,id,event,x,y\n");

    for (int fi = 0; fi < to; ++fi) {
        Solvers::HeatmapFrame& f = frames[static_cast<size_t>(fi)];
        if (!f.masterWasRead) continue;

        const int repeats = (fi == holdFrame && !legacy) ? holdCount + 1 : 1;
        bool dispatch = false;
        for (int rp = 0; rp < repeats; ++rp, ++step) {
            if (legacy) {
                dispatch = pipeline->Process(f);
                continue;
            }
            if (fi == idleExit && rp == 0) tsa->OnAfeIdleExit();
            // 时间戳取录制里的接收时间（没有时按 120 Hz 合成），回放结果只取决于输入；
            // --hold 多喂的帧每帧顺延 8 ms
            if (rp != 0) holdShiftMs += 8;
            const uint64_t baseMs = f.receiveSystemEpochUs != 0
                ? f.receiveSystemEpochUs / 1000
                : static_cast<uint64_t>(fed) * 25 / 3;
            dispatch = tsa->Process(f, baseMs + holdShiftMs);
            statusSeen |= tsa->LastAfeStatus();
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
        if (!legacy && fi == dump) {
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
    if (!legacy) {
        std::fprintf(stderr, ", tsa available=%d, afe status bits seen 0x%02x", tsa->IsAvailable() ? 1 : 0,
                     statusSeen);
    }
    std::fputc('\n', stderr);
    return 0;
}
