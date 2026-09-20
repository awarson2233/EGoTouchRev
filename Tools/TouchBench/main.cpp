// TouchBench —— 手指解算管线吞吐基准。
//
// 从一段 .dvrbin 录制里取出原始 SPI 帧，反复灌进 TouchPipeline 并统计耗时。
// 喂法与 DeviceRuntime::OnFrame 一致（rawPtr / rawLen + masterWasRead），管线
// 看到的输入与真机相同，因此测出来的是解算本身的成本，不含 SPI 读取与 HID 派发。
//
// 播放顺序是**往返**的：从头放到尾算一轮，再从尾放回头算第二轮，如此交替。
// 这样跨轮衔接处不会出现坐标跳变，基线、追踪、滤波这些跨帧状态始终工作在连续
// 输入上，不会因为每轮开头的突变走进与真机不同的分支。
//
//   TouchBench.exe [file.dvrbin] [--frames N] [--rounds N] [--warmup N] [--csv path]
//
//   --frames N   共解算 N 帧后停止（默认 10000）
//   --rounds N   改为按轮计数：跑满 N 轮（一轮 = 单向扫完整段录制一次）
//   --warmup N   正式计时前先空跑 N 帧，不计入统计（默认 200）
//   --csv path   把每帧耗时（微秒）逐行写出，供画分布图
//
// 帧数据自己解析，不走 Dvr::ReadBinaryFile —— 后者只在 EGOTOUCH_DIAG 构建里保留
// rawData，而基准必须在 Release 下跑才有意义。这里只读 rawData / rawDataLength /
// masterWasRead 三个字段，字段位置仍按文件自带的 DVR2 帧 schema 定位。

#include "DvrFormat.h"
#include "TouchSolver/TouchPipeline.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace DvrFmt = Dvr::Format;

namespace {

constexpr const char* kDefaultRecording =
    "D:/source/repos/EGoTouchRev/Tools/dvr20260912_044210_361363.dvrbin";

struct Options {
    std::string path = kDefaultRecording;
    long long   frames = 10000;
    long long   rounds = 0;     // 非 0 时改按轮计数，frames 失效
    long long   warmup = 200;
    std::string csvPath;
};

bool ParseArgs(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        auto takePositive = [&](long long& dst) {
            if (i + 1 >= argc) return false;
            dst = std::atoll(argv[++i]);
            return dst > 0;
        };
        if (std::strcmp(a, "--frames") == 0) {
            if (!takePositive(opt.frames)) return false;
            opt.rounds = 0;
        } else if (std::strcmp(a, "--rounds") == 0) {
            if (!takePositive(opt.rounds)) return false;
        } else if (std::strcmp(a, "--warmup") == 0) {
            if (i + 1 >= argc) return false;
            opt.warmup = std::atoll(argv[++i]);
            if (opt.warmup < 0) return false;
        } else if (std::strcmp(a, "--csv") == 0) {
            if (i + 1 >= argc) return false;
            opt.csvPath = argv[++i];
        } else if (a[0] == '-') {
            return false;
        } else {
            opt.path = a;
        }
    }
    return true;
}

/// 一帧录制里解算真正要吃的东西：原始 SPI 字节。
struct RawFrame {
    std::vector<uint8_t> bytes;
    uint64_t timestamp = 0;
};

