#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "StylusFrameState.hpp"

namespace Asa {

struct PenFrameEvidence {
    bool coordValid = false;
    bool noSignal = false;
    bool tx1BlockValid = false;
    bool sustainActive = false;
    bool activeStylusPresent = false;
    bool hoverSignalPresent = false;
    bool authoritativeDown = false;
    bool keepInkAlive = false;
    bool hoverPresent = false;
    bool immediateRelease = false;
    bool recheckPassed = true;
    bool overlapLike = false;
    bool edgeLike = false;
    bool exitSmoothCandidate = false;
    bool suppressPressureButKeepContact = false;
    bool btPressureResidual = false;
    bool edgeSignalLow = false;
    bool pressureIsReal = false;
    uint16_t mappedPressure = 0;
    uint16_t realPressure = 0;
    uint16_t realMeasuredPressure = 0;
    uint16_t pressureForContact = 0;
    uint16_t tx1Composite = 0;
    uint16_t tx2Composite = 0;
    int32_t curDim1 = 0;
    int32_t curDim2 = 0;
};

struct PenHistoryPoint {
    int32_t dim1 = 0;
    int32_t dim2 = 0;
};

struct PenOutputPolicy {
    uint16_t outputPressure = 0;
    uint16_t retainedPressure = 0;
    bool tipSwitchActive = false;
    bool noPressInkActive = false;
    bool sustainOutput = false;
    bool fastLiftOutput = false;
    bool retainPreviousOutput = false;
    bool keepPreviousCoordinate = false;
    bool keepInRangeOnReleaseFrame = false;
    bool applyExitEdgeSnap = false;
    bool releaseExitOutput = false;
    bool inkLeakageSuspect = false;
    bool pressureDecayActive = false;
};

struct MotionProfile {
    int   iirCoef = 8;
    int   iirDivisorN = 16;
    bool  skipIIR = false;
    bool  freezeOutput = false;

    int   jitterStrength = 2;

    bool  enableLinearFilter = false;
    bool  enableCoorReviser = true;

    float pressureAlpha = 0.25f;
    bool  pressureDecay = false;
    bool  pressureDecayActive = false;

    bool  isLongPress = false;
    bool  isTap = false;
};

struct PenUpdateResult {
    MotionProfile motion{};
    PenOutputPolicy output{};
};

class PenStateMachine {
public:
    enum class State : uint8_t {
        NoSignal = 0,
        Hover = 1,
        Writing = 2,
    };

    enum class Branch : uint8_t {
        None = 0,
        FastDown = 1,
        Sustain = 2,
        FastLift = 3,
    };

    static constexpr int kHistoryCapacity = 400;

    inline PenUpdateResult Process(const PenFrameEvidence& evidence) {
        return Update(evidence);
    }

    inline PenUpdateResult Process(Solvers::StylusFrameState& state,
                                   const PenFrameEvidence& evidence) {
        const auto result = Update(evidence);
        ApplyUpdateToState(state, result);
        return result;
    }

    inline MotionProfile Process(bool coordValid, uint16_t pressure,
                                 int32_t curDim1, int32_t curDim2) {
        return Update(coordValid, pressure, curDim1, curDim2);
    }

    inline void ApplyUpdateToState(Solvers::StylusFrameState& state,
                                   const PenUpdateResult& result) const {
        state.lifecycle.outputPressure = result.output.outputPressure;
        state.lifecycle.tipSwitchActive = result.output.tipSwitchActive;
        state.lifecycle.keepPreviousCoordinate = result.output.keepPreviousCoordinate;
        state.lifecycle.keepInRangeOnReleaseFrame = result.output.keepInRangeOnReleaseFrame;
        state.lifecycle.applyExitEdgeSnap = result.output.applyExitEdgeSnap;
        state.lifecycle.enableLinearFilter = result.motion.enableLinearFilter;
        state.lifecycle.enableCoorReviser = result.motion.enableCoorReviser;
        state.lifecycle.iirCoef = result.motion.iirCoef;
        state.lifecycle.iirDivisorN = result.motion.iirDivisorN;
        state.lifecycle.skipIIR = result.motion.skipIIR;
        state.lifecycle.jitterStrength = result.motion.jitterStrength;
        state.lifecycle.currentlyWriting = (m_state == State::Writing);
    }

