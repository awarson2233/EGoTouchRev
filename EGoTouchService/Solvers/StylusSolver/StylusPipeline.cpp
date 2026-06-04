#include "StylusPipeline.h"
#include "StylusPipelineConfigKeys.h"
#include "ConfigParse.h"

#include <algorithm>
#include <ostream>

namespace {
#if EGOTOUCH_CONFIG_ENABLED
Solvers::StylusConfig::StylusPipelineMembers MakeConfigMembers(Solvers::StylusPipeline& p) {
    Solvers::StylusConfig::StylusPipelineMembers m{};
    m.hpp2 = &p.m_hpp2;
    m.frameParser = &p.m_frameParser;
    m.featureExtractor = &p.m_hpp3.m_featureExtractor;
    m.coordinateSolver = &p.m_hpp3.m_coordinateSolver;
    m.tiltProcess = &p.m_hpp3.m_tiltProcess;
    m.pressureSolver = &p.m_hpp3.m_pressureSolver;
    m.postPressure = &p.m_hpp3.m_postPressure;
    m.edgeCoorProcess = &p.m_edgeCoorProcess;
    m.edgeCoorPostProcess = &p.m_edgeCoorPostProcess;
    m.noisePostProcess = &p.m_hpp3.m_noisePostProcess;
    m.linearFilterProcess = &p.m_commonPost.m_linearFilterProcess;
    m.coorReviseProcess = &p.m_commonPost.m_coorReviseProcess;
    m.coorSpeedProcess = &p.m_commonPost.m_coorSpeedProcess;
    m.coorIIRProcess = &p.m_commonPost.m_coorIIRProcess;
    m.aftCoorProcess = &p.m_commonPost.m_aftCoorProcess;
    return m;
}
#endif
} // namespace

namespace Solvers {

bool StylusPipeline::Process(HeatmapFrame& frame) {
    frame.stylus.ResetPerFrameState();
    ReadLatestBtSample(frame.stylus.input.btSample);

    // ── Shared: frame parsing ──
    m_frameParser.Process(frame);
    if (frame.stylus.runtime.Active().flow.terminal) {
        FinalizeTerminalFrame(frame);
        return true;
    }

    const uint32_t auxStatusFlags = frame.stylus.input.auxStatusFlags;
    const bool isHpp2 = (auxStatusFlags & 0x1u) != 0 && (auxStatusFlags & 0x2u) == 0;

    if (isHpp2) {
        if (!m_hpp2.Process(frame)) {
            FinalizeTerminalFrame(frame);
            return true;
        }
    } else {
        // ── HPP3: feature extraction → pressure ────────────────────
        // Preserve the existing HPP3 path when auxStatusFlags is still zero;
        // the current shared parser does not expose TSACore stylusFrame flags yet.
        if (!m_hpp3.Process(frame)) {
            FinalizeTerminalFrame(frame);
            return true;
        }
    }

    m_lastFrameWasTerminal = false;

    // ── Shared / common post-processing tail ───────────────────────
    m_edgeCoorProcess.Process(frame);
    m_edgeCoorPostProcess.Process(frame);
    m_commonPost.Process(frame);
    m_edgeCoorProcess.CaptureFinal(frame.stylus.runtime.Active());
    m_commit.Commit(frame);
    return true;
}

void StylusPipeline::FinalizeTerminalFrame(HeatmapFrame& frame) {
    if (!m_lastFrameWasTerminal) {
        m_hpp2.ResetOnTerminal();
        m_hpp3.ResetOnTerminal();
        m_edgeCoorProcess.Reset();
        m_edgeCoorPostProcess.Reset();
        m_commonPost.ResetOnTerminal();
    }
    m_lastFrameWasTerminal = true;
#if EGOTOUCH_DIAG
    frame.stylus.runtime.ResetDiagnosticFields();
#endif
    m_edgeCoorProcess.CaptureFinal(frame.stylus.runtime.Active());
    m_commit.Commit(frame);
}

std::vector<ConfigParam> StylusPipeline::GetConfigSchema() const {
#if EGOTOUCH_CONFIG_ENABLED
    auto m = MakeConfigMembers(const_cast<StylusPipeline&>(*this));
    return StylusConfig::GetConfigSchema(m);
#else
    return {};
#endif
}

void StylusPipeline::SaveConfig(std::ostream& out) const {
#if EGOTOUCH_CONFIG_ENABLED
    auto m = MakeConfigMembers(const_cast<StylusPipeline&>(*this));
    StylusConfig::SaveConfig(m, out);
#else
    (void)out;
#endif
}

void StylusPipeline::LoadConfig(const std::string& key, const std::string& value) {
#if EGOTOUCH_CONFIG_ENABLED
    std::string canonicalKey = key;
    if (canonicalKey == "sp.preEnabled") {
        canonicalKey = "sp.frameParserEnabled";
    } else if (canonicalKey == "sp.solveEnabled") {
        canonicalKey = "sp.peakDetectorEnabled";
    }

    auto m = MakeConfigMembers(*this);
    StylusConfig::LoadConfig(m, canonicalKey, value);
#else
    (void)key;
    (void)value;
#endif
}

void StylusPipeline::SetBtMcuPressure(uint16_t pressure) {
    Asa::BtInputSnapshot next{};
    next.pressure[3] = pressure;
    next.hasSample = true;

    std::lock_guard<std::mutex> lk(m_btMutex);
    next.seq = m_btSample.seq + 1;
    m_btSample = next;
}

void StylusPipeline::SetBtMcuPressurePacket(const std::array<uint16_t, 4>& pressure,
                                            const std::array<uint16_t, 4>& rawPressure,
                                            uint8_t freq1,
                                            uint8_t freq2) {
    Asa::BtInputSnapshot next{};
    next.pressure = pressure;
    next.rawPressure = rawPressure;
    next.freq1 = freq1;
    next.freq2 = freq2;
    next.hasSample = true;
    next.hasFreq = true;

    std::lock_guard<std::mutex> lk(m_btMutex);
    next.seq = m_btSample.seq + 1;
    m_btSample = next;
}

void StylusPipeline::ReadLatestBtSample(Asa::BtInputSnapshot& out) const {
    std::lock_guard<std::mutex> lk(m_btMutex);
    out = m_btSample;
}

} // namespace Solvers