bool LoadRawFrames(const std::string& path, std::vector<RawFrame>& out,
                   uint32_t& outVersion, size_t& outTotalRecords, std::string& err) {
    std::ifstream in(path, std::ios::binary);
    if (!in) { err = "cannot open " + path; return false; }

    DvrFmt::Dvr2FileHeader header{};
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!in || std::memcmp(header.magic, "EGODVR2", 7) != 0) {
        err = "not a DVR2 file"; return false;
    }
    outVersion = header.formatVersion;

    std::vector<DvrFmt::Dvr2SectionEntry> toc(header.sectionCount);
    in.seekg(static_cast<std::streamoff>(header.tocOffset));
    in.read(reinterpret_cast<char*>(toc.data()),
            static_cast<std::streamsize>(toc.size() * sizeof(DvrFmt::Dvr2SectionEntry)));
    if (!in) { err = "truncated TOC"; return false; }

    const DvrFmt::Dvr2SectionEntry* schemaSec = nullptr;
    const DvrFmt::Dvr2SectionEntry* frameSec = nullptr;
    for (const auto& s : toc) {
        if (s.type == static_cast<uint32_t>(DvrFmt::Dvr2SectionType::FrameSchema)) schemaSec = &s;
        if (s.type == static_cast<uint32_t>(DvrFmt::Dvr2SectionType::Frames))      frameSec = &s;
    }
    if (!schemaSec || !frameSec) { err = "missing FrameSchema/Frames section"; return false; }

    DvrFmt::Dvr2FrameSchemaHeader schema{};
    in.seekg(static_cast<std::streamoff>(schemaSec->offset));
    in.read(reinterpret_cast<char*>(&schema), sizeof(schema));
    if (!in || schema.fieldRecordSize != sizeof(DvrFmt::Dvr2FieldDef)) {
        err = "unsupported frame schema layout"; return false;
    }
    std::vector<DvrFmt::Dvr2FieldDef> fields(schema.fieldCount);
    in.read(reinterpret_cast<char*>(fields.data()),
            static_cast<std::streamsize>(fields.size() * sizeof(DvrFmt::Dvr2FieldDef)));
    if (!in) { err = "truncated frame schema"; return false; }

    const auto* rawField = DvrFmt::FindField(fields, "rawData");
    const auto* rawLenField = DvrFmt::FindField(fields, "rawDataLength");
    const auto* masterField = DvrFmt::FindField(fields, "masterWasRead");
    const auto* tsField = DvrFmt::FindField(fields, "timestamp");
    if (!rawField || !rawLenField) { err = "recording carries no rawData"; return false; }

    const uint32_t recordSize = schema.frameRecordSize;
    if (recordSize == 0) { err = "zero frame record size"; return false; }
    const size_t recordCount = static_cast<size_t>(frameSec->size / recordSize);
    outTotalRecords = recordCount;

    std::vector<uint8_t> record(recordSize);
    in.seekg(static_cast<std::streamoff>(frameSec->offset));
    for (size_t i = 0; i < recordCount; ++i) {
        in.read(reinterpret_cast<char*>(record.data()), recordSize);
        if (!in) { err = "truncated frame section"; return false; }

        // 只有 master 帧会进手指管线，其余帧跳过，免得摊薄每帧耗时。
        if (masterField != nullptr) {
            uint8_t wasRead = 0;
            std::memcpy(&wasRead, record.data() + masterField->offset, sizeof(wasRead));
            if (wasRead == 0) continue;
        }

        uint16_t rawLen = 0;
        std::memcpy(&rawLen, record.data() + rawLenField->offset, sizeof(rawLen));
        const size_t len = std::min<size_t>({rawLen, Frame::kTotalFrameSize, rawField->size});
        if (len == 0) continue;

        RawFrame rf;
        rf.bytes.assign(record.data() + rawField->offset,
                        record.data() + rawField->offset + len);
        if (tsField != nullptr && tsField->size == sizeof(uint64_t)) {
            std::memcpy(&rf.timestamp, record.data() + tsField->offset, sizeof(rf.timestamp));
        }
        out.push_back(std::move(rf));
    }
    if (out.empty()) { err = "no usable master frames"; return false; }
    return true;
}

/// 往返播放的第 step 步落在录制的哪一帧。
/// 一轮 = 单向扫完 n 帧；偶数轮正放，奇数轮倒放。
size_t PingPongIndex(long long step, size_t n) noexcept {
    const long long span = static_cast<long long>(n);
    const long long round = step / span;
    const long long pos = step % span;
    return (round % 2 == 0) ? static_cast<size_t>(pos)
                            : static_cast<size_t>(span - 1 - pos);
}

