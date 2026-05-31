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

static_assert(Ipc::kSharedFrameAbiVersion == 5, "SharedFrameData slot seqlock ABI version must remain 5");
static_assert(sizeof(Ipc::SharedStylusRawGridBlock) == 168, "SharedStylusRawGridBlock ABI size changed");
static_assert(sizeof(Ipc::SharedStylusRawGrid) == 336, "SharedStylusRawGrid ABI size changed");
static_assert(offsetof(Ipc::SharedFrameData, stylusRawGrid) == 16106, "SharedFrameData::stylusRawGrid ABI offset changed");
static_assert(offsetof(Ipc::SharedFrameData, stylusAsaMode) == 16442, "SharedFrameData::stylusAsaMode ABI offset changed");
static_assert(sizeof(Ipc::SharedFrameData) == 17264, "SharedFrameData ABI size changed");
static_assert(offsetof(Ipc::SharedTripleBuffer, slotFrameIds) == 320, "SharedTripleBuffer::slotFrameIds ABI offset changed");
static_assert(offsetof(Ipc::SharedTripleBuffer, slotSequences) == 384, "SharedTripleBuffer::slotSequences ABI offset changed");
static_assert(offsetof(Ipc::SharedTripleBuffer, slots) == 408, "SharedTripleBuffer::slots ABI offset changed");
static_assert(sizeof(Ipc::SharedTripleBuffer) == 52224, "SharedTripleBuffer ABI size changed");
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

} // namespace

int main() {
    using namespace Ipc;

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