    inline PenUpdateResult Update(const PenFrameEvidence& evidence) {
        float rawSpeedThisFrame = 0.0f;
        float rawVelDim1 = 0.0f;
        float rawVelDim2 = 0.0f;
        if (evidence.coordValid && m_hasLastValidCoord) {
            const float dx = static_cast<float>(evidence.curDim1 - m_lastValidDim1);
            const float dy = static_cast<float>(evidence.curDim2 - m_lastValidDim2);
            rawSpeedThisFrame = std::sqrt(dx * dx + dy * dy);
            rawVelDim1 = std::abs(dx);
            rawVelDim2 = std::abs(dy);
        }

        if (evidence.coordValid && m_hasLastValidCoord) {
            const float alpha = 2.0f / (static_cast<float>(speedSmoothWindow) + 1.0f);
            m_smoothedSpeed = alpha * rawSpeedThisFrame + (1.0f - alpha) * m_smoothedSpeed;
            m_smoothedVelDim1 = alpha * rawVelDim1 + (1.0f - alpha) * m_smoothedVelDim1;
            m_smoothedVelDim2 = alpha * rawVelDim2 + (1.0f - alpha) * m_smoothedVelDim2;
        } else {
            m_smoothedSpeed = rawSpeedThisFrame;
            m_smoothedVelDim1 = rawVelDim1;
            m_smoothedVelDim2 = rawVelDim2;
        }
        m_instantSpeed = rawSpeedThisFrame;

        const State prevState = m_state;
        const bool leavingWriting = (prevState == State::Writing);

        m_justLeftRange = false;
        if (m_state != State::NoSignal) {
            ++m_stateFrameCount;
        } else {
            m_stateFrameCount = 0;
        }
        if (m_branch != Branch::None) {
            ++m_branchFrameCount;
        } else {
            m_branchFrameCount = 0;
        }

        const bool hasCoord = evidence.coordValid;
        const bool hasSignal = !evidence.noSignal && evidence.tx1BlockValid;
        const bool contactLike = evidence.authoritativeDown;
        const bool activeLike = evidence.keepInkAlive || evidence.hoverPresent ||
                                evidence.activeStylusPresent || evidence.hoverSignalPresent ||
                                (hasSignal && hasCoord);
        const bool hoverLike = evidence.hoverPresent ||
                               (!contactLike && (evidence.hoverSignalPresent || (hasSignal && hasCoord)));
        const bool noSignalLike = !activeLike || !hasCoord;

        if (activeLike && evidence.recheckPassed) {
            m_activeEnterFrames = std::min(m_activeEnterFrames + 1,
                                           std::max(1, activeEnterDebounceFrames));
        } else {
            m_activeEnterFrames = 0;
        }

        if (!evidence.recheckPassed && activeLike) {
            m_weakRecheckFrames = std::min(m_weakRecheckFrames + 1,
                                           std::max(1, weakRecheckHoldFrames));
        } else {
            m_weakRecheckFrames = 0;
        }

        switch (m_state) {
        case State::NoSignal:
            if (contactLike && m_activeEnterFrames >= std::max(1, activeEnterDebounceFrames)) {
                EnterWriting(Branch::FastDown);
                m_releaseHoldRemaining = std::max(1, releaseHoldFrames);
            } else if (hoverLike) {
                EnterHover();
            }
            break;

        case State::Hover:
            if (contactLike && m_activeEnterFrames >= std::max(1, activeEnterDebounceFrames)) {
                EnterWriting(Branch::FastDown);
                m_releaseHoldRemaining = std::max(1, releaseHoldFrames);
            } else if (noSignalLike) {
                EnterNoSignal();
            }
            break;

        case State::Writing:
            if (evidence.immediateRelease) {
                m_releaseHoldRemaining = 0;
                if (hoverLike) {
                    EnterHover();
                } else {
                    EnterNoSignal();
                }
                break;
            }
            if (evidence.keepInkAlive) {
                m_releaseHoldRemaining = std::max(1, releaseHoldFrames);
                ++m_btFrameCount;
                if (m_branch == Branch::FastDown) {
                    const bool fastDownExpired =
                        (m_branchFrameCount >= std::max(1, fastDownFrames));
                    if (fastDownExpired) {
                        SetBranch(Branch::None);
                    }
                } else {
                    SetBranch(Branch::None);
                }
            } else {
                if (m_releaseHoldRemaining > 0) {
                    --m_releaseHoldRemaining;
                }
                if (hoverLike) {
                    EnterHover();
                } else if (!activeLike && m_releaseHoldRemaining == 0) {
                    EnterNoSignal();
                }
            }
            break;
        }

        if (m_state == State::Writing && evidence.keepInkAlive) {
            ++m_totalContactFrames;
        } else if (m_state != State::Writing) {
            m_totalContactFrames = 0;
        }

        if (m_state == State::Writing) {
            if (m_smoothedSpeed < stillSpeedThreshold) {
                ++m_lowSpeedFrames;
            } else {
                m_lowSpeedFrames = 0;
            }
        } else {
            m_lowSpeedFrames = 0;
        }

        if (prevState != State::NoSignal && m_state == State::NoSignal) {
            m_justLeftRange = true;
        }
        if (leavingWriting && m_state != State::Writing) {
            m_wasTap = (m_totalContactFrames <= tapMaxFrames);
        } else if (m_state == State::Writing) {
            m_wasTap = false;
        }

        PenUpdateResult result{};
        result.motion = BuildProfile();
        result.output = BuildOutputPolicy(evidence);
        PushHistory(evidence);
        return result;
    }

