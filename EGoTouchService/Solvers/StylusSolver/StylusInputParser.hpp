#pragma once

#include "AsaTypes.hpp"
#include "StylusFrameState.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>

namespace Asa {

class StylusInputParser {
public:
    static constexpr size_t kSlaveHeaderBytes = Asa::kSlaveHeaderBytes;
    static constexpr size_t kSlaveWordCount = Frame::kSlaveSuffixWords;
    static constexpr size_t kSlaveWordOffset = kSlaveHeaderBytes;
    static constexpr size_t kSlaveFrameBytes = Frame::kSlaveFrameSize;
    static constexpr size_t kFrameRawOffset = Frame::kSlaveHeaderOffset;
    static constexpr size_t kMinimumSlaveSignalBytes = kSlaveWordOffset + 4;

    struct ParseResult {
        StylusFrameClass frameClass = StylusFrameClass::ShortFrame;
        bool valid = false;
        bool slaveValid = false;
        bool isFullFrame = false;
        uint16_t status = 0;
        AsaGridData gridData{};
        bool checksumFailed = false;
        uint16_t checksumValue = 0;
        bool hasCurrentStylusSignal = false;
        std::array<uint8_t, kSlaveHeaderBytes> rawSlaveHdr{};
    };

    bool enableSlaveChecksum = false;

    // Primary pipeline entry: parse the raw frame owned by state and write
    // parser classification/output directly into state.parse/state.stylus/state.flow.
    inline ParseResult Process(Solvers::StylusFrameState& state) const {
        return Process(state, enableSlaveChecksum);
    }

    inline ParseResult Process(Solvers::StylusFrameState& state,
                               bool enableSlaveChecksum) const {
        const ParseResult res = ParseFrame(state.frame, enableSlaveChecksum);
        ApplyToState(state, res);
        return res;
    }

    // Compatibility wrapper for callers that still operate on the slave overlay bytes.
    inline ParseResult Process(std::span<const uint8_t> rawData) const {
        return Parse(rawData, enableSlaveChecksum);
    }

    inline ParseResult Process(std::span<const uint8_t> rawData,
                               bool enableSlaveChecksum) const {
        return Parse(rawData, enableSlaveChecksum);
    }

    // Legacy parsing entry kept for incremental migration; new pipeline code should
    // prefer Process(state, ...), which also updates StylusFrameState in-place.
    static inline ParseResult Parse(std::span<const uint8_t> rawData, bool enableSlaveChecksum) {
        return ParseSlaveFrame(rawData, enableSlaveChecksum);
    }

private:
    static inline ParseResult ParseFrame(const Solvers::HeatmapFrame& frame,
                                         bool enableSlaveChecksum) {
        return ParseSlaveFrame(GetSlaveFrameView(frame), enableSlaveChecksum);
    }

    static inline ParseResult ParseSlaveFrame(std::span<const uint8_t> rawData,
                                              bool enableSlaveChecksum) {
        ParseResult res{};
        DecodeSignalPresence(rawData, res);

        const size_t required = kSlaveHeaderBytes + kSlaveWordCount * 2;
        if (rawData.size() < required) {
            CopyHeader(rawData, res);
            res.frameClass = StylusFrameClass::ShortFrame;
            return res;
        }

        res.isFullFrame = true;

        if (enableSlaveChecksum &&
            !ValidateChecksum16(rawData.data() + kSlaveHeaderBytes, kSlaveWordCount, res.checksumValue)) {
            res.checksumFailed = true;
        }

        CopyHeader(rawData, res);

        if (res.checksumFailed) {
            res.frameClass = StylusFrameClass::ParseFail;
            return res;
        }

        if (!res.hasCurrentStylusSignal) {
            res.frameClass = StylusFrameClass::NoSignal;
            return res;
        }

        std::array<uint16_t, kSlaveWordCount> sw{};
        const uint8_t* payload = rawData.data() + kSlaveHeaderBytes;
        for (size_t i = 0; i < kSlaveWordCount; ++i) {
            sw[i] = ReadU16Le(payload + i * 2);
        }
        res.gridData = ExtractGridFromSlaveWords(sw.data(), static_cast<int>(sw.size()));

        if (!res.gridData.tx1.valid) {
            res.frameClass = StylusFrameClass::Tx1Missing;
            return res;
        }

        res.frameClass = StylusFrameClass::Valid;
        res.valid = true;
        return res;
    }
    static inline uint16_t ReadU16Le(const uint8_t* p) {
        return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
    }

