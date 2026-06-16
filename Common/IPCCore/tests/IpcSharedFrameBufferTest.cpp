#include "SolverTypes.h"

#if __has_include("Ipc/SharedFrameBuffer.h")
#include "Ipc/SharedFrameBuffer.h"
#else
#include "SharedFrameBuffer.h"
#endif

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <type_traits>

namespace {

static_assert(Ipc::kSharedFrameAbiVersion == 6, "SharedFrameData slot seqlock ABI version must remain 6");
static_assert(sizeof(Ipc::SharedStylusRawGridBlock) == 168, "SharedStylusRawGridBlock ABI size changed");
static_assert(sizeof(Ipc::SharedStylusRawGrid) == 336, "SharedStylusRawGrid ABI size changed");
static_assert(offsetof(Ipc::SharedFrameData, stylusRawGrid) == 16106, "SharedFrameData::stylusRawGrid ABI offset changed");
static_assert(offsetof(Ipc::SharedFrameData, stylusAsaMode) == 16442, "SharedFrameData::stylusAsaMode ABI offset changed");
static_assert(sizeof(Ipc::SharedFrameData) == 18544, "SharedFrameData ABI size changed");
static_assert(offsetof(Ipc::SharedTripleBuffer, slotFrameIds) == 320, "SharedTripleBuffer::slotFrameIds ABI offset changed");
static_assert(offsetof(Ipc::SharedTripleBuffer, slotSequences) == 384, "SharedTripleBuffer::slotSequences ABI offset changed");
static_assert(offsetof(Ipc::SharedTripleBuffer, slots) == 408, "SharedTripleBuffer::slots ABI offset changed");
static_assert(sizeof(Ipc::SharedTripleBuffer) == 56064, "SharedTripleBuffer ABI size changed");
static_assert(std::is_trivially_copyable_v<Ipc::SharedFrameData>, "SharedFrameData must remain trivially copyable");
static_assert(std::is_standard_layout_v<Ipc::SharedFrameData>, "SharedFrameData must remain standard layout");

void Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << "\n";
        std::exit(1);
    }
}

bool IsSkippableWindowsGlobalObjectError(DWORD error) noexcept {
    return error == ERROR_ACCESS_DENIED ||
           error == ERROR_PRIVILEGE_NOT_HELD ||
           error == ERROR_ALREADY_EXISTS;
}

