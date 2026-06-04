#pragma once

#include "SolverTypes.h"
#include "Hpp2Runtime.hpp"
#include "StylusSolver/AsaTypes.hpp"

namespace Solvers::Stylus::Hpp2 {

class Hpp2RangeStatusProcess {
public:
    bool Process(Context& ctx) const {
        auto& runtime = ctx.runtime;
        const auto& hpp2 = runtime;
        const bool inRange = hpp2.selectedPeakDim1 != kInvalidPeak && hpp2.selectedPeakDim2 != kInvalidPeak;
        runtime.decision.inRangeCandidate = inRange;
        runtime.post.finalValid = inRange;
        if (!inRange) {
            // TSACore ASAStaticStatusProcess: distinguish release (was in-range → now out)
            // from no-signal/bypass (was already out → still out).
            if (ctx.state.m_wasInRange) {
                // Previously in-range, now out-of-range: release exit stylus (TSACore return 3).
                runtime.flow.frameClass = Asa::FrameClass::NoSignal;
            }
            // else: already out-of-range → no-report/bypass (TSACore return 5).
            // Both paths set m_wasInRange=false so the next frame starts fresh.
            ctx.state.m_wasInRange = false;
            return false;
        }
        ctx.state.m_wasInRange = true;
        return true;
    }
};

} // namespace Solvers::Stylus::Hpp2
