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

    const auto selectTerminalProtocol = [&]() {
        if (m_lastActiveProtocol == StylusRuntime::Protocol::Hpp2) {
            frame.stylus.runtime.SelectHpp2().flow.terminal = true;
        } else if (m_lastActiveProtocol == StylusRuntime::Protocol::Hpp3) {
            frame.stylus.runtime.SelectHpp3().flow.terminal = true;
        } else if (m_penSession.protocolHint == StylusProtocolHint::Hpp2) {
            frame.stylus.runtime.SelectHpp2().flow.terminal = true;
        } else if (m_penSession.protocolHint == StylusProtocolHint::Hpp3) {
            frame.stylus.runtime.SelectHpp3().flow.terminal = true;
        } else {
            // Protocol-neutral terminal: keep activeProtocol as None so a fresh
            // disconnected session does not get misclassified as HPP3. Mark both
            // runtimes terminal because Active() maps None to HPP3 for legacy reads.
            frame.stylus.runtime.hpp2.flow.terminal = true;
            frame.stylus.runtime.hpp3.flow.terminal = true;
        }
    };

    if (m_penSession.hasConnectionState && !m_penSession.connected) {
        selectTerminalProtocol();
        FinalizeTerminalFrame(frame);
        return true;
    }

    const bool hasHpp3Evidence = frame.rawPtr != nullptr || frame.slaveSuffixValid;
    const StylusInputSnapshot inputBeforeParse = frame.stylus.input;
    m_frameParser.Process(frame);

    const bool parsedTerminal = frame.stylus.runtime.Active().flow.terminal;
    const bool parsedHpp2 = frame.stylus.runtime.activeProtocol == StylusRuntime::Protocol::Hpp2;

    if (parsedTerminal && !hasHpp3Evidence &&
        m_penSession.protocolHint == StylusProtocolHint::Hpp2 && !parsedHpp2) {
        frame.stylus.input = inputBeforeParse;
        m_frameParser.ProcessHpp2Line(frame);
    }

    if (frame.stylus.runtime.Active().flow.terminal) {
        FinalizeTerminalFrame(frame);
        return true;
    }

    bool completed = false;
    if (frame.stylus.runtime.activeProtocol == StylusRuntime::Protocol::Hpp2) {
        completed = m_hpp2.Process(frame);
    } else if (frame.stylus.runtime.activeProtocol == StylusRuntime::Protocol::Hpp3) {
        completed = m_hpp3.Process(frame);
    }

    if (!completed) {
        FinalizeTerminalFrame(frame);
        return true;
    }

    m_lastFrameWasTerminal = false;

    // ── Shared / common post-processing tail ───────────────────────
    m_edgeCoorProcess.Process(frame);
    m_edgeCoorPostProcess.Process(frame);
    m_commonPost.Process(frame);
    m_edgeCoorProcess.CaptureFinal(frame.stylus.runtime.Active());
    m_commit.Commit(frame);
    if (frame.stylus.runtime.activeProtocol != StylusRuntime::Protocol::None) {
        m_lastActiveProtocol = frame.stylus.runtime.activeProtocol;
    }
    return true;
}

void StylusPipeline::ApplyPenSession(const StylusPenSession& session) {
    const bool changed =
        m_penSession.hasConnectionState != session.hasConnectionState ||
        m_penSession.connected != session.connected ||
        m_penSession.hasStylusId != session.hasStylusId ||
        m_penSession.stylusId != session.stylusId ||
        m_penSession.protocolHint != session.protocolHint ||
        m_penSession.revision != session.revision;

    m_penSession = session;
    if (!changed) {
        return;
    }

    ResetStatefulStages();
    ClearBtSample();
    if (m_penSession.connected) {
        if (m_penSession.protocolHint == StylusProtocolHint::Hpp2) {
            m_lastActiveProtocol = StylusRuntime::Protocol::Hpp2;
        } else if (m_penSession.protocolHint == StylusProtocolHint::Hpp3) {
            m_lastActiveProtocol = StylusRuntime::Protocol::Hpp3;
        } else {
            m_lastActiveProtocol = StylusRuntime::Protocol::None;
        }
    }
    m_lastFrameWasTerminal = true;
}

void StylusPipeline::ResetStatefulStages() {
    m_hpp2.ResetOnTerminal();
    m_hpp3.ResetOnTerminal();
    m_edgeCoorProcess.Reset();
    m_edgeCoorPostProcess.Reset();
    m_commonPost.ResetOnTerminal();
}

void StylusPipeline::ClearBtSample() {
    std::lock_guard<std::mutex> lk(m_btMutex);
    m_btSample = {};
}

void StylusPipeline::FinalizeTerminalFrame(HeatmapFrame& frame) {
    if (frame.stylus.runtime.activeProtocol != StylusRuntime::Protocol::None) {
        m_lastActiveProtocol = frame.stylus.runtime.activeProtocol;
    }
    if (!m_lastFrameWasTerminal) {
        ResetStatefulStages();
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
