#include "StylusPipeline.h"

#include <algorithm>

namespace Solvers {

bool StylusPipeline::Process(HeatmapFrame& frame) {
    frame.stylus.ResetRuntime();
    const StylusBtInputSnapshot bt = ReadLatestBtSample();
    frame.stylus.input.btSample = bt;

    m_frameParser.Process(frame);
    if (frame.stylus.runtime.flow.terminal) {
        m_tiltProcess.Reset();
        m_postPressure.Reset();
        m_commit.Commit(frame);
        return true;
    }

    m_featureExtractor.Process(frame);
    if (frame.stylus.runtime.flow.terminal) {
        m_tiltProcess.Reset();
        m_postPressure.Reset();
        m_commit.Commit(frame);
        return true;
    }

    m_coordinateSolver.Process(frame);
    if (frame.stylus.runtime.flow.terminal) {
        m_tiltProcess.Reset();
        m_postPressure.Reset();
        m_commit.Commit(frame);
        return true;
    }

    m_tiltProcess.Process(frame);
    m_pressureSolver.Process(frame);
    m_postPressure.Process(frame);
    m_commit.Commit(frame);
    return true;
}

std::vector<ConfigParam> StylusPipeline::GetConfigSchema() const {
    std::vector<ConfigParam> schema;
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
                        ConfigParam::Int, const_cast<uint16_t*>(&m_postPressure.m_pressureEdgeEnterThreshold), 0.0f, 65535.0f)
        .Module("Pressure");
    schema.emplace_back("sp.pressureEdgeExitThreshold", "Pressure Edge Exit Threshold",
                        ConfigParam::Int, const_cast<uint16_t*>(&m_postPressure.m_pressureEdgeExitThreshold), 0.0f, 65535.0f)
        .Module("Pressure");
    schema.emplace_back("sp.packetSensorRows", "Packet Sensor Rows",
                        ConfigParam::Int, const_cast<int*>(&m_packetSensorRows), 1.0f, 200.0f)
        .Module("Output");
    schema.emplace_back("sp.packetSensorCols", "Packet Sensor Cols",
                        ConfigParam::Int, const_cast<int*>(&m_packetSensorCols), 1.0f, 200.0f)
        .Module("Output");
    schema.emplace_back("sp.emitPacketWhenInvalid", "Emit Invalid Packet",
                        ConfigParam::Bool, const_cast<bool*>(&m_emitPacketWhenInvalid))
        .Module("Output");
    schema.emplace_back("sp.tipDownPressureThreshold", "Tip Down Pressure Threshold",
                        ConfigParam::Int, const_cast<uint16_t*>(&m_pressureSolver.m_tipDownPressureThreshold), 0.0f, 4095.0f)
        .Module("Pressure");
    schema.emplace_back("sp.btPressSignalSuppressEnterThreshold", "BT Press Suppress Enter Threshold",
                        ConfigParam::Int, const_cast<uint16_t*>(&m_pressureSolver.m_btPressSignalSuppressEnterThreshold), 0.0f, 65535.0f)
        .Module("Pressure");
    schema.emplace_back("sp.btPressSignalSuppressExitThreshold", "BT Press Suppress Exit Threshold",
                        ConfigParam::Int, const_cast<uint16_t*>(&m_pressureSolver.m_btPressSignalSuppressExitThreshold), 0.0f, 65535.0f)
        .Module("Pressure");
    schema.emplace_back("sp.signalFloor", "Signal Floor",
                        ConfigParam::Int, const_cast<uint16_t*>(&m_coordinateSolver.m_signalFloor), 0.0f, 65535.0f)
        .Module("Coordinate");
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
    out << "sp.packetSensorRows=" << m_packetSensorRows << "\n";
    out << "sp.packetSensorCols=" << m_packetSensorCols << "\n";
    out << "sp.emitPacketWhenInvalid=" << (m_emitPacketWhenInvalid ? "1" : "0") << "\n";
    out << "sp.tipDownPressureThreshold=" << m_pressureSolver.m_tipDownPressureThreshold << "\n";
    out << "sp.btPressSignalSuppressEnterThreshold=" << m_pressureSolver.m_btPressSignalSuppressEnterThreshold << "\n";
    out << "sp.btPressSignalSuppressExitThreshold=" << m_pressureSolver.m_btPressSignalSuppressExitThreshold << "\n";
    out << "sp.signalFloor=" << m_coordinateSolver.m_signalFloor << "\n";
}

void StylusPipeline::LoadConfig(const std::string& key, const std::string& value) {
    auto toBool = [](const std::string& v) { return v == "1" || v == "true"; };

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
        m_postPressure.m_btFreqShiftDebounceFrames = std::clamp(std::stoi(value), 0, 255);
    } else if (key == "sp.pressureEdgeEnterThreshold") {
        m_postPressure.m_pressureEdgeEnterThreshold =
            static_cast<uint16_t>(std::clamp(std::stoi(value), 0, 0xFFFF));
    } else if (key == "sp.pressureEdgeExitThreshold") {
        m_postPressure.m_pressureEdgeExitThreshold =
            static_cast<uint16_t>(std::clamp(std::stoi(value), 0, 0xFFFF));
    } else if (key == "sp.packetSensorRows") {
        m_packetSensorRows = std::max(1, std::stoi(value));
    } else if (key == "sp.packetSensorCols") {
        m_packetSensorCols = std::max(1, std::stoi(value));
    } else if (key == "sp.emitPacketWhenInvalid") {
        m_emitPacketWhenInvalid = toBool(value);
    } else if (key == "sp.tipDownPressureThreshold") {
        m_pressureSolver.m_tipDownPressureThreshold = static_cast<uint16_t>(std::clamp(std::stoi(value), 0, 4095));
    } else if (key == "sp.btPressSignalSuppressEnterThreshold") {
        m_pressureSolver.m_btPressSignalSuppressEnterThreshold =
            static_cast<uint16_t>(std::clamp(std::stoi(value), 0, 0xFFFF));
    } else if (key == "sp.btPressSignalSuppressExitThreshold") {
        m_pressureSolver.m_btPressSignalSuppressExitThreshold =
            static_cast<uint16_t>(std::clamp(std::stoi(value), 0, 0xFFFF));
    } else if (key == "sp.signalFloor") {
        m_coordinateSolver.m_signalFloor = static_cast<uint16_t>(std::clamp(std::stoi(value), 0, 0xFFFF));
    }
}

void StylusPipeline::SetBtMcuPressure(uint16_t pressure) {
    std::lock_guard<std::mutex> lk(m_btMutex);
    m_btSample.pressure.fill(0);
    m_btSample.pressure[3] = pressure;
    m_btSample.freq1 = 0;
    m_btSample.freq2 = 0;
    m_btSample.seq += 1;
    m_btSample.hasSample = true;
    m_btSample.hasFreq = false;
}

void StylusPipeline::SetBtMcuPressurePacket(const std::array<uint16_t, 4>& pressure,
                                            uint8_t freq1,
                                            uint8_t freq2) {
    std::lock_guard<std::mutex> lk(m_btMutex);
    m_btSample.pressure = pressure;
    m_btSample.freq1 = freq1;
    m_btSample.freq2 = freq2;
    m_btSample.seq += 1;
    m_btSample.hasSample = true;
    m_btSample.hasFreq = true;
}

StylusBtInputSnapshot StylusPipeline::ReadLatestBtSample() const {
    std::lock_guard<std::mutex> lk(m_btMutex);
    return m_btSample;
}

} // namespace Solvers
