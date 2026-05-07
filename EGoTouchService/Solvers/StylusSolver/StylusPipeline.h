#pragma once

#include "ConfigSchema.h"
#include "CommonModeFilter.hpp"
#include "CoordinateSolver.hpp"
#include "GridPeakDetector.hpp"
#include "PressureSolver.hpp"
#include "SolverTypes.h"
#include "StylusFrameParser.hpp"
#include "StylusPostProcessor.hpp"
#include "StylusRuntimeCommit.hpp"

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

    int GetPacketSensorRows() const { return m_packetSensorRows; }
    int GetPacketSensorCols() const { return m_packetSensorCols; }
    bool GetEmitPacketWhenInvalid() const { return m_emitPacketWhenInvalid; }

    int GetFilterMode() const { return m_post.m_filterMode; }
    void SetFilterMode(int mode);

    Stylus::StylusFrameParser m_frameParser;
    Stylus::CommonModeFilter m_cmf;
    Stylus::GridPeakDetector m_peakDetector;
    Stylus::CoordinateSolver m_coordinateSolver;
    Stylus::PressureSolver m_pressureSolver;
    Stylus::StylusPostProcessor m_post;
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
