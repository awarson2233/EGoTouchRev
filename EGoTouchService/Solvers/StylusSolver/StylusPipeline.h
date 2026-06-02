#pragma once

#include "ConfigSchema.h"
#include "AftCoorProcess.hpp"
#include "CoorIIRProcess.hpp"
#include "CoorReviseProcess.hpp"
#include "CoorSpeedProcess.hpp"
#include "CoordinateSolver.hpp"
#include "EdgeCoorPostProcess.hpp"
#include "EdgeCoorProcess.hpp"
#include "GridFeatureExtractor.hpp"
#include "Hpp3NoisePostProcess.hpp"
#include "Hpp3PostPressureProcess.hpp"
#include "LinearFilterProcess.hpp"
#include "PressureSolver.hpp"
#include "SolverTypes.h"
#include "StylusFrameParser.hpp"
#include "StylusRuntimeCommit.hpp"
#include "TiltProcess.hpp"

#include <array>
#include <mutex>
#include <iosfwd>    // std::ostream 前向声明
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
                                const std::array<uint16_t, 4>& rawPressure,
                                uint8_t freq1,
                                uint8_t freq2);

    int GetPacketSensorRows() const { return kPacketSensorRows; }
    int GetPacketSensorCols() const { return kPacketSensorCols; }
    bool GetEmitPacketWhenInvalid() const { return true; }

    Stylus::StylusFrameParser m_frameParser;
    Stylus::GridFeatureExtractor m_featureExtractor;
    Stylus::CoordinateSolver m_coordinateSolver;
    Stylus::TiltProcess m_tiltProcess;
    Stylus::PressureSolver m_pressureSolver;
    Stylus::Hpp3PostPressureProcess m_postPressure;
    Stylus::EdgeCoorProcess m_edgeCoorProcess;
    Stylus::EdgeCoorPostProcess m_edgeCoorPostProcess;
    Stylus::Hpp3NoisePostProcess m_noisePostProcess;
    Stylus::LinearFilterProcess m_linearFilterProcess;
    Stylus::CoorReviseProcess m_coorReviseProcess;
    Stylus::CoorSpeedProcess m_coorSpeedProcess;
    Stylus::CoorIIRProcess m_coorIIRProcess;
    Stylus::AftCoorProcess m_aftCoorProcess;
    Stylus::StylusRuntimeCommit m_commit;

private:
    void ReadLatestBtSample(StylusBtInputSnapshot& out) const;

    // NOTE: In the current architecture, all callers are serialized by
    // DeviceRuntime::m_pipelineMu. This mutex provides defense-in-depth
    // if the pipeline is ever used outside that lock.
    mutable std::mutex m_btMutex;
    StylusBtInputSnapshot m_btSample{};

    static constexpr int kPacketSensorRows = 40;
    static constexpr int kPacketSensorCols = 60;
};

} // namespace Solvers