    inline MotionProfile Update(bool coordValid, uint16_t pressure,
                                int32_t curDim1, int32_t curDim2) {
        PenFrameEvidence evidence{};
        evidence.coordValid = coordValid;
        evidence.noSignal = !coordValid;
        evidence.tx1BlockValid = coordValid;
        evidence.realPressure = pressure;
        evidence.curDim1 = curDim1;
        evidence.curDim2 = curDim2;
        return Update(evidence).motion;
    }

    State GetState() const { return m_state; }
    Branch GetBranch() const { return m_branch; }
    uint8_t GetAnimState() const {
        if (m_state == State::NoSignal) return 0;
        if (m_state == State::Hover) return 1;
        return 2;
    }
    float GetInstantSpeed() const { return m_instantSpeed; }
    float GetSmoothedSpeed() const { return m_smoothedSpeed; }
    int   GetStateFrameCount() const { return m_stateFrameCount; }
    int   GetBranchFrameCount() const { return m_branchFrameCount; }
    int   GetTotalContactFrames() const { return m_totalContactFrames; }
    int   GetLowSpeedFrames() const { return m_lowSpeedFrames; }
    bool  JustLeftRange() const { return m_justLeftRange; }
    bool  WasTap() const { return m_wasTap; }
    int   GetHistoryCount() const { return m_historyCount; }
    int   GetValidHistoryCount() const { return m_validHistoryCount; }
    int   GetBtFrameCount() const { return m_btFrameCount; }

    inline bool TryGetHistoryPoint(int logicalIndex, PenHistoryPoint& out) const {
        if (logicalIndex < 0 || logicalIndex >= m_historyCount) {
            return false;
        }
        const auto& entry = HistoryAt(logicalIndex);
        if (!entry.coordValid) {
            return false;
        }
        out.dim1 = entry.dim1;
        out.dim2 = entry.dim2;
        return true;
    }

    inline void Reset() {
        m_state = State::NoSignal;
        m_branch = Branch::None;
        m_stateFrameCount = 0;
        m_branchFrameCount = 0;
        m_totalContactFrames = 0;
        m_lowSpeedFrames = 0;
        m_instantSpeed = 0.0f;
        m_smoothedSpeed = 0.0f;
        m_smoothedVelDim1 = 0.0f;
        m_smoothedVelDim2 = 0.0f;
        m_activeEnterFrames = 0;
        m_releaseHoldRemaining = 0;
        m_weakRecheckFrames = 0;
        m_fastLiftEdgeFrames = 0;
        m_justLeftRange = false;
        m_wasTap = false;
        m_lastCommittedPressure = 0;
        m_btFrameCount = 0;
        ClearHistory();
    }

