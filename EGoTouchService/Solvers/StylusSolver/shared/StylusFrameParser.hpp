#pragma once

#include "SolverTypes.h"
#include "StylusSolver/hpp3/Hpp3Runtime.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace Solvers::Stylus {

class StylusFrameParser {
public:
    static constexpr std::size_t kSlaveWordCount = static_cast<std::size_t>(Hpp3::kBlockWords * 2);
    static constexpr std::size_t kSlaveWordOffset = static_cast<std::size_t>(Hpp3::kSlaveHeaderBytes);
    static constexpr std::size_t kSlaveFrameBytes =
        static_cast<std::size_t>(Hpp3::kSlaveHeaderBytes) + kSlaveWordCount * sizeof(uint16_t);
    static constexpr std::size_t kMinimumSlaveSignalBytes = kSlaveWordOffset + 4;

    enum class Hpp2InputPolicy : uint8_t {
        RequireAuxFlag,
        AllowWithoutAuxFlag,
    };

    bool m_enabled = true;
    bool m_enableSlaveChecksum = false;

    static inline bool IsHpp2AuxStatusFlags(uint32_t auxStatusFlags) {
        return (auxStatusFlags & 0x1u) != 0 && (auxStatusFlags & 0x2u) == 0;
    }

    inline bool Process(HeatmapFrame& frame) const {
        auto& stylus = frame.stylus;
        auto& runtime = stylus.runtime.SelectHpp3();
        auto& flow = runtime.flow;
        auto& parse = runtime.parse;
        auto& rawGrid = runtime.rawGrid;

        flow.pipelineStage = 1;
        flow.frameClass = Asa::FrameClass::ShortFrame;
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
            // Final fallback: pre-populated HPP2 line-mode input.
            // Only use it after all HPP3 raw/slave evidence paths are absent.
            return ProcessWithHpp2Fallback(frame, priorInput);
        }

        const std::size_t available = std::min(frame.rawLen, kSlaveFrameBytes);
        if (available < kMinimumSlaveSignalBytes) {
            if (TryProcessFromSlaveSuffix(frame, priorInput)) {
                return true;
            }
            return TerminalParseFailure(frame, Asa::FrameClass::ShortFrame);
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
        std::memcpy(rawGrid.rawSlaveHdr.data(), slave, Hpp3::kSlaveHeaderBytes);

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
            flow.frameClass = Asa::FrameClass::ShortFrame;
            parse.valid = false;
            parse.hasCurrentStylusSignal = false;
            return true;
        }

        parse.isFullFrame = true;
        if (m_enableSlaveChecksum &&
            !ValidateChecksum16(slave + Hpp3::kSlaveHeaderBytes, checksum16)) {
            parse.checksumOk = false;
            stylus.input.checksumOk = false;
            flow.terminal = true;
            flow.frameClass = Asa::FrameClass::ParseFail;
            parse.valid = false;
            parse.hasCurrentStylusSignal = false;
            return true;
        }

        if (!hasCurrentStylusSignal) {
            flow.terminal = true;
            flow.frameClass = Asa::FrameClass::NoSignal;
            parse.valid = false;
            parse.hasCurrentStylusSignal = false;
            return true;
        }

        const uint8_t* wordPtr = slave + Hpp3::kSlaveHeaderBytes;
        rawGrid.grid = Hpp3::ExtractGridFromSlavePayloadBytes(
            wordPtr, kSlaveWordCount * sizeof(uint16_t));
        parse.hasCurrentStylusSignal = true;
        stylus.input.tx1BlockValid = rawGrid.grid.tx1.valid;
        stylus.input.tx2BlockValid = rawGrid.grid.tx2.valid;

        if (!rawGrid.grid.tx1.valid) {
            flow.terminal = true;
            flow.frameClass = Asa::FrameClass::Tx1Missing;
            parse.valid = false;
            return true;
        }

