#include "StylusPipeline.h"
#include "StylusPipelineConfigKeys.h"
#include "ConfigParse.h"

#include <algorithm>
#include <ostream>

namespace {
#if EGOTOUCH_CONFIG_ENABLED
Solvers::StylusConfig::StylusPipelineMembers MakeConfigMembers(Solvers::StylusPipeline& p) {
    Solvers::StylusConfig::StylusPipelineMembers m{};
    m.frameParser = &p.m_frameParser;
    m.featureExtractor = &p.m_featureExtractor;
    m.coordinateSolver = &p.m_coordinateSolver;
    m.tiltProcess = &p.m_tiltProcess;
    m.pressureSolver = &p.m_pressureSolver;
    m.postPressure = &p.m_postPressure;
    m.edgeCoorProcess = &p.m_edgeCoorProcess;
    m.edgeCoorPostProcess = &p.m_edgeCoorPostProcess;
    m.noisePostProcess = &p.m_noisePostProcess;
    m.linearFilterProcess = &p.m_linearFilterProcess;
    m.coorReviseProcess = &p.m_coorReviseProcess;
    m.coorSpeedProcess = &p.m_coorSpeedProcess;
    m.coorIIRProcess = &p.m_coorIIRProcess;
    m.aftCoorProcess = &p.m_aftCoorProcess;
    return m;
}
#endif
} // namespace