    float stillSpeedThreshold = 3.0f;
    float speedLow = 3.0f;
    float speedHigh = 300.0f;

    int   speedSmoothWindow = 5;

    int   longPressFrames = 120;
    int   liftTimeout = 10;
    int   tapMaxFrames = 5;
    int   fastDownFrames = 2;

    int   movingIirLow = 4;
    int   movingIirHigh = 16;
    int   iirDivisorN = 16;

    bool  enableDirectionalHalve = true;
    float directionalVelThreshold = 1.0f;

    int   hoverIirCoef = 4;
    int   jitterMax = 3;
    int   noPressSyntheticMin = 10;
    int   activeEnterDebounceFrames = 1;
    int   releaseHoldFrames = 2;
    int   weakRecheckHoldFrames = 2;
    int   edgeFastLiftHoldFrames = 1;

private:
    struct HistoryEntry {
        bool coordValid = false;
        int32_t dim1 = 0;
        int32_t dim2 = 0;
    };

    State m_state = State::NoSignal;
    Branch m_branch = Branch::None;
    int   m_stateFrameCount = 0;
    int   m_branchFrameCount = 0;
    int   m_totalContactFrames = 0;
    int   m_lowSpeedFrames = 0;
    float m_instantSpeed = 0.0f;
    float m_smoothedSpeed = 0.0f;
    float m_smoothedVelDim1 = 0.0f;
    float m_smoothedVelDim2 = 0.0f;

    int   m_activeEnterFrames = 0;
    int   m_releaseHoldRemaining = 0;
    int   m_weakRecheckFrames = 0;
    int   m_fastLiftEdgeFrames = 0;

    bool  m_justLeftRange = false;
    bool  m_wasTap = false;

    bool  m_hasLastValidCoord = false;
    int32_t m_lastValidDim1 = 0;
    int32_t m_lastValidDim2 = 0;
    uint16_t m_lastCommittedPressure = 0;
    int   m_btFrameCount = 0;

    std::array<HistoryEntry, kHistoryCapacity> m_history{};
    int m_historyHead = 0;
    int m_historyCount = 0;
    int m_validHistoryCount = 0;

    inline void EnterHover() {
        m_state = State::Hover;
        m_branch = Branch::None;
        m_stateFrameCount = 0;
        m_branchFrameCount = 0;
        m_releaseHoldRemaining = 0;
        m_fastLiftEdgeFrames = 0;
    }

    inline void EnterWriting(Branch branch) {
        if (m_state != State::Writing) {
            m_totalContactFrames = 0;
            m_lowSpeedFrames = 0;
            m_btFrameCount = 0;
        }
        m_state = State::Writing;
        m_branch = branch;
        m_stateFrameCount = 0;
        m_branchFrameCount = 0;
    }

    inline void EnterNoSignal() {
        m_state = State::NoSignal;
        m_branch = Branch::None;
        m_stateFrameCount = 0;
        m_branchFrameCount = 0;
        m_lowSpeedFrames = 0;
        m_instantSpeed = 0.0f;
        m_smoothedSpeed = 0.0f;
        m_smoothedVelDim1 = 0.0f;
        m_smoothedVelDim2 = 0.0f;
        m_releaseHoldRemaining = 0;
        m_fastLiftEdgeFrames = 0;
        m_lastCommittedPressure = 0;
        ClearHistory();
    }

    inline void SetBranch(Branch branch) {
        if (m_branch == branch) return;
        m_branch = branch;
        m_branchFrameCount = 0;
    }

    inline void ClearHistory() {
        m_historyHead = 0;
        m_historyCount = 0;
        m_validHistoryCount = 0;
        m_hasLastValidCoord = false;
        m_lastValidDim1 = 0;
        m_lastValidDim2 = 0;
    }