void VerifyPreserveMasterTouchVisualization() {
    Ipc::SharedFrameData cached{};
    cached.masterWasRead = true;
    cached.timestamp = 0x1111;
    cached.workerState = 1;
    cached.rawDataLength = static_cast<uint16_t>(Frame::kTotalFrameSize);
    cached.rawData[0] = 0xA1;
    cached.rawData[Frame::kMasterFrameSize - 1] = 0xA2;
    cached.rawData[Frame::kSlaveHeaderOffset] = 0xA3;
    cached.heatmapMatrix[3][4] = 321;
    cached.masterSuffixValid = true;
    cached.masterSuffix.words[0] = 0x1234;
    cached.touchZones[2][3] = 5;
    cached.peakZones[4][5] = 6;
    cached.peakCount = 1;
    cached.peaks[0].r = 7;
    cached.peaks[0].c = 8;
    cached.peaks[0].z = 900;
    cached.peaks[0].id = 9;
    cached.contactCount = 1;
    cached.contacts[0].id = 42;
    cached.contacts[0].x = 12.5f;
    cached.touchPackets[0].valid = true;
    cached.touchPackets[0].bytes[0] = 0x5A;
    cached.zoneBoxCount = 1;
    cached.zoneBoxes[0].zoneId = 3;
    cached.zoneBoxes[0].zoneIndex = 2;
    cached.zoneBoxes[0].bbox = {4, 8, 10, 15};
    cached.palmBoxCount = 1;
    cached.palmBoxes[0].id = 7;
    cached.palmBoxes[0].bbox = {5, 9, 11, 16};
    cached.palmBoxes[0].expandedBbox = {4, 10, 10, 17};

    Ipc::SharedFrameData current{};
    current.masterWasRead = false;
    current.timestamp = 0x2222;
    current.workerState = 11;
    current.rawDataLength = static_cast<uint16_t>(Frame::kTotalFrameSize);
    current.rawData[0] = 0xB1;
    current.rawData[Frame::kMasterFrameSize - 1] = 0xB2;
    current.rawData[Frame::kSlaveHeaderOffset] = 0xB3;
    current.slaveSuffixValid = true;
    current.slaveSuffix.words[0] = 0xABCD;
    current.stylusPressure = 55;
    current.stylusPoint.valid = true;
    current.stylusPoint.x = 100.0f;
    current.stylusRawGrid.tx1.valid = true;
    current.stylusRawGrid.tx1.grid[0][0] = 222;

    Ipc::PreserveMasterTouchVisualizationFromCachedFrame(current, cached);

    Require(!current.masterWasRead, "preserve helper keeps skipped-master identity");
    Require(current.timestamp == 0x2222 && current.workerState == 11,
            "preserve helper keeps live runtime identity");
    Require(current.stylusPressure == 55 && current.stylusPoint.valid && current.stylusRawGrid.tx1.grid[0][0] == 222,
            "preserve helper keeps live stylus fields");
    Require(current.slaveSuffixValid && current.slaveSuffix.words[0] == 0xABCD,
            "preserve helper keeps live slave suffix");
    Require(current.rawData[0] == 0xA1 && current.rawData[Frame::kMasterFrameSize - 1] == 0xA2,
            "preserve helper copies cached master raw bytes");
    Require(current.rawData[Frame::kSlaveHeaderOffset] == 0xB3,
            "preserve helper keeps live slave raw bytes");
    Require(current.heatmapMatrix[3][4] == 321, "preserve helper copies cached heatmap");
    Require(current.masterSuffixValid && current.masterSuffix.words[0] == 0x1234,
            "preserve helper copies cached master suffix");
    Require(current.touchZones[2][3] == 5 && current.peakZones[4][5] == 6,
            "preserve helper copies cached touch zone maps");
    Require(current.peakCount == 1 && current.peaks[0].z == 900 && current.peaks[0].id == 9,
            "preserve helper copies cached peaks");
    Require(current.contactCount == 1 && current.contacts[0].id == 42 && current.contacts[0].x == 12.5f,
            "preserve helper copies cached contacts");
    Require(current.touchPackets[0].valid && current.touchPackets[0].bytes[0] == 0x5A,
            "preserve helper copies cached touch packets");
    Require(current.zoneBoxCount == 1 && current.zoneBoxes[0].zoneId == 3 && current.zoneBoxes[0].bbox.maxR == 8,
            "preserve helper copies cached zone boxes");
    Require(current.palmBoxCount == 1 && current.palmBoxes[0].id == 7 && current.palmBoxes[0].expandedBbox.maxC == 17,
            "preserve helper copies cached palm boxes");
}

