#include "LinearFilter.h"
#include <cmath>
#include <algorithm>

namespace Asa {

void LinearFilter::Reset() {
    m_state = State::Init;
    m_bufCount = 0;
    m_fit = LineFit{};
    m_output = AsaCoorResult{};
}

int LinearFilter::GetState() const {
    return static_cast<int>(m_state);
}

void LinearFilter::PushPoint(const AsaCoorResult& c) {
    if (m_bufCount < kMaxBufLen) {
        m_buf[static_cast<size_t>(m_bufCount)] = {c.dim1, c.dim2};
        m_bufCount++;
    } else {
        // Shift buffer (drop oldest)
        for (int i = 0; i < kMaxBufLen - 1; ++i) {
            m_buf[static_cast<size_t>(i)] =
                m_buf[static_cast<size_t>(i + 1)];
        }
        m_buf[kMaxBufLen - 1] = {c.dim1, c.dim2};
    }
}

// ── Least-squares line fit ──
// Mirrors UpdateStraightLinePrmt: fits y = Ax + B (or x = Ay + B for steep lines)
LinearFilter::LineFit LinearFilter::FitLine(int startIdx, int count) const {
    LineFit result{};
    if (count < 3) return result;

    const int end = std::min(startIdx + count, m_bufCount);
    const int n = end - startIdx;
    if (n < 3) return result;

    // Try both orientations, pick the one with smaller residual
    auto fitOneDim = [&](bool yAsDependent) -> LineFit {
        LineFit fit{};
        fit.useYasX = !yAsDependent;

        double sumX = 0, sumY = 0, sumXX = 0, sumXY = 0;
        for (int i = startIdx; i < end; ++i) {
            const auto& p = m_buf[static_cast<size_t>(i)];
            double x = yAsDependent ? p.x : p.y;
            double y = yAsDependent ? p.y : p.x;
            sumX += x; sumY += y;
            sumXX += x * x; sumXY += x * y;
        }

        const double dn = static_cast<double>(n);
        const double det = dn * sumXX - sumX * sumX;
        if (std::abs(det) < 1e-10) return fit;

        fit.slope = static_cast<float>((dn * sumXY - sumX * sumY) / det);
        fit.intercept = static_cast<float>((sumY - fit.slope * sumX) / dn);

        // Compute residual and max deviation
        double sumErr2 = 0;
        float maxDev = 0;
        for (int i = startIdx; i < end; ++i) {
            const auto& p = m_buf[static_cast<size_t>(i)];
            double x = yAsDependent ? p.x : p.y;
            double y = yAsDependent ? p.y : p.x;
            double predicted = fit.slope * x + fit.intercept;
            double err = y - predicted;
            sumErr2 += err * err;
            float absErr = static_cast<float>(std::abs(err));
            if (absErr > maxDev) maxDev = absErr;
        }

        fit.residual = static_cast<float>(sumErr2 / dn);
        fit.maxDev = maxDev;
        fit.valid = true;
        return fit;
    };

    LineFit fitA = fitOneDim(true);   // y = Ax + B
    LineFit fitB = fitOneDim(false);  // x = Ay + B

    // Pick orientation with smaller residual
    if (fitA.valid && fitB.valid)
        return (fitA.residual <= fitB.residual) ? fitA : fitB;
    return fitA.valid ? fitA : fitB;
}

// ── Constrain coordinate to the fitted line ──
AsaCoorResult LinearFilter::ConstrainToLine(const AsaCoorResult& c) const {
    if (!m_fit.valid) return c;

    AsaCoorResult out = c;
    const float strength = std::clamp(perpConstraint, 0.0f, 1.0f);

    if (!m_fit.useYasX) {
        // Line: y = A*x + B → constrain Y
        float predicted = m_fit.slope * static_cast<float>(c.dim1) +
                         m_fit.intercept;
        float actual = static_cast<float>(c.dim2);
        float filtered = actual + strength * (predicted - actual);
        out.dim2 = static_cast<int32_t>(std::lround(filtered));
    } else {
        // Line: x = A*y + B → constrain X
        float predicted = m_fit.slope * static_cast<float>(c.dim2) +
                         m_fit.intercept;
        float actual = static_cast<float>(c.dim1);
        float filtered = actual + strength * (predicted - actual);
        out.dim1 = static_cast<int32_t>(std::lround(filtered));
    }
    return out;
}

void LinearFilter::ProcessCurveLine(const AsaCoorResult& c) {
    if (m_bufCount < minFitLength) return;

    // Fit line on recent points
    int fitStart = std::max(0, m_bufCount - minFitLength);
    m_fit = FitLine(fitStart, minFitLength);

    if (m_fit.valid && m_fit.residual < enterResidualThreshold) {
        // Line detected → enter straight mode
        m_state = State::EnterStraight;
    }
}

void LinearFilter::ProcessEnterStraight(const AsaCoorResult& c) {
    // Apply partial constraint during transition
    m_output = ConstrainToLine(c);
    m_state = State::StraightLine;
}

void LinearFilter::ProcessStraightLine(const AsaCoorResult& c) {
    // Re-fit with large window
    int fitLen = std::min(m_bufCount, kMaxBufLen);
    int fitStart = std::max(0, m_bufCount - fitLen);
    m_fit = FitLine(fitStart, fitLen);

    if (!m_fit.valid || m_fit.maxDev > exitDeviation) {
        // Line broken → exit
        m_state = State::ExitStraight;
        return;
    }

    // Apply full constraint
    m_output = ConstrainToLine(c);
}

void LinearFilter::ProcessExitStraight(const AsaCoorResult& c) {
    // Transition back to curve mode
    m_state = State::CurveLine;
    m_output = c;
}

// ── Main state machine entry ──
AsaCoorResult LinearFilter::Process(
        const AsaCoorResult& coor, uint16_t pressure) {
    if (!enabled) return coor;

    // Pen up or invalid → reset state machine
    if (pressure == 0 || !coor.valid) {
        Reset();
        return coor;
    }

    m_output = coor;

    // Push to rolling buffer
    PushPoint(coor);

    // State machine
    switch (m_state) {
    case State::Init:
        m_state = State::Wait;
        break;
    case State::Wait:
        m_state = State::Collect;
        break;
    case State::Collect:
        if (m_bufCount >= collectFrames)
            m_state = State::CurveLine;
        break;
    case State::CurveLine:
        ProcessCurveLine(coor);
        break;
    case State::EnterStraight:
        ProcessEnterStraight(coor);
        break;
    case State::StraightLine:
        ProcessStraightLine(coor);
        break;
    case State::ExitStraight:
        ProcessExitStraight(coor);
        break;
    default:
        m_state = State::CurveLine;
        break;
    }

    return m_output;
}

} // namespace Asa
