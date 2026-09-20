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

    /// 握手是否已经走到 0x7D01 之后。被动重握手依赖 MCU 自发事件推进，
    /// 调用方据此判断是否需要升级为强制重连。
    bool IsHandshakeComplete() const noexcept { return m_phase == Phase::Running; }

    /// 主机休眠/恢复后重新武装握手序列，但不产生任何主动发包动作。
    /// 与 OnConnected() 的区别：调用方不重开 transport，也不注入 0x7101/0x7701，
    /// 完全依赖 MCU 自身的事件链（0x77 PenScreenStatus → 0x7B PenRepParam）
    /// 把会话重新推回 0x7D01 InitProtocolParams。
    void RearmForLinkRecovery() noexcept {
        m_phase = Phase::AwaitingMcuStatus;
        m_initParamSent = false;
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
