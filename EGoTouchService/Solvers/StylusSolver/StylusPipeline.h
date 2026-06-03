#pragma once

#include "ConfigSchema.h"
#include "hpp3/Hpp3Pipeline.h"
#include "shared/StylusFrameParser.hpp"
#include "StylusRuntimeCommit.hpp"
#include "SolverTypes.h"

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

    // ── Shared / protocol-agnostic stages ──
    Stylus::StylusFrameParser    m_frameParser;     // shared/
    Stylus::StylusRuntimeCommit  m_commit;           // root

    // ── Protocol-specific sub-pipelines ──
    Stylus::Hpp3::Pipeline       m_hpp3;             // hpp3/

private:
    void FinalizeTerminalFrame(HeatmapFrame& frame);
    void ReadLatestBtSample(StylusBtInputSnapshot& out) const;

    bool m_lastFrameWasTerminal = false;

    // NOTE: In the current architecture, all callers are serialized by
    // DeviceRuntime::m_pipelineMu. This mutex provides defense-in-depth
    // if the pipeline is ever used outside that lock.
    mutable std::mutex m_btMutex;
    StylusBtInputSnapshot m_btSample{};

    static constexpr int kPacketSensorRows = 40;
    static constexpr int kPacketSensorCols = 60;
};

} // namespace Solvers
