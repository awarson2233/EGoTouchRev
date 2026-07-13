#pragma once

#include "btmcu/PenUsbTypes.h"

namespace Himax::Pen {

enum class PenUsbInitAction : uint8_t {
    None = 0,
    SendInitialQueries,
    SendSecondMcuStatusQuery,
    SendFactoryInitProtocolParams,
};

class PenUsbInitSession {
public:
    PenUsbInitAction OnConnected() noexcept {
        m_phase = Phase::AwaitingMcuStatus;
        m_initParamSent = false;
        return PenUsbInitAction::SendInitialQueries;
    }

    PenUsbInitAction OnEvent(PenUsbEventCode code) noexcept {
        switch (m_phase) {
        case Phase::AwaitingPenStatus:
            return PenUsbInitAction::None;

        case Phase::AwaitingMcuStatus:
            if (code == PenUsbEventCode::PenScreenStatus) {
                m_phase = Phase::AwaitingInitParamRequest;
                return PenUsbInitAction::SendSecondMcuStatusQuery;
            }
            return PenUsbInitAction::None;

        case Phase::AwaitingInitParamRequest:
            if (code == PenUsbEventCode::PenRepParam && !m_initParamSent) {
                m_initParamSent = true;
                m_phase = Phase::Running;
                return PenUsbInitAction::SendFactoryInitProtocolParams;
            }
            return PenUsbInitAction::None;

        case Phase::Running:
            return PenUsbInitAction::None;
        }
        return PenUsbInitAction::None;
    }

private:
    enum class Phase : uint8_t {
        AwaitingPenStatus = 0,
        AwaitingMcuStatus,
        AwaitingInitParamRequest,
        Running,
    };

    Phase m_phase = Phase::AwaitingPenStatus;
    bool m_initParamSent = false;
};

} // namespace Himax::Pen