#if EGOTOUCH_DIAG
void VerifySolverDebugBoxesRoundTrip() {
    Solvers::HeatmapFrame solverIn{};

    Solvers::TouchZoneDebugBox zoneBox;
    zoneBox.zoneId = 4;
    zoneBox.zoneIndex = 3;
    zoneBox.bbox = {2, 6, 8, 12};
    zoneBox.area = 20;
    zoneBox.signalSum = 4567;
    solverIn.touch.debug.zoneBoxes.push_back(zoneBox);

    Solvers::TouchPalmDebugBox palmBox;
    palmBox.id = 11;
    palmBox.bbox = {10, 16, 20, 26};
    palmBox.expandedBbox = {9, 17, 19, 27};
    palmBox.age = 5;
    palmBox.missed = 2;
    palmBox.lastMatchedZoneIndex = 3;
    palmBox.anchorPeakCount = 4;
    palmBox.signalSum = 6789;
    palmBox.matchedPalmThisFrame = false;
    solverIn.touch.debug.palmBoxes.push_back(palmBox);

    Ipc::SharedFrameData shared{};
    Ipc::PopulateSharedFrameDataFromSolverFrame(shared, solverIn);
    Require(shared.zoneBoxCount == 1, "solver zone box count copies to shared frame");
    Require(shared.palmBoxCount == 1, "solver palm box count copies to shared frame");

    Solvers::HeatmapFrame solverOut{};
    Ipc::PopulateSolverFrameFromSharedFrameData(solverOut, shared);
    Require(solverOut.touch.debug.zoneBoxes.size() == 1, "shared zone box count copies back to solver frame");
    Require(solverOut.touch.debug.zoneBoxes[0].bbox.minR == 2 && solverOut.touch.debug.zoneBoxes[0].bbox.maxR == 6 &&
            solverOut.touch.debug.zoneBoxes[0].bbox.minC == 8 && solverOut.touch.debug.zoneBoxes[0].bbox.maxC == 12,
            "solver zone box bbox round-trips through shared conversion");
    Require(solverOut.touch.debug.palmBoxes.size() == 1, "shared palm box count copies back to solver frame");
    Require(solverOut.touch.debug.palmBoxes[0].id == 11, "solver palm box id round-trips through shared conversion");
    Require(solverOut.touch.debug.palmBoxes[0].expandedBbox.minR == 9 &&
            solverOut.touch.debug.palmBoxes[0].expandedBbox.maxR == 17 &&
            solverOut.touch.debug.palmBoxes[0].expandedBbox.minC == 19 &&
            solverOut.touch.debug.palmBoxes[0].expandedBbox.maxC == 27,
            "solver palm box expanded bbox round-trips through shared conversion");
    Require(!solverOut.touch.debug.palmBoxes[0].matchedPalmThisFrame,
            "solver palm box matched state round-trips through shared conversion");
}
#endif

} // namespace