    static inline void DecodeSignalPresence(std::span<const uint8_t> rawData, ParseResult& res) {
        res.slaveValid = (rawData.size() >= kMinimumSlaveSignalBytes);
        if (!res.slaveValid) {
            res.hasCurrentStylusSignal = false;
            return;
        }

        const uint8_t* anchor = rawData.data() + kSlaveWordOffset;
        const uint16_t anchorRow = ReadU16Le(anchor);
        const uint16_t anchorCol = ReadU16Le(anchor + 2);
        res.hasCurrentStylusSignal = !((anchorRow & 0xFFu) == Frame::kAnchorInvalid &&
                                       (anchorCol & 0xFFu) == Frame::kAnchorInvalid);
    }

    static inline void CopyHeader(std::span<const uint8_t> rawData, ParseResult& res) {
        if (rawData.size() < kSlaveHeaderBytes) {
            return;
        }

        const uint8_t* p = rawData.data();
        std::memcpy(res.rawSlaveHdr.data(), p, kSlaveHeaderBytes);
        res.status = ReadU16Le(p);
    }

    static inline bool ValidateChecksum16(const uint8_t* bytes, size_t wordCount, uint16_t& outChecksum) {
        uint32_t sum = 0;
        for (size_t i = 0; i < wordCount; ++i) {
            sum += ReadU16Le(bytes + i * 2);
        }
        outChecksum = static_cast<uint16_t>(sum & 0xFFFF);
        return (outChecksum == 0) && (sum != 0);
    }

    static inline std::span<const uint8_t> GetSlaveFrameView(const Solvers::HeatmapFrame& frame) {
        if (frame.rawPtr == nullptr || frame.rawLen <= kFrameRawOffset) {
            return {};
        }

        const size_t available = std::min(frame.rawLen - kFrameRawOffset, kSlaveFrameBytes);
        return std::span<const uint8_t>(frame.rawPtr + kFrameRawOffset, available);
    }

    static inline void ApplyToState(Solvers::StylusFrameState& state, const ParseResult& res) {
        state.flow = {};
        state.parse = {};

        state.parse.frameClass = res.frameClass;
        state.parse.valid = res.valid;
        state.parse.slaveValid = res.slaveValid;
        state.parse.isFullFrame = res.isFullFrame;
        state.parse.checksumFailed = res.checksumFailed;
        state.parse.hasCurrentStylusSignal = res.hasCurrentStylusSignal;
        state.parse.status = res.status;
        state.parse.checksumValue = res.checksumValue;
        state.parse.rawSlaveHdr = res.rawSlaveHdr;
        state.parse.gridData = res.gridData;

        auto& stylus = state.stylus;
        stylus.slaveValid = res.slaveValid;
        stylus.status = res.status;
        stylus.checksumOk = !res.checksumFailed;
        stylus.checksum16 = res.checksumValue;
        stylus.slaveWordOffset = static_cast<uint8_t>(kSlaveWordOffset);
        stylus.tx1BlockValid = res.gridData.tx1.valid;
        stylus.tx2BlockValid = res.gridData.tx2.valid;

        ApplyFrameClassToFlow(state.flow, res.frameClass);
    }

    static inline void ApplyFrameClassToFlow(Solvers::StylusFlowState& flow,
                                             StylusFrameClass frameClass) {
        flow = {};

        switch (frameClass) {
        case StylusFrameClass::Valid:
            flow.packetRoute = Solvers::StylusPacketRoute::Valid;
            flow.pipelineStage = 0;
            break;

        case StylusFrameClass::ShortFrame:
            flow.terminal = true;
            flow.clearCommitted = true;
            flow.resetPost = true;
            flow.resetNoise = false;
            flow.pipelineStage = 1;
            flow.packetRoute = Solvers::StylusPacketRoute::ParseFailure13;
            break;

        case StylusFrameClass::NoSignal:
            flow.terminal = true;
            flow.clearCommitted = true;
            flow.resetPost = true;
            flow.resetNoise = true;
            flow.pipelineStage = 0;
            flow.packetRoute = Solvers::StylusPacketRoute::InvalidZeroState;
            break;

        case StylusFrameClass::ParseFail:
            flow.terminal = true;
            flow.clearCommitted = true;
            flow.resetPost = true;
            flow.resetNoise = false;
            flow.pipelineStage = 1;
            flow.packetRoute = Solvers::StylusPacketRoute::ParseFailure13;
            break;

        case StylusFrameClass::Tx1Missing:
            flow.terminal = true;
            flow.clearCommitted = true;
            flow.resetPost = true;
            flow.resetNoise = true;
            flow.pipelineStage = 2;
            flow.packetRoute = Solvers::StylusPacketRoute::InvalidZeroState;
            break;
        }
    }
};

} // namespace Asa
