#include "FrameLayout.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void PutLe16(uint8_t* bytes, int wordIndex, uint16_t value) {
    bytes[wordIndex * 2] = static_cast<uint8_t>(value & 0xFF);
    bytes[wordIndex * 2 + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void TestFrameConstants() {
    Require(Frame::kHeaderBytes == 7, "Frame header size should remain 7 bytes");
    Require(Frame::kTxCount == 40, "Frame TX count should remain 40");
    Require(Frame::kRxCount == 60, "Frame RX count should remain 60");
    Require(Frame::kMatrixCells == Frame::kTxCount * Frame::kRxCount, "Matrix cell count should match TX*RX");
    Require(Frame::kMatrixBytes == Frame::kMatrixCells * 2, "Matrix byte count should be two bytes per cell");
    Require(Frame::kMasterFrameSize == Frame::kHeaderBytes + Frame::kMatrixBytes + Frame::kMasterSuffixBytes,
            "Master frame size should match header + matrix + master suffix");
    Require(Frame::kSlaveFrameSize == Frame::kHeaderBytes + Frame::kSlaveSuffixBytes,
            "Slave frame size should match header + slave suffix");
    Require(Frame::kTotalFrameSize == 5402, "Total frame size should remain 5402 bytes");
    Require(Frame::kMatrixOffset == Frame::kHeaderBytes, "Matrix offset should follow the master header");
    Require(Frame::kMasterSuffixOffset == Frame::kHeaderBytes + Frame::kMatrixBytes,
            "Master suffix offset should follow the matrix payload");
    Require(Frame::kSlaveHeaderOffset == Frame::kMasterFrameSize, "Slave header offset should follow master frame");
    Require(Frame::kSlaveSuffixOffset == Frame::kSlaveHeaderOffset + Frame::kHeaderBytes,
            "Slave suffix offset should follow the slave header");
}

void TestMasterSuffixLoadAndPredicates() {
    std::array<uint8_t, Frame::kMasterSuffixBytes> bytes{};
    PutLe16(bytes.data(), Frame::MasterWord::kIdleFlag, 0x1234);
    PutLe16(bytes.data(), Frame::MasterWord::kFreqShiftDone, 0x5678);
    PutLe16(bytes.data(), Frame::MasterWord::kDiagStatus, 0x00BB);
    PutLe16(bytes.data(), Frame::MasterWord::kFwResetFlag, 0x0102);
    PutLe16(bytes.data(), Frame::MasterWord::kTpFreq1, 0x0304);
    PutLe16(bytes.data(), Frame::MasterWord::kTimestamp, 0x0506);
    PutLe16(bytes.data(), Frame::MasterWord::kPenF0NoiseCount, 0x0708);
    PutLe16(bytes.data(), Frame::MasterWord::kPenF1NoiseCount, 0x090A);
    PutLe16(bytes.data(), Frame::MasterWord::kTouchX, 0x0123);
    PutLe16(bytes.data(), Frame::MasterWord::kTouchY, 0x0456);

    Frame::MasterSuffixView view;
    view.LoadFromBytes(bytes.data());

    Require(view.idleFlag() == 0x1234, "MasterSuffixView should load idle flag as little-endian u16");
    Require(view.freqShiftDone() == 0x5678, "MasterSuffixView should load freqShiftDone as little-endian u16");
    Require(view.diagStatus() == 0x00BB, "MasterSuffixView should load diagStatus as little-endian u16");
    Require(view.fwResetFlag() == 0x0102, "MasterSuffixView should load fwResetFlag as little-endian u16");
    Require(view.tpFreq1() == 0x0304, "MasterSuffixView should load tpFreq1 as little-endian u16");
    Require(view.timestamp() == 0x0506 && view.tpFreq2() == 0x0506,
            "MasterSuffixView timestamp and tpFreq2 should share the confirmed word");
    Require(view.penF0NoiseCount() == 0x0708, "MasterSuffixView should load penF0NoiseCount");
    Require(view.penF1NoiseCount() == 0x090A, "MasterSuffixView should load penF1NoiseCount");
    Require(view.touchX() == 0x0123 && view.touchY() == 0x0456, "MasterSuffixView should load touch coordinates");
    Require(view.hasFinger(), "MasterSuffixView should report finger when low bytes are not both 0xFF");

    PutLe16(bytes.data(), Frame::MasterWord::kTouchX, 0x00FF);
    PutLe16(bytes.data(), Frame::MasterWord::kTouchY, 0x00FF);
    view.LoadFromBytes(bytes.data());
    Require(!view.hasFinger(), "MasterSuffixView should report no finger when X/Y low bytes are both 0xFF");
}

void TestSlaveSuffixLoadAndPredicates() {
    std::array<uint8_t, Frame::kSlaveSuffixBytes> bytes{};
    PutLe16(bytes.data(), 0, 0x0011);
    PutLe16(bytes.data(), 1, 0x0022);
    PutLe16(bytes.data(), 2 + 3 * Frame::kGridDim + 4, 0xFF80);
    PutLe16(bytes.data(), Frame::kBlockWords, 0x0033);
    PutLe16(bytes.data(), Frame::kBlockWords + 1, 0x0044);
    PutLe16(bytes.data(), Frame::kBlockWords + 2 + 8 * Frame::kGridDim + 8, 0x007F);

    Frame::SlaveSuffixView view;
    view.LoadFromBytes(bytes.data());

    Require(view.tx1AnchorRow() == 0x0011 && view.tx1AnchorCol() == 0x0022,
            "SlaveSuffixView should load TX1 anchor words as little-endian u16");
    Require(view.tx1Grid(3, 4) == static_cast<int16_t>(0xFF80),
            "SlaveSuffixView should load TX1 grid words as signed values");
    Require(view.tx2AnchorRow() == 0x0033 && view.tx2AnchorCol() == 0x0044,
            "SlaveSuffixView should load TX2 anchor words as little-endian u16");
    Require(view.tx2Grid(8, 8) == 0x007F, "SlaveSuffixView should load TX2 grid words");
    Require(view.tx1Valid(), "SlaveSuffixView should report valid TX1 when an anchor is valid");
    Require(view.tx2Valid(), "SlaveSuffixView TX2 validity currently follows TX1 validity");
    Require(view.hasStylus(), "SlaveSuffixView should report stylus when anchor low bytes are not both 0xFF");

    PutLe16(bytes.data(), 0, 0x00FF);
    PutLe16(bytes.data(), 1, 0x00FF);
    view.LoadFromBytes(bytes.data());
    Require(!view.hasStylus(), "SlaveSuffixView should report no stylus when anchor low bytes are both 0xFF");
    Require(!view.tx1Valid(), "SlaveSuffixView should report invalid TX1 when both anchors equal kAnchorInvalid");
}

} // namespace

int main() {
    try {
        TestFrameConstants();
        TestMasterSuffixLoadAndPredicates();
        TestSlaveSuffixLoadAndPredicates();
        std::cout << "[TEST] CommonFrameLayoutTest passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[TEST] CommonFrameLayoutTest failed: " << ex.what() << '\n';
        return 1;
    }
}