double Percentile(const std::vector<double>& sorted, double p) {
    if (sorted.empty()) return 0.0;
    const size_t idx = static_cast<size_t>(p * static_cast<double>(sorted.size() - 1) + 0.5);
    return sorted[std::min(idx, sorted.size() - 1)];
}

} // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!ParseArgs(argc, argv, opt)) {
        std::fprintf(stderr,
            "usage: TouchBench [file.dvrbin] [--frames N] [--rounds N] [--warmup N] [--csv path]\n");
        return 2;
    }

    std::vector<RawFrame> raw;
    uint32_t version = 0;
    size_t totalRecords = 0;
    std::string err;
    if (!LoadRawFrames(opt.path, raw, version, totalRecords, err)) {
        std::fprintf(stderr, "%s\n", err.c_str());
        return 1;
    }

    // 预建 HeatmapFrame，计时循环里只改 timestamp，不做任何分配。
    std::vector<Solvers::HeatmapFrame> frames(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        frames[i].rawPtr = raw[i].bytes.data();
        frames[i].rawLen = raw[i].bytes.size();
        frames[i].masterWasRead = true;
        frames[i].timestamp = raw[i].timestamp;
    }

    const size_t n = frames.size();
    const long long total = (opt.rounds > 0)
                          ? opt.rounds * static_cast<long long>(n)
                          : opt.frames;

    std::printf("recording : %s\n", opt.path.c_str());
    std::printf("dvr ver   : %u\n", version);
    std::printf("master fr : %zu / %zu\n", n, totalRecords);
    std::printf("play order: ping-pong (1 round = one sweep over the recording)\n");
    std::printf("rounds    : %.2f\n", static_cast<double>(total) / static_cast<double>(n));
    std::printf("frames    : %lld  (warmup %lld)\n\n", total, opt.warmup);

    Solvers::TouchPipeline pipeline;

    long long step = 0;
    for (long long i = 0; i < opt.warmup; ++i, ++step) {
        pipeline.Process(frames[PingPongIndex(step, n)]);
    }

    std::vector<double> us;
    us.reserve(static_cast<size_t>(total));

    const auto wallStart = std::chrono::steady_clock::now();
    for (long long i = 0; i < total; ++i, ++step) {
        Solvers::HeatmapFrame& f = frames[PingPongIndex(step, n)];
        const auto t0 = std::chrono::steady_clock::now();
        pipeline.Process(f);
        const auto t1 = std::chrono::steady_clock::now();
        us.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
    }
    const auto wallEnd = std::chrono::steady_clock::now();

    const double wallMs = std::chrono::duration<double, std::milli>(wallEnd - wallStart).count();
    double sum = 0.0;
    for (double v : us) sum += v;
    const double mean = sum / static_cast<double>(us.size());

    std::vector<double> sorted = us;
    std::sort(sorted.begin(), sorted.end());

    std::printf("total     : %.3f ms  (%.3f s)\n", wallMs, wallMs / 1000.0);
    std::printf("per frame : %.3f us  mean\n", mean);
    std::printf("            %.3f us  min\n", sorted.front());
    std::printf("            %.3f us  p50\n", Percentile(sorted, 0.50));
    std::printf("            %.3f us  p90\n", Percentile(sorted, 0.90));
    std::printf("            %.3f us  p99\n", Percentile(sorted, 0.99));
    std::printf("            %.3f us  max\n", sorted.back());
    std::printf("throughput: %.0f frames/s\n", 1e6 / mean);
    // 真机 master 帧率约 120 Hz，一帧预算 8333 us
    std::printf("budget    : %.2f %% of one 120 Hz frame (8333 us)\n", mean / 8333.0 * 100.0);

    if (!opt.csvPath.empty()) {
        FILE* fp = std::fopen(opt.csvPath.c_str(), "w");
        if (fp == nullptr) {
            std::fprintf(stderr, "\ncannot write %s\n", opt.csvPath.c_str());
            return 1;
        }
        std::fprintf(fp, "frame,us\n");
        for (size_t i = 0; i < us.size(); ++i) {
            std::fprintf(fp, "%zu,%.3f\n", i, us[i]);
        }
        std::fclose(fp);
        std::printf("\nper-frame timings written to %s\n", opt.csvPath.c_str());
    }
    return 0;
}