int main() {
    using namespace Ipc;

    VerifyPreserveMasterTouchVisualization();
#if EGOTOUCH_DIAG
    VerifySolverDebugBoxesRoundTrip();
#endif

    SharedFrameWriter writer;
    if (!writer.Create(L"Global\\EGoTouchIpccoreSharedFrameBufferTest")) {
        const DWORD error = GetLastError();
        if (IsSkippableWindowsGlobalObjectError(error)) {
            std::cout << "[SKIP] Shared frame global mapping could not be created in this session. GetLastError=" << error << "\n";
            return 0;
        }
        Require(false, "SharedFrameWriter::Create succeeds");
    }
    Require(writer.IsOpen(), "writer reports open after Create");

    SharedFrameReader reader;
    if (!reader.Open(L"Global\\EGoTouchIpccoreSharedFrameBufferTest")) {
        const DWORD error = GetLastError();
        if (IsSkippableWindowsGlobalObjectError(error)) {
            std::cout << "[SKIP] Shared frame mapping could not be opened with current token. GetLastError=" << error << "\n";
            writer.Close();
            return 0;
        }
        Require(false, "SharedFrameReader::Open succeeds");
    }
    Require(reader.IsOpen(), "reader reports open after Open");
    Require(reader.FrameReadyEvent() != nullptr, "reader opens frame-ready event handle");
    Require(reader.RawBuffer() != nullptr, "reader exposes raw triple buffer");

    const SharedTripleBuffer* raw = reader.RawBuffer();
    Require(raw->abi.abiVersion == kSharedFrameAbiVersion, "ABI version matches expected value");
    Require(raw->abi.totalSize == sizeof(SharedTripleBuffer), "ABI total size matches SharedTripleBuffer");
    Require(raw->abi.headerSize == sizeof(SharedFrameAbiHeader), "ABI header size matches SharedFrameAbiHeader");
    Require(raw->abi.capabilities == kSharedFrameAbiCapabilities, "ABI capabilities match expected value");
    Require(raw->abi.slotCount == SharedTripleBuffer::kSlotCount, "ABI slot count matches triple buffer");
    Require(raw->abi.reserved == kSharedFrameAbiReserved, "ABI reserved field matches expected value");
    Require(reader.LastFrameId() == 0, "initial frame id is zero");
    Require(reader.LastSlaveFrameId() == 0, "initial slave frame id is zero");
    Require(reader.LastMasterFrameId() == 0, "initial master frame id is zero");

    SharedFrameData out{};
    Require(!reader.Read(out), "reader reports no frame before first write");

    SharedFrameData first{};
    first.streaming = true;
    first.workerState = 7;
    first.timestamp = 0x1122334455667788ull;
    first.rawDataLength = 3;
    first.rawData[0] = 0x10;
    first.rawData[1] = 0x20;
    first.rawData[2] = 0x30;
    first.contactCount = 1;
    first.contacts[0].id = 42;
    first.contacts[0].x = 123.5f;
    first.contacts[0].y = 456.25f;
    first.zoneBoxCount = 1;
    first.zoneBoxes[0].zoneId = 3;
    first.zoneBoxes[0].zoneIndex = 2;
    first.zoneBoxes[0].bbox = {4, 8, 10, 15};
    first.zoneBoxes[0].area = 12;
    first.zoneBoxes[0].signalSum = 3456;
    first.palmBoxCount = 1;
    first.palmBoxes[0].id = 7;
    first.palmBoxes[0].bbox = {5, 9, 11, 16};
    first.palmBoxes[0].expandedBbox = {4, 10, 10, 17};
    first.palmBoxes[0].age = 3;
    first.palmBoxes[0].missed = 1;
    first.palmBoxes[0].lastMatchedZoneIndex = -1;
    first.palmBoxes[0].anchorPeakCount = 2;
    first.palmBoxes[0].signalSum = 7890;
    first.palmBoxes[0].matchedPalmThisFrame = 1;
    first.stylusRawGrid.tx1.valid = true;
    first.stylusRawGrid.tx1.anchorRow = 2;
    first.stylusRawGrid.tx1.anchorCol = 3;
    first.stylusRawGrid.tx1.grid[2][3] = 1234;
    first.stylusRawGrid.tx2.valid = true;
    first.stylusRawGrid.tx2.anchorRow = 5;
    first.stylusRawGrid.tx2.anchorCol = 6;
    first.stylusRawGrid.tx2.grid[4][5] = 2345;
    first.masterWasRead = true;
    writer.Write(first);

    Require(reader.LastFrameId() == 1, "frame id increments after first write");
    const uint32_t firstReadyIdx = raw->readyIdx.load(std::memory_order_acquire);
    Require(raw->slotFrameIds[firstReadyIdx].load(std::memory_order_acquire) == reader.LastFrameId(),
            "published slot frame id matches first frame id");
    const uint64_t firstSlotSequence = raw->slotSequences[firstReadyIdx].load(std::memory_order_acquire);
    Require((firstSlotSequence & 1ull) == 0,
            "published first slot sequence is clean");
    Require(reader.LastSlaveFrameId() == 1, "slave frame id increments after first write");
    Require(reader.LastMasterFrameId() == 1, "master frame id increments when masterWasRead is true");
    Require(reader.Read(out), "reader reads first published frame");
    Require(out.streaming == first.streaming, "streaming field round-trips");
    Require(out.workerState == first.workerState, "workerState field round-trips");
    Require(out.timestamp == first.timestamp, "timestamp field round-trips");
    Require(out.rawDataLength == first.rawDataLength, "rawDataLength field round-trips");
    Require(out.rawData[0] == first.rawData[0] && out.rawData[1] == first.rawData[1] && out.rawData[2] == first.rawData[2], "rawData bytes round-trip");
    Require(out.contactCount == 1, "contact count round-trips");
    Require(out.contacts[0].id == 42, "contact id round-trips");
    Require(out.contacts[0].x == 123.5f && out.contacts[0].y == 456.25f, "contact coordinates round-trip");
    Require(out.zoneBoxCount == 1, "zone box count round-trips");
    Require(out.zoneBoxes[0].zoneId == 3 && out.zoneBoxes[0].zoneIndex == 2,
            "zone box identity round-trips");
    Require(out.zoneBoxes[0].bbox.minR == 4 && out.zoneBoxes[0].bbox.maxR == 8 &&
            out.zoneBoxes[0].bbox.minC == 10 && out.zoneBoxes[0].bbox.maxC == 15,
            "zone box bbox round-trips");
    Require(out.zoneBoxes[0].area == 12 && out.zoneBoxes[0].signalSum == 3456,
            "zone box metrics round-trip");
    Require(out.palmBoxCount == 1, "palm box count round-trips");
    Require(out.palmBoxes[0].id == 7, "palm box id round-trips");
    Require(out.palmBoxes[0].bbox.minR == 5 && out.palmBoxes[0].bbox.maxR == 9 &&
            out.palmBoxes[0].bbox.minC == 11 && out.palmBoxes[0].bbox.maxC == 16,
            "palm box bbox round-trips");
    Require(out.palmBoxes[0].expandedBbox.minR == 4 && out.palmBoxes[0].expandedBbox.maxR == 10 &&
            out.palmBoxes[0].expandedBbox.minC == 10 && out.palmBoxes[0].expandedBbox.maxC == 17,
            "palm box expanded bbox round-trips");
    Require(out.palmBoxes[0].age == 3 && out.palmBoxes[0].missed == 1 &&
            out.palmBoxes[0].lastMatchedZoneIndex == -1 && out.palmBoxes[0].anchorPeakCount == 2 &&
            out.palmBoxes[0].signalSum == 7890 && out.palmBoxes[0].matchedPalmThisFrame == 1,
            "palm box tracking fields round-trip");
    Require(out.stylusRawGrid.tx1.valid && out.stylusRawGrid.tx1.anchorRow == 2 &&
            out.stylusRawGrid.tx1.anchorCol == 3 && out.stylusRawGrid.tx1.grid[2][3] == 1234,
            "stylus TX1 raw grid round-trips");
    Require(out.stylusRawGrid.tx2.valid && out.stylusRawGrid.tx2.anchorRow == 5 &&
            out.stylusRawGrid.tx2.anchorCol == 6 && out.stylusRawGrid.tx2.grid[4][5] == 2345,
            "stylus TX2 raw grid round-trips");
    Require(!reader.Read(out), "reader suppresses duplicate read without a new frame");

    SharedFrameData second{};
    second.workerState = 9;
    second.masterWasRead = false;
    writer.Write(second);
    Require(reader.LastFrameId() == 2, "frame id increments after second write");
    const uint32_t secondReadyIdx = raw->readyIdx.load(std::memory_order_acquire);
    Require(raw->slotFrameIds[secondReadyIdx].load(std::memory_order_acquire) == reader.LastFrameId(),
            "published slot frame id matches second frame id");
    Require((raw->slotSequences[secondReadyIdx].load(std::memory_order_acquire) & 1ull) == 0,
            "published second slot sequence is clean");
    Require(reader.LastSlaveFrameId() == 2, "slave frame id increments after second write");
    Require(reader.LastMasterFrameId() == 1, "master frame id is unchanged when masterWasRead is false");
    Require(reader.Read(out), "reader reads second published frame");
    Require(out.workerState == second.workerState, "second frame payload round-trips");

    SharedFrameData third{};
    third.workerState = 10;
    writer.Write(third);
    SharedFrameData fourth{};
    fourth.workerState = 11;
    writer.Write(fourth);
    Require(reader.LastFrameId() == 4, "frame id increments through slot reuse");
    const uint32_t reusedReadyIdx = raw->readyIdx.load(std::memory_order_acquire);
    Require(reusedReadyIdx == firstReadyIdx, "fourth write reuses first slot in triple buffer rotation");
    Require(raw->slotFrameIds[reusedReadyIdx].load(std::memory_order_acquire) == reader.LastFrameId(),
            "reused slot frame id matches fourth frame id");
    const uint64_t reusedSlotSequence = raw->slotSequences[reusedReadyIdx].load(std::memory_order_acquire);
    Require((reusedSlotSequence & 1ull) == 0, "reused slot sequence is clean");
    Require(reusedSlotSequence > firstSlotSequence, "reused slot sequence advances to avoid ABA");
    Require(reader.Read(out), "reader reads reused slot frame");
    Require(out.workerState == fourth.workerState, "reused slot payload round-trips");

    reader.Close();
    Require(!reader.IsOpen(), "reader closes cleanly");
    Require(reader.RawBuffer() == nullptr, "reader RawBuffer is null after Close");
    Require(reader.FrameReadyEvent() == nullptr, "reader event handle is null after Close");
    Require(reader.LastFrameId() == 0, "reader LastFrameId returns zero after Close");
    reader.Close();

    writer.Close();
    Require(!writer.IsOpen(), "writer closes cleanly");
    writer.Close();

    std::cout << "[PASS] IpcSharedFrameBufferTest\n";
    return 0;
}
