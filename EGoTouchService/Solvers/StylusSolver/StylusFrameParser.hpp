#pragma once

#include "SolverTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace Solvers::Stylus {

class StylusFrameParser {
public:
    static constexpr std::size_t kSlaveWordCount = static_cast<std::size_t>(Asa::kBlockWords * 2);
    static constexpr std::size_t kSlaveWordOffset = static_cast<std::size_t>(Asa::kSlaveHeaderBytes);
    static constexpr std::size_t kSlaveFrameBytes =
        static_cast<std::size_t>(Asa::kSlaveHeaderBytes) + kSlaveWordCount * sizeof(uint16_t);
    static constexpr std::size_t kMinimumSlaveSignalBytes = kSlaveWordOffset + 4;

    bool m_enabled = true;
    bool m_enableSlaveChecksum = false;

    inline bool Process(HeatmapFrame& frame) const {
        auto& stylus = frame.stylus;
        auto& flow = stylus.runtime.flow;
        auto& parse = stylus.runtime.parse;
        auto& rawGrid = stylus.runtime.rawGrid;

        flow.pipelineStage = 1;
        flow.frameClass = Asa::StylusFrameClass::ShortFrame;
        parse = {};
        rawGrid = {};

        const StylusInputSnapshot priorInput = stylus.input;
        stylus.input = {};
        stylus.input.btSample = priorInput.btSample;

        if (!m_enabled) {
            flow.terminal = true;
            parse.valid = false;
            parse.slaveValid = false;
            parse.checksumOk = false;
            return true;
        }

        if (frame.rawPtr == nullptr) {
            if (TryProcessFromSlaveSuffix(frame, priorInput)) {
                return true;
            }
            flow.terminal = true;
            parse.valid = false;
            parse.slaveValid = false;
            parse.checksumOk = false;
            return true;
        }

        const std::size_t available = std::min(frame.rawLen, kSlaveFrameBytes);
        if (available < kMinimumSlaveSignalBytes) {
            if (TryProcessFromSlaveSuffix(frame, priorInput)) {
                return true;
            }
            flow.terminal = true;
            parse.valid = false;
            parse.slaveValid = false;
            parse.checksumOk = false;
            return true;
        }

        const std::size_t slaveOffset = frame.rawLen - available;
        const uint8_t* slave = frame.rawPtr + slaveOffset;

        const uint16_t status = ReadLe16(slave);
        const uint16_t checksum16 = (available >= 4) ? ReadLe16(slave + 2) : 0;
        const bool hasCurrentStylusSignal = DecodeSignalPresence(slave, available);

        parse.slaveValid = true;
        parse.status = status;
        parse.checksum16 = checksum16;
        parse.checksumOk = true;
        std::memcpy(parse.rawSlaveHdr.data(), slave, Asa::kSlaveHeaderBytes);

        stylus.input.slaveValid = true;
        stylus.input.checksumOk = true;
        stylus.input.slaveWordOffset = static_cast<uint8_t>(kSlaveWordOffset);
        stylus.input.checksum16 = checksum16;
        stylus.input.status = status;

        const StylusInputSnapshot currentInput = stylus.input;

        if (available < kSlaveFrameBytes) {
            if (TryProcessFromSlaveSuffix(frame, currentInput)) {
                return true;
            }
            parse.checksumOk = false;
            stylus.input.checksumOk = false;
            flow.terminal = true;
            flow.frameClass = Asa::StylusFrameClass::ShortFrame;
            parse.valid = false;
            parse.hasCurrentStylusSignal = false;
            return true;
        }

        parse.isFullFrame = true;
        if (m_enableSlaveChecksum &&
            !ValidateChecksum16(slave + Asa::kSlaveHeaderBytes, checksum16)) {
            parse.checksumOk = false;
            stylus.input.checksumOk = false;
            flow.terminal = true;
            flow.frameClass = Asa::StylusFrameClass::ParseFail;
            parse.valid = false;
            parse.hasCurrentStylusSignal = false;
            return true;
        }

        if (!hasCurrentStylusSignal) {
            flow.terminal = true;
            flow.frameClass = Asa::StylusFrameClass::NoSignal;
            parse.valid = false;
            parse.hasCurrentStylusSignal = false;
            return true;
        }

        const uint8_t* wordPtr = slave + Asa::kSlaveHeaderBytes;
        rawGrid.asaGrid = Asa::ExtractGridFromSlavePayloadBytes(
            wordPtr, kSlaveWordCount * sizeof(uint16_t));
        parse.hasCurrentStylusSignal = true;
        stylus.input.tx1BlockValid = rawGrid.asaGrid.tx1.valid;
        stylus.input.tx2BlockValid = rawGrid.asaGrid.tx2.valid;

        if (!rawGrid.asaGrid.tx1.valid) {
            flow.terminal = true;
            flow.frameClass = Asa::StylusFrameClass::Tx1Missing;
            parse.valid = false;
            return true;
        }

        flow.terminal = false;
        flow.frameClass = Asa::StylusFrameClass::Valid;
        parse.valid = true;
        parse.hasCurrentStylusSignal = true;
        return true;
    }

private:
    static inline bool TryProcessFromSlaveSuffix(HeatmapFrame& frame,
                                                 const StylusInputSnapshot& priorInput) {
        if (!frame.slaveSuffixValid) return false;

        auto& stylus = frame.stylus;
        auto& flow = stylus.runtime.flow;
        auto& parse = stylus.runtime.parse;
        auto& rawGrid = stylus.runtime.rawGrid;

        rawGrid.asaGrid = Asa::ExtractGridFromSlaveWords(
            frame.slaveSuffix.words, Frame::kSlaveSuffixWords);

        parse.slaveValid = true;
        parse.status = priorInput.status;
        parse.checksum16 = priorInput.checksum16;
        parse.checksumOk = priorInput.checksumOk;
        parse.hasCurrentStylusSignal = rawGrid.asaGrid.tx1.valid || rawGrid.asaGrid.tx2.valid;

        stylus.input.slaveValid = true;
        stylus.input.checksumOk = priorInput.checksumOk;
        stylus.input.slaveWordOffset = priorInput.slaveWordOffset;
        stylus.input.checksum16 = priorInput.checksum16;
        stylus.input.status = priorInput.status;
        stylus.input.tx1BlockValid = rawGrid.asaGrid.tx1.valid;
        stylus.input.tx2BlockValid = rawGrid.asaGrid.tx2.valid;

        if (!parse.hasCurrentStylusSignal) {
            flow.terminal = true;
            flow.frameClass = Asa::StylusFrameClass::NoSignal;
            parse.valid = false;
            return true;
        }

        if (!rawGrid.asaGrid.tx1.valid) {
            flow.terminal = true;
            flow.frameClass = Asa::StylusFrameClass::Tx1Missing;
            parse.valid = false;
            return true;
        }

        flow.terminal = false;
        flow.frameClass = Asa::StylusFrameClass::Valid;
        parse.valid = true;
        return true;
    }

    static inline uint16_t ReadLe16(const uint8_t* ptr) {
        return static_cast<uint16_t>(
            static_cast<uint16_t>(ptr[0]) |
            (static_cast<uint16_t>(ptr[1]) << 8));
    }

    static inline bool DecodeSignalPresence(const uint8_t* slave, std::size_t available) {
        if (available < kMinimumSlaveSignalBytes) return false;
        const uint16_t anchorRow = ReadLe16(slave + kSlaveWordOffset);
        const uint16_t anchorCol = ReadLe16(slave + kSlaveWordOffset + 2);
        return !(((anchorRow & 0xFFu) == Asa::kAnchorInvalid) &&
                 ((anchorCol & 0xFFu) == Asa::kAnchorInvalid));
    }

    static inline bool ValidateChecksum16(const uint8_t* payload, uint16_t expectedChecksum) {
        uint32_t sum = 0;
        for (std::size_t i = 0; i < kSlaveWordCount; ++i) {
            sum += ReadLe16(payload + i * sizeof(uint16_t));
        }
        const uint16_t computed = static_cast<uint16_t>(sum & 0xFFFFu);
        return computed == expectedChecksum;
    }
};

} // namespace Solvers::Stylus