    inline void PushHistory(const PenFrameEvidence& evidence) {
        HistoryEntry entry{};
        entry.coordValid = evidence.coordValid;
        entry.dim1 = evidence.curDim1;
        entry.dim2 = evidence.curDim2;

        if (m_historyCount < kHistoryCapacity) {
            const int insert = (m_historyHead + m_historyCount) % kHistoryCapacity;
            m_history[static_cast<size_t>(insert)] = entry;
            ++m_historyCount;
        } else {
            if (m_history[static_cast<size_t>(m_historyHead)].coordValid) {
                --m_validHistoryCount;
            }
            m_history[static_cast<size_t>(m_historyHead)] = entry;
            m_historyHead = (m_historyHead + 1) % kHistoryCapacity;
        }

        if (entry.coordValid) {
            ++m_validHistoryCount;
            m_hasLastValidCoord = true;
            m_lastValidDim1 = entry.dim1;
            m_lastValidDim2 = entry.dim2;
        }
    }

    inline const HistoryEntry& HistoryAt(int logicalIndex) const {
        const int idx = (m_historyHead + logicalIndex) % kHistoryCapacity;
        return m_history[static_cast<size_t>(idx)];
    }

    inline MotionProfile BuildProfile() const {
        MotionProfile p;
        p.iirDivisorN = iirDivisorN;

        switch (m_state) {
        case State::NoSignal:
            p.iirCoef = movingIirLow;
            p.skipIIR = true;
            p.jitterStrength = 0;
            p.enableLinearFilter = false;
            p.enableCoorReviser = false;
            p.pressureAlpha = 1.0f;
            p.pressureDecay = false;
            return p;

        case State::Hover:
            p.iirCoef = hoverIirCoef;
            p.skipIIR = false;
            p.jitterStrength = jitterMax;
            p.enableLinearFilter = false;
            p.enableCoorReviser = true;
            p.pressureAlpha = 1.0f;
            p.pressureDecay = false;
            return p;

        case State::Writing:
            CalcWritingProfile(p);
            if (m_branch == Branch::FastDown) {
                p.enableLinearFilter = false;
                p.jitterStrength = std::max(0, p.jitterStrength - 1);
                p.pressureAlpha = 1.0f;
            }
            return p;
        }

        return p;
    }

    inline PenOutputPolicy BuildOutputPolicy(const PenFrameEvidence& evidence) {
        PenOutputPolicy policy{};

        switch (m_state) {
        case State::NoSignal:
        case State::Hover:
            m_lastCommittedPressure = 0;
            return policy;

        case State::Writing:
            policy.outputPressure = evidence.realPressure;
            break;
        }

        policy.tipSwitchActive = (m_state == State::Writing);
        policy.noPressInkActive = false;
        policy.sustainOutput = false;
        policy.fastLiftOutput = false;

        if (policy.tipSwitchActive) {
            m_lastCommittedPressure = policy.outputPressure;
        }
        return policy;
    }

    inline void CalcWritingProfile(MotionProfile& p) const {
        const float effectiveSpeed = m_smoothedSpeed;

        int effIirLow = movingIirLow;
        int effIirHigh = movingIirHigh;
        if (enableDirectionalHalve &&
            (m_smoothedVelDim1 > directionalVelThreshold ||
             m_smoothedVelDim2 > directionalVelThreshold)) {
            effIirHigh = std::max(1, effIirHigh >> 1);
            effIirLow = std::max(1, effIirLow >> 1);
        }

        const float range = speedHigh - speedLow;
        const float t = (range > 0.0f)
            ? std::clamp((effectiveSpeed - speedLow) / range, 0.0f, 1.0f)
            : 0.0f;

        p.iirCoef = static_cast<int>(
            static_cast<float>(effIirLow) +
            t * static_cast<float>(effIirHigh - effIirLow) + 0.5f);
        p.skipIIR = false;
        p.freezeOutput = false;
        p.jitterStrength = static_cast<int>(
            static_cast<float>(jitterMax) * (1.0f - t) + 0.5f);
        p.enableLinearFilter = true;
        p.enableCoorReviser = true;
        p.pressureAlpha = 0.25f + 0.25f * t;
        p.pressureDecay = false;
        p.isLongPress = (m_lowSpeedFrames > longPressFrames);
        p.isTap = false;
    }
};

} // namespace Asa
