#pragma once

#include "SolverTypes.h"

namespace Solvers::Stylus {

class CommonModeFilter {
public:
    bool m_enabled = true;

    inline bool Process(HeatmapFrame& frame) const {
        frame.stylus.runtime.flow.pipelineStage = 2;
        return true;
    }
};

} // namespace Solvers::Stylus
