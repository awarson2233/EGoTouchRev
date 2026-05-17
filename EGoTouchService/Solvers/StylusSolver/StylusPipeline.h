#pragma once

#include "ConfigSchema.h"
#include "CoordinateSolver.hpp"
#include "GridFeatureExtractor.hpp"
#include "Hpp3PostPressureProcess.hpp"
#include "PressureSolver.hpp"
#include "TiltProcess.hpp"
#include "SolverTypes.h"
#include "StylusFrameParser.hpp"
#include "StylusRuntimeCommit.hpp"

#include <array>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

namespace Solvers {

class StylusPipeline : public IConfigProvider {
public:
    StylusPipeline() = default;

    bool Process(HeatmapFrame& frame);

    std::vector<ConfigParam> GetConfigSchema() const override;
    void SaveConfig(std::ostream& out) const override;
    void LoadConfig(const std::string& key, const std::string& value) override;

    void SetBtMcuPressure(uint16_t pressure);
    void SetBtMcuPressurePacket(const std::array<uint16_t, 4>& pressure,
                                uint8_t freq1,
                                uint8_t freq2);

    int GetPacketSensorRows() const { return m_packetSensorRows; }
    int GetPacketSensorCols() const { return m_packetSensorCols; }
    bool GetEmitPacketWhenInvalid() const { return m_emitPacketWhenInvalid; }

    Stylus::StylusFrameParser m_frameParser;
    Stylus::GridFeatureExtractor m_featureExtractor;
    Stylus::CoordinateSolver m_coordinateSolver;
    Stylus::TiltProcess m_tiltProcess;
    Stylus::PressureSolver m_pressureSolver;
    Stylus::Hpp3PostPressureProcess m_postPressure;
    Stylus::StylusRuntimeCommit m_commit;

private:
    StylusBtInputSnapshot ReadLatestBtSample() const;

    mutable std::mutex m_btMutex;
    StylusBtInputSnapshot m_btSample{};

    int m_packetSensorRows = 40;
    int m_packetSensorCols = 60;
    bool m_emitPacketWhenInvalid = true;
};

} // namespace Solvers