namespace Solvers {

bool StylusPipeline::Process(HeatmapFrame& frame) {
    frame.stylus.ResetPerFrameState();
    ReadLatestBtSample(frame.stylus.input.btSample);

    m_frameParser.Process(frame);
    if (frame.stylus.runtime.flow.terminal) {
        FinalizeTerminalFrame(frame);
        return true;
    }

    m_featureExtractor.Process(frame);
    if (frame.stylus.runtime.flow.terminal) {
        FinalizeTerminalFrame(frame);
        return true;
    }

    m_coordinateSolver.Process(frame);
    if (frame.stylus.runtime.flow.terminal) {
        FinalizeTerminalFrame(frame);
        return true;
    }

    m_lastFrameWasTerminal = false;
    m_noisePostProcess.Process(frame);
    if (frame.stylus.runtime.post.noiseRejected) {
        m_tiltProcess.Reset();
        frame.stylus.runtime.tilt = {};
    } else {
        m_tiltProcess.Process(frame);
    }
    m_pressureSolver.Process(frame);
    m_postPressure.Process(frame);
    m_edgeCoorProcess.Process(frame);
    m_edgeCoorPostProcess.Process(frame);
    m_linearFilterProcess.Process(frame);
    m_coorReviseProcess.Process(frame);
    m_coorSpeedProcess.Process(frame);
    m_coorIIRProcess.Process(frame);
    m_aftCoorProcess.Process(frame);
    m_edgeCoorProcess.CaptureFinal(frame.stylus.runtime);
    m_commit.Commit(frame);
    return true;
}

void StylusPipeline::FinalizeTerminalFrame(HeatmapFrame& frame) {
    if (!m_lastFrameWasTerminal) {
        m_tiltProcess.Reset();
        m_postPressure.Reset();
        m_edgeCoorProcess.Reset();
        m_edgeCoorPostProcess.Reset();
        m_linearFilterProcess.Reset();
        m_coorReviseProcess.Reset();
        m_coorSpeedProcess.Reset();
        m_coorIIRProcess.Reset();
        m_aftCoorProcess.Reset();
    }
    m_lastFrameWasTerminal = true;
#if EGOTOUCH_DIAG
    frame.stylus.runtime.ResetDiagnosticFields();
#endif
    m_edgeCoorProcess.CaptureFinal(frame.stylus.runtime);
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

#if 0  // Legacy handwritten config implementation kept for review/reference.
std::vector<ConfigParam> StylusPipeline::GetConfigSchema() const {
    std::vector<ConfigParam> schema;
    schema.reserve(41);
    schema.emplace_back("sp.frameParserEnabled", "Frame Parser Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_frameParser.m_enabled))
        .Module("Frame Parser");
    schema.emplace_back("sp.peakDetectorEnabled", "Peak Detector Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_featureExtractor.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.coordinateSolverEnabled", "Coordinate Solver Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_coordinateSolver.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.tiltProcessEnabled", "Tilt Process Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_tiltProcess.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.pressureSolverEnabled", "Pressure Solver Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_pressureSolver.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.postPressureEnabled", "Post Pressure Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_postPressure.m_enabled))
        .Module("Pressure");
    schema.emplace_back("sp.fakePressureDecreaseEnabled", "Fake Pressure Decrease Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_postPressure.m_fakePressureDecreaseEnabled))
        .Module("Pressure");
    schema.emplace_back("sp.btFreqShiftDebounceFrames", "BT Freq Shift Debounce Frames",
                        ConfigParam::Int, const_cast<int*>(&m_postPressure.m_btFreqShiftDebounceFrames), 0.0f, 255.0f)
        .Module("Pressure");
    schema.emplace_back("sp.pressureEdgeEnterThreshold", "Pressure Edge Enter Threshold",
                        ConfigParam::UInt16, const_cast<uint16_t*>(&m_postPressure.m_pressureEdgeEnterThreshold), 0.0f, 65535.0f)
        .Module("Pressure");
    schema.emplace_back("sp.pressureEdgeExitThreshold", "Pressure Edge Exit Threshold",
                        ConfigParam::UInt16, const_cast<uint16_t*>(&m_postPressure.m_pressureEdgeExitThreshold), 0.0f, 65535.0f)
        .Module("Pressure");
    schema.emplace_back("sp.tipDownPressureThreshold", "Tip Down Pressure Threshold",
                        ConfigParam::UInt16, const_cast<uint16_t*>(&m_pressureSolver.m_tipDownPressureThreshold), 0.0f, 4095.0f)
        .Module("Pressure");
    schema.emplace_back("sp.btPressSignalSuppressEnterThreshold", "BT Press Suppress Enter Threshold",
                        ConfigParam::UInt16, const_cast<uint16_t*>(&m_pressureSolver.m_btPressSignalSuppressEnterThreshold), 0.0f, 65535.0f)
        .Module("Pressure");
    schema.emplace_back("sp.btPressSignalSuppressExitThreshold", "BT Press Suppress Exit Threshold",
                        ConfigParam::UInt16, const_cast<uint16_t*>(&m_pressureSolver.m_btPressSignalSuppressExitThreshold), 0.0f, 65535.0f)
        .Module("Pressure");
    schema.emplace_back("sp.signalFloor", "Signal Floor",
                        ConfigParam::UInt16, const_cast<uint16_t*>(&m_coordinateSolver.m_signalFloor), 0.0f, 65535.0f)
        .Module("Coordinate");
    schema.emplace_back("sp.edgeCoorEnabled", "Edge Coor Process Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_edgeCoorProcess.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.edgeCoorPostEnabled", "Edge Coor Post Process Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_edgeCoorPostProcess.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.noisePostEnabled", "Noise Post Process Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_noisePostProcess.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.noiseSignalRatioThold", "Noise Signal Ratio Threshold",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_noisePostProcess.m_signalRatioThreshold), 1.0f, 16.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.noiseSignalDropRatio", "Noise Signal Magnitude Drop Ratio",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_noisePostProcess.m_signalMagnitudeDropRatio), 1.0f, 16.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.linearFilterEnabled", "Linear Filter Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_linearFilterProcess.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.coorReviseEnabled", "CoorRevise Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_coorReviseProcess.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.coorReviseFactorDim1", "CoorRevise Factor Dim1",
                        ConfigParam::Int, const_cast<int*>(&m_coorReviseProcess.m_factorDim1), 0.0f, 255.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.coorReviseFactorDim2", "CoorRevise Factor Dim2",
                        ConfigParam::Int, const_cast<int*>(&m_coorReviseProcess.m_factorDim2), 0.0f, 255.0f)
        .Module("Data Solve");

    // ── CoorSpeedProcess ──
    schema.emplace_back("sp.coorSpeedEnabled", "Coor Speed Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_coorSpeedProcess.m_enabled))
        .Module("Data Solve");

    // ── CoorIIRProcess ──
    schema.emplace_back("sp.iirFilterEnabled", "IIR Filter Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_coorIIRProcess.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.iirCoefLowInBand", "IIR Coef Low In-Band",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_coorIIRProcess.m_coefLowInBand), 0.0f, 255.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.iirCoefHighInBand", "IIR Coef High In-Band",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_coorIIRProcess.m_coefHighInBand), 0.0f, 255.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.iirSpeedTholdInBand", "IIR Speed Thold In-Band",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_coorIIRProcess.m_speedTholdInBand), 0.0f, 255.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.iirCoefLowEdge", "IIR Coef Low Edge",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_coorIIRProcess.m_coefLowEdge), 0.0f, 255.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.iirCoefHighEdge", "IIR Coef High Edge",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_coorIIRProcess.m_coefHighEdge), 0.0f, 255.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.iirSpeedTholdEdge", "IIR Speed Thold Edge",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_coorIIRProcess.m_speedTholdEdge), 0.0f, 255.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.iirSpeedMax", "IIR Speed Max",
                        ConfigParam::Int, const_cast<int*>(&m_coorIIRProcess.m_speedMax), 0.0f, 1000.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.iirMaxCoef", "IIR Max Coef (Denominator)",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_coorIIRProcess.m_maxCoef), 0.0f, 255.0f)
        .Module("Data Solve");

    // ── AftCoorProcess ──
    schema.emplace_back("sp.aftCoorEnabled", "AFT Coor Process Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_aftCoorProcess.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.lockFlashInBandX", "Lock Flash In-Band X",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_aftCoorProcess.m_lockFlashInBandX), 0.0f, 255.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.lockFlashInBandY", "Lock Flash In-Band Y",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_aftCoorProcess.m_lockFlashInBandY), 0.0f, 255.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.lockFlashEdgeX", "Lock Flash Edge X",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_aftCoorProcess.m_lockFlashEdgeX), 0.0f, 255.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.lockFlashEdgeY", "Lock Flash Edge Y",
                        ConfigParam::UInt8, const_cast<uint8_t*>(&m_aftCoorProcess.m_lockFlashEdgeY), 0.0f, 255.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.lockSensorTxCount", "Lock Sensor TX Count",
                        ConfigParam::Int, const_cast<int*>(&m_aftCoorProcess.m_sensorTxCount), 1.0f, 200.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.lockSensorRxCount", "Lock Sensor RX Count",
                        ConfigParam::Int, const_cast<int*>(&m_aftCoorProcess.m_sensorRxCount), 1.0f, 200.0f)
        .Module("Data Solve");
    schema.emplace_back("sp.lockBypass", "Lock Bypass",
                        ConfigParam::Bool, const_cast<bool*>(&m_aftCoorProcess.m_bypassLock))
        .Module("Data Solve");
    return schema;
}

void StylusPipeline::SaveConfig(std::ostream& out) const {
    out << "sp.frameParserEnabled=" << (m_frameParser.m_enabled ? "1" : "0") << "\n";
    out << "sp.peakDetectorEnabled=" << (m_featureExtractor.m_enabled ? "1" : "0") << "\n";
    out << "sp.coordinateSolverEnabled=" << (m_coordinateSolver.m_enabled ? "1" : "0") << "\n";
    out << "sp.tiltProcessEnabled=" << (m_tiltProcess.m_enabled ? "1" : "0") << "\n";
    out << "sp.pressureSolverEnabled=" << (m_pressureSolver.m_enabled ? "1" : "0") << "\n";
    out << "sp.postPressureEnabled=" << (m_postPressure.m_enabled ? "1" : "0") << "\n";
    out << "sp.fakePressureDecreaseEnabled=" << (m_postPressure.m_fakePressureDecreaseEnabled ? "1" : "0") << "\n";
    out << "sp.btFreqShiftDebounceFrames=" << m_postPressure.m_btFreqShiftDebounceFrames << "\n";
    out << "sp.pressureEdgeEnterThreshold=" << m_postPressure.m_pressureEdgeEnterThreshold << "\n";
    out << "sp.pressureEdgeExitThreshold=" << m_postPressure.m_pressureEdgeExitThreshold << "\n";
    out << "sp.tipDownPressureThreshold=" << m_pressureSolver.m_tipDownPressureThreshold << "\n";
    out << "sp.btPressSignalSuppressEnterThreshold=" << m_pressureSolver.m_btPressSignalSuppressEnterThreshold << "\n";
    out << "sp.btPressSignalSuppressExitThreshold=" << m_pressureSolver.m_btPressSignalSuppressExitThreshold << "\n";
    out << "sp.signalFloor=" << m_coordinateSolver.m_signalFloor << "\n";
    out << "sp.edgeCoorEnabled=" << (m_edgeCoorProcess.m_enabled ? "1" : "0") << "\n";
    out << "sp.edgeCoorPostEnabled=" << (m_edgeCoorPostProcess.m_enabled ? "1" : "0") << "\n";
    out << "sp.noisePostEnabled=" << (m_noisePostProcess.m_enabled ? "1" : "0") << "\n";
    out << "sp.noiseSignalRatioThold=" << static_cast<int>(m_noisePostProcess.m_signalRatioThreshold) << "\n";
    out << "sp.noiseSignalDropRatio=" << static_cast<int>(m_noisePostProcess.m_signalMagnitudeDropRatio) << "\n";
    out << "sp.linearFilterEnabled=" << (m_linearFilterProcess.m_enabled ? "1" : "0") << "\n";
    out << "sp.coorReviseEnabled=" << (m_coorReviseProcess.m_enabled ? "1" : "0") << "\n";
    out << "sp.coorReviseFactorDim1=" << m_coorReviseProcess.m_factorDim1 << "\n";
    out << "sp.coorReviseFactorDim2=" << m_coorReviseProcess.m_factorDim2 << "\n";
    out << "sp.coorSpeedEnabled=" << (m_coorSpeedProcess.m_enabled ? "1" : "0") << "\n";
    out << "sp.iirFilterEnabled=" << (m_coorIIRProcess.m_enabled ? "1" : "0") << "\n";
    out << "sp.iirCoefLowInBand=" << static_cast<int>(m_coorIIRProcess.m_coefLowInBand) << "\n";
    out << "sp.iirCoefHighInBand=" << static_cast<int>(m_coorIIRProcess.m_coefHighInBand) << "\n";
    out << "sp.iirSpeedTholdInBand=" << static_cast<int>(m_coorIIRProcess.m_speedTholdInBand) << "\n";
    out << "sp.iirCoefLowEdge=" << static_cast<int>(m_coorIIRProcess.m_coefLowEdge) << "\n";
    out << "sp.iirCoefHighEdge=" << static_cast<int>(m_coorIIRProcess.m_coefHighEdge) << "\n";
    out << "sp.iirSpeedTholdEdge=" << static_cast<int>(m_coorIIRProcess.m_speedTholdEdge) << "\n";
    out << "sp.iirSpeedMax=" << m_coorIIRProcess.m_speedMax << "\n";
    out << "sp.iirMaxCoef=" << static_cast<int>(m_coorIIRProcess.m_maxCoef) << "\n";
    out << "sp.aftCoorEnabled=" << (m_aftCoorProcess.m_enabled ? "1" : "0") << "\n";
    out << "sp.lockFlashInBandX=" << static_cast<int>(m_aftCoorProcess.m_lockFlashInBandX) << "\n";
    out << "sp.lockFlashInBandY=" << static_cast<int>(m_aftCoorProcess.m_lockFlashInBandY) << "\n";
    out << "sp.lockFlashEdgeX=" << static_cast<int>(m_aftCoorProcess.m_lockFlashEdgeX) << "\n";
    out << "sp.lockFlashEdgeY=" << static_cast<int>(m_aftCoorProcess.m_lockFlashEdgeY) << "\n";
    out << "sp.lockSensorTxCount=" << m_aftCoorProcess.m_sensorTxCount << "\n";
    out << "sp.lockSensorRxCount=" << m_aftCoorProcess.m_sensorRxCount << "\n";
    out << "sp.lockBypass=" << (m_aftCoorProcess.m_bypassLock ? "1" : "0") << "\n";
}

void StylusPipeline::LoadConfig(const std::string& key, const std::string& value) {
    auto toBool = [&](const std::string& v) { return ParseConfigBool(key, v); };

    try {
    if (key == "sp.preEnabled" || key == "sp.frameParserEnabled") {
        m_frameParser.m_enabled = toBool(value);
    } else if (key == "sp.solveEnabled" || key == "sp.peakDetectorEnabled") {
        m_featureExtractor.m_enabled = toBool(value);
    } else if (key == "sp.coordinateSolverEnabled") {
        m_coordinateSolver.m_enabled = toBool(value);
    } else if (key == "sp.tiltProcessEnabled") {
        m_tiltProcess.m_enabled = toBool(value);
        if (!m_tiltProcess.m_enabled) {
            m_tiltProcess.Reset();
        }
    } else if (key == "sp.pressureSolverEnabled") {
        m_pressureSolver.m_enabled = toBool(value);
    } else if (key == "sp.postPressureEnabled") {
        m_postPressure.m_enabled = toBool(value);
        if (!m_postPressure.m_enabled) {
            m_postPressure.Reset();
        }
    } else if (key == "sp.fakePressureDecreaseEnabled") {
        m_postPressure.m_fakePressureDecreaseEnabled = toBool(value);
    } else if (key == "sp.btFreqShiftDebounceFrames") {
        m_postPressure.m_btFreqShiftDebounceFrames = std::clamp(ParseConfigInt(key, value), 0, 255);
    } else if (key == "sp.pressureEdgeEnterThreshold") {
        m_postPressure.m_pressureEdgeEnterThreshold =
            static_cast<uint16_t>(std::clamp(ParseConfigInt(key, value), 0, 0xFFFF));
    } else if (key == "sp.pressureEdgeExitThreshold") {
        m_postPressure.m_pressureEdgeExitThreshold =
            static_cast<uint16_t>(std::clamp(ParseConfigInt(key, value), 0, 0xFFFF));
    } else if (key == "sp.tipDownPressureThreshold") {
        m_pressureSolver.m_tipDownPressureThreshold = static_cast<uint16_t>(std::clamp(ParseConfigInt(key, value), 0, 4095));
    } else if (key == "sp.btPressSignalSuppressEnterThreshold") {
        m_pressureSolver.m_btPressSignalSuppressEnterThreshold =
            static_cast<uint16_t>(std::clamp(ParseConfigInt(key, value), 0, 0xFFFF));
    } else if (key == "sp.btPressSignalSuppressExitThreshold") {
        m_pressureSolver.m_btPressSignalSuppressExitThreshold =
            static_cast<uint16_t>(std::clamp(ParseConfigInt(key, value), 0, 0xFFFF));
    } else if (key == "sp.signalFloor") {
        m_coordinateSolver.m_signalFloor = static_cast<uint16_t>(std::clamp(ParseConfigInt(key, value), 0, 0xFFFF));
    } else if (key == "sp.edgeCoorEnabled") {
        m_edgeCoorProcess.m_enabled = toBool(value);
        if (!m_edgeCoorProcess.m_enabled) {
            m_edgeCoorProcess.Reset();
        }
    } else if (key == "sp.edgeCoorPostEnabled") {
        m_edgeCoorPostProcess.m_enabled = toBool(value);
    } else if (key == "sp.noisePostEnabled") {
        m_noisePostProcess.m_enabled = toBool(value);
        if (!m_noisePostProcess.m_enabled) {
            m_noisePostProcess.Reset();
        }
    } else if (key == "sp.noiseSignalRatioThold") {
        m_noisePostProcess.m_signalRatioThreshold =
            static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 1, 16));
    } else if (key == "sp.noiseSignalDropRatio") {
        m_noisePostProcess.m_signalMagnitudeDropRatio =
            static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 1, 16));
    } else if (key == "sp.linearFilterEnabled") {
        m_linearFilterProcess.m_enabled = toBool(value);
        if (!m_linearFilterProcess.m_enabled) {
            m_linearFilterProcess.Reset();
        }
    } else if (key == "sp.coorReviseEnabled") {
        m_coorReviseProcess.m_enabled = toBool(value);
        if (!m_coorReviseProcess.m_enabled) {
            m_coorReviseProcess.Reset();
        }
    } else if (key == "sp.coorReviseFactorDim1") {
        m_coorReviseProcess.m_factorDim1 = std::clamp(ParseConfigInt(key, value), 0, 255);
    } else if (key == "sp.coorReviseFactorDim2") {
        m_coorReviseProcess.m_factorDim2 = std::clamp(ParseConfigInt(key, value), 0, 255);
    } else if (key == "sp.coorSpeedEnabled") {
        m_coorSpeedProcess.m_enabled = toBool(value);
        if (!m_coorSpeedProcess.m_enabled) {
            m_coorSpeedProcess.Reset();
        }
    } else if (key == "sp.iirFilterEnabled") {
        m_coorIIRProcess.m_enabled = toBool(value);
        if (!m_coorIIRProcess.m_enabled) {
            m_coorIIRProcess.Reset();
        }
    } else if (key == "sp.iirCoefLowInBand") {
        m_coorIIRProcess.m_coefLowInBand = static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 0, 255));
    } else if (key == "sp.iirCoefHighInBand") {
        m_coorIIRProcess.m_coefHighInBand = static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 0, 255));
    } else if (key == "sp.iirSpeedTholdInBand") {
        m_coorIIRProcess.m_speedTholdInBand = static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 0, 255));
    } else if (key == "sp.iirCoefLowEdge") {
        m_coorIIRProcess.m_coefLowEdge = static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 0, 255));
    } else if (key == "sp.iirCoefHighEdge") {
        m_coorIIRProcess.m_coefHighEdge = static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 0, 255));
    } else if (key == "sp.iirSpeedTholdEdge") {
        m_coorIIRProcess.m_speedTholdEdge = static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 0, 255));
    } else if (key == "sp.iirSpeedMax") {
        m_coorIIRProcess.m_speedMax = std::clamp(ParseConfigInt(key, value), 0, 1000);
    } else if (key == "sp.iirMaxCoef") {
        m_coorIIRProcess.m_maxCoef = static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 0, 255));
    } else if (key == "sp.aftCoorEnabled") {
        m_aftCoorProcess.m_enabled = toBool(value);
        if (!m_aftCoorProcess.m_enabled) {
            m_aftCoorProcess.Reset();
        }
    } else if (key == "sp.lockFlashInBandX") {
        m_aftCoorProcess.m_lockFlashInBandX = static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 0, 255));
    } else if (key == "sp.lockFlashInBandY") {
        m_aftCoorProcess.m_lockFlashInBandY = static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 0, 255));
    } else if (key == "sp.lockFlashEdgeX") {
        m_aftCoorProcess.m_lockFlashEdgeX = static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 0, 255));
    } else if (key == "sp.lockFlashEdgeY") {
        m_aftCoorProcess.m_lockFlashEdgeY = static_cast<uint8_t>(std::clamp(ParseConfigInt(key, value), 0, 255));
    } else if (key == "sp.lockSensorTxCount") {
        m_aftCoorProcess.m_sensorTxCount = std::max(1, ParseConfigInt(key, value));
    } else if (key == "sp.lockSensorRxCount") {
        m_aftCoorProcess.m_sensorRxCount = std::max(1, ParseConfigInt(key, value));
    } else if (key == "sp.lockBypass") {
        m_aftCoorProcess.m_bypassLock = toBool(value);
    }
    } catch (const ConfigParseError& error) {
        LogConfigParseWarning("StylusPipeline", __func__, key, value, error);
    }
}

#endif

void StylusPipeline::SetBtMcuPressure(uint16_t pressure) {
    StylusBtInputSnapshot next{};
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
    StylusBtInputSnapshot next{};
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

void StylusPipeline::ReadLatestBtSample(StylusBtInputSnapshot& out) const {
    std::lock_guard<std::mutex> lk(m_btMutex);
    out = m_btSample;
}

} // namespace Solvers
