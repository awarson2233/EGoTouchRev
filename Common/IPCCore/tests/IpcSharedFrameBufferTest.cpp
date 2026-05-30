#if __has_include("Ipc/SharedFrameBuffer.h")
#include "Ipc/SharedFrameBuffer.h"
#else
#include "SharedFrameBuffer.h"
#endif

#include <cstdlib>
#include <iostream>

namespace {

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
    first.masterWasRead = true;
    writer.Write(first);

    Require(reader.LastFrameId() == 1, "frame id increments after first write");
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
    Require(!reader.Read(out), "reader suppresses duplicate read without a new frame");

    SharedFrameData second{};
    second.workerState = 9;
    second.masterWasRead = false;
    writer.Write(second);
    Require(reader.LastFrameId() == 2, "frame id increments after second write");
    Require(reader.LastSlaveFrameId() == 2, "slave frame id increments after second write");
    Require(reader.LastMasterFrameId() == 1, "master frame id is unchanged when masterWasRead is false");
    Require(reader.Read(out), "reader reads second published frame");
    Require(out.workerState == second.workerState, "second frame payload round-trips");

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