        flow.terminal = false;
        flow.frameClass = Asa::FrameClass::Valid;
        parse.valid = true;
        parse.hasCurrentStylusSignal = true;
        return true;
    }

    inline bool ProcessHpp2Line(HeatmapFrame& frame) const {
        auto& stylus = frame.stylus;
        auto& runtime = stylus.runtime.SelectHpp2();
        auto& flow = runtime.flow;
        auto& parse = runtime.parse;

        flow.pipelineStage = 1;
        flow.frameClass = Asa::FrameClass::ShortFrame;
        parse = {};

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

        if (TryProcessFromHpp2Input(frame, priorInput, Hpp2InputPolicy::AllowWithoutAuxFlag)) {
            return true;
        }

        return TerminalParseFailure(frame, Asa::FrameClass::NoSignal);
    }

    // When no raw pointer, no slave suffix, and no grid data is available,
    // fall back to a pre-populated HPP2 line-mode input as a last resort.
    inline bool ProcessWithHpp2Fallback(HeatmapFrame& frame, const StylusInputSnapshot& priorInput) const {
        // All real-data paths have been exhausted above; HPP2 input is the final fallback.
        if (TryProcessFromHpp2Input(frame, priorInput, Hpp2InputPolicy::RequireAuxFlag)) {
            return true;
        }

        return TerminalParseFailure(frame, Asa::FrameClass::NoSignal);
    }

private:
    static inline bool TryProcessFromHpp2Input(HeatmapFrame& frame,
                                               const StylusInputSnapshot& priorInput,
                                               Hpp2InputPolicy policy) {
        if (!priorInput.hpp2LineValid) {
            return false;
        }
        if (policy == Hpp2InputPolicy::RequireAuxFlag &&
            !IsHpp2AuxStatusFlags(priorInput.auxStatusFlags)) {
            return false;
        }

        auto& stylus = frame.stylus;
        auto& runtime = stylus.runtime.SelectHpp2();
        auto& flow = runtime.flow;
        auto& parse = runtime.parse;

        stylus.input.auxStatusFlags = priorInput.auxStatusFlags;
        stylus.input.mainFreq = priorInput.mainFreq;
        stylus.input.auxFreq = priorInput.auxFreq;
        stylus.input.framePressure = priorInput.framePressure;
        stylus.input.buttonBits = priorInput.buttonBits;
        stylus.input.hpp2LineValid = priorInput.hpp2LineValid;
        stylus.input.hpp2LineData = priorInput.hpp2LineData;

        parse.valid = true;
        parse.slaveValid = false;
        parse.checksumOk = true;
        parse.hasCurrentStylusSignal = true;
        flow.terminal = false;
        flow.frameClass = Asa::FrameClass::Valid;
        return true;
    }

    static inline bool TerminalParseFailure(HeatmapFrame& frame, Asa::FrameClass frameClass) {
        auto& runtime = frame.stylus.runtime.Active();
        auto& flow = runtime.flow;
        auto& parse = runtime.parse;
        flow.terminal = true;
        flow.frameClass = frameClass;
        parse.valid = false;
        parse.slaveValid = false;
        parse.checksumOk = false;
        parse.hasCurrentStylusSignal = false;
        return true;
    }

    static inline bool TryProcessFromSlaveSuffix(HeatmapFrame& frame,
                                                 const StylusInputSnapshot& priorInput) {
        if (!frame.slaveSuffixValid) return false;

        auto& stylus = frame.stylus;
        auto& runtime = stylus.runtime.SelectHpp3();
        auto& flow = runtime.flow;
        auto& parse = runtime.parse;
        auto& rawGrid = runtime.rawGrid;

        rawGrid.grid = Hpp3::ExtractGridFromSlaveWords(
            frame.slaveSuffix.words, Frame::kSlaveSuffixWords);

        parse.slaveValid = true;
        parse.status = priorInput.status;
        parse.checksum16 = priorInput.checksum16;
        parse.checksumOk = priorInput.checksumOk;
        parse.hasCurrentStylusSignal = rawGrid.grid.tx1.valid || rawGrid.grid.tx2.valid;

        stylus.input.slaveValid = true;
        stylus.input.checksumOk = priorInput.checksumOk;
        stylus.input.slaveWordOffset = priorInput.slaveWordOffset;
        stylus.input.checksum16 = priorInput.checksum16;
        stylus.input.status = priorInput.status;
        stylus.input.tx1BlockValid = rawGrid.grid.tx1.valid;
        stylus.input.tx2BlockValid = rawGrid.grid.tx2.valid;

        if (!parse.hasCurrentStylusSignal) {
            flow.terminal = true;
            flow.frameClass = Asa::FrameClass::NoSignal;
            parse.valid = false;
            return true;
        }

        if (!rawGrid.grid.tx1.valid) {
            flow.terminal = true;
            flow.frameClass = Asa::FrameClass::Tx1Missing;
            parse.valid = false;
            return true;
        }

        flow.terminal = false;
        flow.frameClass = Asa::FrameClass::Valid;
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
        return !(((anchorRow & 0xFFu) == Hpp3::kAnchorInvalid) &&
                 ((anchorCol & 0xFFu) == Hpp3::kAnchorInvalid));
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
