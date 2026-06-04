#pragma once

#include "SolverTypes.h"
#include "Hpp2Runtime.hpp"

namespace Solvers::Stylus::Hpp2 {

class Hpp2ButtonProcess {
public:
    void Process(Context& ctx) const {
        auto& hpp2 = ctx.runtime;
        if ((hpp2.buttonBits & 1u) != 0) {
            hpp2.buttonPressed = true;
            ctx.state.m_buttonReleaseCnt = 2;
        } else if (ctx.state.m_buttonReleaseCnt != 0) {
            hpp2.buttonPressed = true;
            --ctx.state.m_buttonReleaseCnt;
        } else {
            hpp2.buttonPressed = false;
        }
        hpp2.buttonReleaseFrames = ctx.state.m_buttonReleaseCnt;
    }
};

} // namespace Solvers::Stylus::Hpp2
