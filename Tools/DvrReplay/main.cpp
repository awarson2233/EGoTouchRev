// DvrReplay —— 把一段 .dvrbin 录制重新灌进 TouchPipeline，逐帧打印解算结果。
// 用途是离线复现边缘断触/坐标漂移这类只在真机上出现的现象：录制里存的是原始
// SPI 帧，重放路径与 DeviceRuntime::OnFrame 的喂法一致（rawPtr/rawLen +
// masterWasRead），因此管线看到的输入与真机完全相同。
//
//   DvrReplay.exe <file.dvrbin> [--from N] [--to N]

#include "DvrBinaryIO.h"
#include "TouchSolver/TouchPipeline.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: DvrReplay <file.dvrbin> [--from N] [--to N]\n");
        return 2;
    }

    const std::string path = argv[1];
    int from = 0;
    int to = -1;
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--from") == 0 && i + 1 < argc) from = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--to") == 0 && i + 1 < argc) to = std::atoi(argv[++i]);
    }

    std::vector<Solvers::HeatmapFrame> frames;
    int version = 0;
    std::string err;
    if (!Dvr::ReadBinaryFile(path, frames, version, nullptr, &err)) {
        std::fprintf(stderr, "read failed: %s\n", err.c_str());
        return 1;
    }
    std::fprintf(stderr, "loaded %zu frames (dvr v%d)\n", frames.size(), version);

    if (to < 0 || to > static_cast<int>(frames.size())) to = static_cast<int>(frames.size());

    Solvers::TouchPipeline pipeline;

    std::printf("frame,ts,peaks,zones,tracks,id,cogX,ecX,ratio1,minC,maxC,"
                "curX,outX,state,deb,life,vol,vol2,supp,exc,palm,grip,rep,dispX\n");

    for (int fi = 0; fi < to; ++fi) {
        Solvers::HeatmapFrame& f = frames[static_cast<size_t>(fi)];
        // ReadBinaryFile 已经把 rawData 装好并指了 rawPtr/rawLen，这里只补上
        // DeviceRuntime 会做的那一步：非 master 帧不进手指管线。
        if (!f.masterWasRead) continue;

        pipeline.Process(f);
        if (fi < from) continue;

        const auto& bf = *f.touch.bigFrame;
        if (bf.tracks.empty()) {
            std::printf("%d,%llu,%zu,%zu,0,,,,,,,,,,,,,,,,,,,\n", fi,
                        static_cast<unsigned long long>(f.timestamp),
                        bf.peaks.size(), bf.zones.size());
            continue;
        }
        const Solvers::Touch::ZoneCluster* z = bf.zones.empty() ? nullptr : &bf.zones[0];
        for (const auto& t : bf.tracks) {
            float dispX = -1.0f;
            for (const auto& c : bf.contacts) {
                if (c.id == t.id) { dispX = c.x; break; }
            }
            std::printf("%d,%llu,%zu,%zu,%zu,%u,", fi,
                        static_cast<unsigned long long>(f.timestamp),
                        bf.peaks.size(), bf.zones.size(), bf.tracks.size(), t.id);
            if (z) std::printf("%u,%u,%u,%u,%u,", z->cogX_q8, z->ecX_q8,
                               z->gripRatioDim1, z->minC, z->maxC);
            else   std::printf(",,,,,");
            std::printf("%u,%u,0x%x,%u,0x%x,0x%x,0x%x,0x%x,0x%x,%d,%d,%d,%.2f\n",
                        t.curX_q8, t.outX_q8, static_cast<unsigned>(t.state), t.debounce,
                        t.lifeFlags, t.volatileFlags, t.volatileFlags2,
                        t.suppressFlags, t.exceptionType,
                        t.isPalm ? 1 : 0, t.isGrip ? 1 : 0, t.isReported ? 1 : 0, dispX);
        }
    }
    return 0;
}
