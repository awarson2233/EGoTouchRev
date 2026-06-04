#pragma once

#include "SolverTypes.h"
#include "Hpp2Runtime.hpp"
#include "Hpp2PressureProcess.hpp"

namespace Solvers::Stylus::Hpp2 {

class Hpp2StaticStatusProcess {
public:
    void Process(Context& ctx) const {
        auto& runtime = ctx.runtime;
        runtime.decision.tipDownCandidate =
            runtime.decision.inRangeCandidate && runtime.pressure.outputPressure != 0;
        runtime.decision.authoritativeDown = runtime.decision.tipDownCandidate;
        Hpp2PressureProcess::PublishPressure(ctx.frame);
        ctx.state.m_prevPressure = runtime.pressure.outputPressure;
    }
};

} // namespace Solvers::Stylus::Hpp2
