#include "StylusPipeline.h"

#include <algorithm>

namespace Solvers {

bool StylusPipeline::Process(HeatmapFrame& frame) {
    frame.stylus.ResetRuntime();
    const StylusBtInputSnapshot bt = ReadLatestBtSample();
    frame.stylus.SnapshotBtInput(bt.pressure, bt.seq, bt.hasSample);

    m_frameParser.Process(frame);
    if (frame.stylus.runtime.flow.terminal) {
        m_commit.Commit(frame);
        return true;
    }

    m_cmf.Process(frame);
    m_peakDetector.Process(frame);
    if (frame.stylus.runtime.flow.terminal) {
        m_commit.Commit(frame);
        return true;
    }

    m_coordinateSolver.Process(frame);
    if (frame.stylus.runtime.flow.terminal) {
        m_commit.Commit(frame);
        return true;
    }

    m_pressureSolver.Process(frame);
    m_post.Process(frame);
    m_commit.Commit(frame);
    return true;
}

std::vector<ConfigParam> StylusPipeline::GetConfigSchema() const {
    std::vector<ConfigParam> schema;
    schema.emplace_back("sp.frameParserEnabled", "Frame Parser Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_frameParser.m_enabled))
        .Module("Frame Parser");
    schema.emplace_back("sp.cmfEnabled", "Common Mode Filter Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_cmf.m_enabled))
        .Module("Signal Conditioning");
    schema.emplace_back("sp.peakDetectorEnabled", "Peak Detector Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_peakDetector.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.coordinateSolverEnabled", "Coordinate Solver Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_coordinateSolver.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.pressureSolverEnabled", "Pressure Solver Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_pressureSolver.m_enabled))
        .Module("Data Solve");
    schema.emplace_back("sp.postEnabled", "Post Process Enabled",
                        ConfigParam::Bool, const_cast<bool*>(&m_post.m_enabled))
        .Module("Post Process");
    schema.emplace_back("sp.enableSlaveChecksum", "Enable Slave Checksum",
                        ConfigParam::Bool, const_cast<bool*>(&m_frameParser.m_enableSlaveChecksum))
        .Module("Frame Parser");
    schema.emplace_back("sp.filterMode", "Filter Mode",
                        ConfigParam::Int, const_cast<int*>(&m_post.m_filterMode), 0.0f, 2.0f)
        .Module("Post Process");
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
    schema.emplace_back("sp.signalFloor", "Signal Floor",
                        ConfigParam::Int, const_cast<uint16_t*>(&m_coordinateSolver.m_signalFloor), 0.0f, 65535.0f)
        .Module("Coordinate");
    return schema;
}

void StylusPipeline::SaveConfig(std::ostream& out) const {
    out << "sp.frameParserEnabled=" << (m_frameParser.m_enabled ? "1" : "0") << "\n";
    out << "sp.cmfEnabled=" << (m_cmf.m_enabled ? "1" : "0") << "\n";
    out << "sp.peakDetectorEnabled=" << (m_peakDetector.m_enabled ? "1" : "0") << "\n";
    out << "sp.coordinateSolverEnabled=" << (m_coordinateSolver.m_enabled ? "1" : "0") << "\n";
    out << "sp.pressureSolverEnabled=" << (m_pressureSolver.m_enabled ? "1" : "0") << "\n";
    out << "sp.postEnabled=" << (m_post.m_enabled ? "1" : "0") << "\n";
    out << "sp.enableSlaveChecksum=" << (m_frameParser.m_enableSlaveChecksum ? "1" : "0") << "\n";
    out << "sp.filterMode=" << m_post.m_filterMode << "\n";
    out << "sp.packetSensorRows=" << m_packetSensorRows << "\n";
    out << "sp.packetSensorCols=" << m_packetSensorCols << "\n";
    out << "sp.emitPacketWhenInvalid=" << (m_emitPacketWhenInvalid ? "1" : "0") << "\n";
    out << "sp.tipDownPressureThreshold=" << m_pressureSolver.m_tipDownPressureThreshold << "\n";
    out << "sp.signalFloor=" << m_coordinateSolver.m_signalFloor << "\n";
}

void StylusPipeline::LoadConfig(const std::string& key, const std::string& value) {
    auto toBool = [](const std::string& v) { return v == "1" || v == "true"; };

    if (key == "sp.preEnabled" || key == "sp.frameParserEnabled") {
        m_frameParser.m_enabled = toBool(value);
    } else if (key == "sp.cmfEnabled") {
        m_cmf.m_enabled = toBool(value);
    } else if (key == "sp.solveEnabled" || key == "sp.peakDetectorEnabled") {
        m_peakDetector.m_enabled = toBool(value);
    } else if (key == "sp.coordinateSolverEnabled") {
        m_coordinateSolver.m_enabled = toBool(value);
    } else if (key == "sp.pressureSolverEnabled") {
        m_pressureSolver.m_enabled = toBool(value);
    } else if (key == "sp.postEnabled") {
        m_post.m_enabled = toBool(value);
    } else if (key == "sp.enableSlaveChecksum") {
        m_frameParser.m_enableSlaveChecksum = toBool(value);
    } else if (key == "sp.filterMode") {
        SetFilterMode(std::stoi(value));
    } else if (key == "sp.packetSensorRows") {
        m_packetSensorRows = std::max(1, std::stoi(value));
    } else if (key == "sp.packetSensorCols") {
        m_packetSensorCols = std::max(1, std::stoi(value));
    } else if (key == "sp.emitPacketWhenInvalid") {
        m_emitPacketWhenInvalid = toBool(value);
    } else if (key == "sp.tipDownPressureThreshold") {
        m_pressureSolver.m_tipDownPressureThreshold = static_cast<uint16_t>(std::clamp(std::stoi(value), 0, 4095));
    } else if (key == "sp.signalFloor") {
        m_coordinateSolver.m_signalFloor = static_cast<uint16_t>(std::clamp(std::stoi(value), 0, 0xFFFF));
    }
}

void StylusPipeline::SetBtMcuPressure(uint16_t pressure) {
    std::lock_guard<std::mutex> lk(m_btMutex);
    m_btSample.pressure = pressure;
    m_btSample.seq += 1;
    m_btSample.hasSample = true;
}

void StylusPipeline::SetFilterMode(int mode) {
    m_post.m_filterMode = std::clamp(
        mode,
        static_cast<int>(Stylus::StylusPostProcessor::IirQ8),
        static_cast<int>(Stylus::StylusPostProcessor::Bypass));
}

StylusBtInputSnapshot StylusPipeline::ReadLatestBtSample() const {
    std::lock_guard<std::mutex> lk(m_btMutex);
    return m_btSample;
}

} // namespace Solvers
