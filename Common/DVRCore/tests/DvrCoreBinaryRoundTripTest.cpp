#include "DvrCoreTestSupport.h"

#include <iostream>

namespace {

void TestBinaryRoundTrip() {
    const auto path = DvrCoreTest::TempPath("binary_roundtrip");
    const std::vector<Dvr::DvrFrameSlot> frames{
        DvrCoreTest::MakeFrameSlot(100000, 1710000000000000ull, 1),
        DvrCoreTest::MakeFrameSlot(104000, 1710000000004000ull, 2),
    };

    uint32_t writeFlags = 0;
    DvrCoreTest::Require(Dvr::WriteBinaryFile(path, frames, nullptr, nullptr, nullptr, &writeFlags), "DVR2 write should succeed");
    DvrCoreTest::Require((writeFlags & Dvr::Format::kDvrFlagHasStylusDiagnostics) != 0, "write should mark stylus diagnostics");
    DvrCoreTest::Require((writeFlags & Dvr::Format::kDvrFlagHasStructuredSuffix) != 0, "write should mark structured suffix");
    DvrCoreTest::Require((writeFlags & Dvr::Format::kDvrFlagHasReceiveSystemEpochUs) != 0, "write should mark receive epoch");
    DvrCoreTest::Require((writeFlags & Dvr::Format::kDvrFlagHasRuntimeConfig) == 0, "write should omit runtime config flag");

    std::vector<Solvers::HeatmapFrame> readFrames;
    int version = 0;
    uint32_t readFlags = 0;
    std::string error;
    DvrCoreTest::Require(Dvr::ReadBinaryFile(path, readFrames, version, &readFlags, &error), error.empty() ? "DVR2 read should succeed" : error.c_str());

    DvrCoreTest::Require(version == Dvr::Format::kCurrentDvrFormatVersion, "DVR2 version should round-trip");
    DvrCoreTest::Require(readFlags == writeFlags, "DVR2 flags should round-trip");
    DvrCoreTest::Require(readFrames.size() == frames.size(), "frame count should round-trip");
    const auto& first = readFrames[0];
    DvrCoreTest::Require(first.timestamp == frames[0].timestamp, "timestamp should round-trip");
    DvrCoreTest::Require(first.receiveSystemEpochUs == frames[0].receiveSystemEpochUs, "receive epoch should round-trip");
    DvrCoreTest::Require(first.masterWasRead == frames[0].masterWasRead, "masterWasRead should round-trip");
    DvrCoreTest::Require(first.masterSuffixValid, "master suffix valid should round-trip");
    DvrCoreTest::Require(first.slaveSuffixValid, "slave suffix valid should round-trip");
    DvrCoreTest::Require(first.heatmapMatrix[0][0] == 101 && first.heatmapMatrix[7][9] == -202, "heatmap payload should round-trip");
    DvrCoreTest::Require(first.masterSuffix.words[0] == 0x1111 && first.slaveSuffix.words[0] == 0x3333, "suffix words should round-trip");
    DvrCoreTest::Require(first.touchPackets[0].valid && first.touchPackets[0].bytes[0] == 0xAA && first.touchPackets[0].bytes[31] == 0x55, "touch packet should round-trip");
    DvrCoreTest::Require(first.stylus.input.slaveValid && first.stylus.input.checksum16 == 0xBEEF, "stylus input should round-trip");
    DvrCoreTest::Require(first.stylus.output.valid && first.stylus.output.packet.bytes[16] == 0x34, "stylus output packet should round-trip");
    DvrCoreTest::Require(first.stylus.output.point.valid && first.stylus.output.point.reportX == 125, "stylus point should round-trip");
    DvrCoreTest::Require(first.contacts.size() == 1 && first.contacts[0].id == 7 && first.contacts[0].signalSum == 1000, "contacts should round-trip");
#if EGOTOUCH_DIAG
    DvrCoreTest::Require(first.rawLen == 4 && first.rawPtr != nullptr && first.rawPtr[3] == 0x40, "raw data should round-trip in diagnostic builds");
    DvrCoreTest::Require(first.touchZones[0] == 3 && first.peakZones[1] == 4, "zone arrays should round-trip in diagnostic builds");
    DvrCoreTest::Require(first.peaks.size() == 1 && first.peaks[0].z == 500, "peaks should round-trip in diagnostic builds");
#endif

    std::filesystem::remove(path);
}

void TestEmptyFrameWriteFails() {
    const auto path = DvrCoreTest::TempPath("empty_write");
    uint32_t flags = 0xFFFFFFFFu;
    DvrCoreTest::Require(!Dvr::WriteBinaryFile(path, {}, nullptr, nullptr, nullptr, &flags), "empty DVR2 write should fail");
}

} // namespace

int main() {
    try {
        TestBinaryRoundTrip();
        TestEmptyFrameWriteFails();
        std::cout << "[TEST] DVRCore binary round-trip tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] " << ex.what() << "\n";
        return 1;
    }
}
