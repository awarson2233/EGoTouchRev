#include "TouchTracker.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Engine {

// =============================================================================
// Static helpers
// =============================================================================
float TouchTracker::DistanceSq(float x1, float y1, float x2, float y2) {
    const float dx = x1 - x2;
    const float dy = y1 - y2;
    return dx * dx + dy * dy;
}

bool TouchTracker::IsEdgeTouch(float x, float y, int cols, int rows, float edgeMargin) {
    return (x <= edgeMargin) ||
           (y <= edgeMargin) ||
           (x >= static_cast<float>(cols) - edgeMargin) ||
           (y >= static_cast<float>(rows) - edgeMargin);
}

// Hungarian assignment (Jonker-Volgenant)
std::vector<int> TouchTracker::SolveAssignment(
        const std::vector<std::vector<float>>& cost) {
    const int rowsOriginal = static_cast<int>(cost.size());
    if (rowsOriginal == 0) return {};
    const int colsOriginal = static_cast<int>(cost[0].size());
    if (colsOriginal == 0) return std::vector<int>(rowsOriginal, -1);

    bool transposed = false;
    std::vector<std::vector<float>> matrix = cost;
    int n = rowsOriginal, m = colsOriginal;
    if (n > m) {
        transposed = true;
        std::vector<std::vector<float>> tm(m, std::vector<float>(n, 0.0f));
        for (int r = 0; r < n; ++r)
            for (int c = 0; c < m; ++c) tm[c][r] = matrix[r][c];
        matrix = std::move(tm);
        std::swap(n, m);
    }
    const float kInf = std::numeric_limits<float>::max() / 8.0f;
    std::vector<float> u(n+1,0), v(m+1,0);
    std::vector<int> p(m+1,0), way(m+1,0);
    for (int i=1;i<=n;++i) {
        p[0]=i; int j0=0;
        std::vector<float> minv(m+1,kInf);
        std::vector<char> used(m+1,false);
        do {
            used[j0]=true;
            const int i0=p[j0]; float delta=kInf; int j1=0;
            for (int j=1;j<=m;++j) {
                if(used[j]) continue;
                const float cur=matrix[i0-1][j-1]-u[i0]-v[j];
                if(cur<minv[j]){minv[j]=cur;way[j]=j0;}
                if(minv[j]<delta){delta=minv[j];j1=j;}
            }
            for (int j=0;j<=m;++j) {
                if(used[j]){u[p[j]]+=delta;v[j]-=delta;}
                else minv[j]-=delta;
            }
            j0=j1;
        } while(p[j0]!=0);
        do { const int j1=way[j0]; p[j0]=p[j1]; j0=j1; } while(j0!=0);
    }
    std::vector<int> rowToCol(n,-1);
    for (int j=1;j<=m;++j)
        if(p[j]!=0) rowToCol[p[j]-1]=j-1;
    if (!transposed) return rowToCol;
    std::vector<int> result(rowsOriginal, -1);
    for (int prev = 0; prev < colsOriginal; ++prev) {
        const int cur = rowToCol[prev];
        if (cur >= 0 && cur < rowsOriginal) result[cur] = prev;
    }
    return result;
}

float TouchTracker::EstimateSizeMm(int area, int signalSum) const {
    if (signalSum > 0)
        return std::max(m_fallbackSizeMm, std::cbrt(static_cast<float>(signalSum)) * m_sizeSignalScale);
    if (area > 0)
        return std::max(m_fallbackSizeMm, std::sqrt(static_cast<float>(area)) * m_sizeAreaScale);
    return m_fallbackSizeMm;
}

int TouchTracker::ComputeTouchDownDebounceFrames(const TouchContact& touch) const {
    int frames = m_touchDownDebounceFrames;
    if (!m_dynamicDebounceEnabled) return frames;
    int extra = 0;
    if (touch.signalSum > 0 && touch.signalSum < m_touchDownWeakSignalThreshold) extra += 1;
    if (touch.sizeMm > 0.0f && touch.sizeMm < m_touchDownSmallSizeThresholdMm) extra += 1;
    if (touch.isEdge) extra += 1;
    return frames + std::clamp(extra, 0, m_touchDownDebounceMaxExtra);
}

int TouchTracker::AllocateId(const std::vector<TrackState>& reservedNextTracks) const {
    for (int i = 0; i < m_maxTouchCount; ++i) {
        const int candidate = ((m_nextIdSeed - 1 + i) % m_maxTouchCount) + 1;
        bool used = false;
        for (const auto& t : reservedNextTracks) if (t.id == candidate) { used = true; break; }
        if (used) continue;
        for (const auto& t : m_tracks) if (t.id == candidate) { used = true; break; }
        if (!used) return candidate;
    }
    return 0;
}

bool TouchTracker::ApplyStylusTouchSuppression(HeatmapFrame& frame) {
    if (m_stylusSuppressGlobalEnabled && frame.stylus.touchSuppressActive) {
        frame.contacts.clear();
        m_tracks.clear();
        return true;
    }
    if (!m_stylusSuppressLocalEnabled || !frame.stylus.point.valid) return false;
    const int penPeak = std::max({static_cast<int>(frame.stylus.signalX),
                                  static_cast<int>(frame.stylus.signalY),
                                  static_cast<int>(frame.stylus.maxRawPeak)});
    if (penPeak < m_stylusSuppressPenPeakThreshold) return false;
    const float radiusSq = m_stylusSuppressLocalDistance * m_stylusSuppressLocalDistance;
    const float sx = frame.stylus.point.x, sy = frame.stylus.point.y;
    frame.contacts.erase(std::remove_if(frame.contacts.begin(), frame.contacts.end(),
        [&](const TouchContact& c) {
            if (DistanceSq(c.x,c.y,sx,sy) > radiusSq) return false;
            return !((c.signalSum >= m_stylusSuppressTouchSignalKeep) &&
                     (c.area >= m_stylusSuppressTouchAreaKeep));
        }), frame.contacts.end());
    return false;
}

bool TouchTracker::ResolveStylusAftContext(const HeatmapFrame& frame, float& outX, float& outY) {
    if (!m_stylusAftEnabled) return false;
    if (frame.stylus.point.valid) {
        m_lastStylusX = frame.stylus.point.x;
        m_lastStylusY = frame.stylus.point.y;
        m_stylusFramesSinceActive = 0;
    } else if (m_stylusFramesSinceActive < 1000000) {
        m_stylusFramesSinceActive += 1;
    }
    if (m_stylusFramesSinceActive > m_stylusAftRecentFrames) return false;
    outX = m_lastStylusX; outY = m_lastStylusY;
    return true;
}

bool TouchTracker::ShouldStylusAftSuppress(
    const TouchContact& touch, int touchAge,
    float stylusX, float stylusY, int& outHoldFrames) const {
    outHoldFrames = 0;
    if (!m_stylusAftEnabled) return false;
    if (DistanceSq(touch.x,touch.y,stylusX,stylusY) > m_stylusAftRadius*m_stylusAftRadius)
        return false;
    const bool palm = (touch.area >= m_stylusAftPalmAreaThreshold) ||
                      (touch.sizeMm >= m_stylusAftPalmSizeThresholdMm);
    const bool weak = (touch.signalSum < m_stylusAftWeakSignalThreshold) &&
                      (touch.sizeMm < m_stylusAftWeakSizeThresholdMm);
    const bool young = (touchAge <= m_stylusAftDebounceFrames);
    if (palm) { outHoldFrames = m_stylusAftPalmSuppressFrames; return true; }
    if (weak || young) { outHoldFrames = m_stylusAftSuppressFrames; return true; }
    return false;
}

// =============================================================================
// Process — pure IDT: match, track, emit state (Down/Move/Up).
// All reporting decisions are delegated to TouchGestureStateMachine.
// =============================================================================
bool TouchTracker::Process(HeatmapFrame& frame) {
    if (!m_enabled) return true;
    if (frame.contacts.size() > static_cast<size_t>(m_maxTouchCount))
        frame.contacts.resize(static_cast<size_t>(m_maxTouchCount));
    if (ApplyStylusTouchSuppression(frame)) return true;

    constexpr int kRows = 40, kCols = 60;
    constexpr float kEdgeMargin = 2.0f;
    float stylusAftX = 0, stylusAftY = 0;
    const bool stylusAftActive = ResolveStylusAftContext(frame, stylusAftX, stylusAftY);

    const size_t curCount = frame.contacts.size();
    const size_t preCount = m_tracks.size();
    std::vector<int> curToPre(curCount, -1);
    std::vector<char> alwaysMatched(curCount, false);
    const float maxDistSq = m_maxTrackDistance * m_maxTrackDistance;
    const float alwaysMatchSq = m_alwaysMatchDistance * m_alwaysMatchDistance;

    // ---- Hungarian / greedy matching ----
    if (curCount > 0 && preCount > 0) {
        std::vector<std::vector<float>> cost(curCount, std::vector<float>(preCount, 0));
        for (size_t c=0; c<curCount; ++c)
            for (size_t p=0; p<preCount; ++p) {
                const float px = m_tracks[p].x + m_tracks[p].vx * m_predictionScale;
                const float py = m_tracks[p].y + m_tracks[p].vy * m_predictionScale;
                cost[c][p] = DistanceSq(frame.contacts[c].x, frame.contacts[c].y, px, py);
            }
        if (m_useHungarian) curToPre = SolveAssignment(cost);
        else {
            std::vector<char> preUsed(preCount, false);
            for (size_t c=0; c<curCount; ++c) {
                float best = std::numeric_limits<float>::max(); int bi=-1;
                for (size_t p=0; p<preCount; ++p) {
                    if(preUsed[p]) continue;
                    if(cost[c][p]<best){best=cost[c][p];bi=static_cast<int>(p);}
                }
                if (bi>=0){preUsed[bi]=true;curToPre[c]=bi;}
            }
        }
        // Distance-gate matched pairs.
        for (size_t c=0; c<curCount; ++c) {
            if (curToPre[c]<0) continue;
            const auto& tk = m_tracks[curToPre[c]];
            const float px = tk.x+tk.vx*m_predictionScale;
            const float py = tk.y+tk.vy*m_predictionScale;
            float thresh = maxDistSq;
            const bool edge = IsEdgeTouch(tk.x,tk.y,kCols,kRows,kEdgeMargin) ||
                              IsEdgeTouch(frame.contacts[c].x,frame.contacts[c].y,kCols,kRows,kEdgeMargin);
            if (edge) thresh *= m_edgeTrackBoost*m_edgeTrackBoost;
            const float sz = std::max(tk.sizeMm, EstimateSizeMm(frame.contacts[c].area, frame.contacts[c].signalSum));
            if (edge || sz <= m_accBoostSizeMm) thresh *= m_accThresholdBoost*m_accThresholdBoost;
            if (DistanceSq(frame.contacts[c].x,frame.contacts[c].y,px,py) > thresh)
                curToPre[c]=-1;
        }
        // AlwaysMatch fallback.
        std::vector<char> cUsed(curCount,false), pUsed(preCount,false);
        for (size_t c=0; c<curCount; ++c)
            if(curToPre[c]>=0){cUsed[c]=true;pUsed[curToPre[c]]=true;}
        for (size_t c=0; c<curCount; ++c) {
            if(cUsed[c]) continue;
            float best=std::numeric_limits<float>::max(); int bi=-1;
            for (size_t p=0;p<preCount;++p) {
                if(pUsed[p]) continue;
                const float px=m_tracks[p].x+m_tracks[p].vx*m_predictionScale;
                const float py=m_tracks[p].y+m_tracks[p].vy*m_predictionScale;
                const float d=DistanceSq(frame.contacts[c].x,frame.contacts[c].y,px,py);
                if(d<best){best=d;bi=static_cast<int>(p);}
            }
            if(bi>=0 && best<=alwaysMatchSq &&
               !IsEdgeTouch(frame.contacts[c].x,frame.contacts[c].y,kCols,kRows,kEdgeMargin)) {
                curToPre[c]=bi; alwaysMatched[c]=true;
                cUsed[c]=true; pUsed[bi]=true;
            }
        }
    }

    // ---- Build next tracks + output contacts ----
    std::vector<char> preMatched(preCount, false);
    std::vector<TrackState> nextTracks;
    nextTracks.reserve(m_maxTouchCount);
    std::vector<TouchContact> out;
    out.reserve(curCount + preCount);

    for (size_t c = 0; c < curCount; ++c) {
        TouchContact o = frame.contacts[c];
        const int pre = curToPre[c];
        if (pre >= 0) {
            preMatched[pre] = true;
            TrackState t = m_tracks[pre];
            o.id = t.id;
            o.prevIndex = pre;
            o.isEdge = IsEdgeTouch(o.x,o.y,kCols,kRows,kEdgeMargin);
            o.lifeFlags = TouchLifeMapped;
            if (o.isEdge) o.lifeFlags |= TouchLifeEdge;
            if (alwaysMatched[c]) o.lifeFlags |= TouchLifeAlwaysMatch;

            const float curSize = EstimateSizeMm(o.area, o.signalSum);
            t.sizeMm = std::max({curSize, t.sizeMm, m_fallbackSizeMm});
            o.sizeMm = t.sizeMm;
            o.state = (t.age<=1 || t.downDebounceFrames>0) ? TouchStateDown : TouchStateMove;
            if (t.downDebounceFrames>0) {
                o.lifeFlags |= TouchLifeDebounced;
                t.downDebounceFrames -= 1;
            }
            t.vx = o.x - t.x;
            t.vy = o.y - t.y;
            t.x = o.x; t.y = o.y;
            t.area = o.area; t.signalSum = o.signalSum;
            t.missed = 0; t.age += 1;
            t.upEventEmitted = false;
            if (!stylusAftActive && t.stylusSuppressFrames>0) t.stylusSuppressFrames -= 1;

            // Stylus AFT suppression: mark but keep tracking.
            bool aftSuppressed = false;
            if (stylusAftActive) {
                if (t.stylusSuppressFrames > 0) {
                    aftSuppressed = true; t.stylusSuppressFrames -= 1;
                } else {
                    int hold = 0;
                    if (ShouldStylusAftSuppress(o,t.age,stylusAftX,stylusAftY,hold)) {
                        aftSuppressed = true; t.stylusSuppressFrames = std::max(0,hold-1);
                    }
                }
            }
            // Pure tracking: always report, let state machine decide.
            o.isReported = !aftSuppressed;
            o.reportEvent = TouchReportIdle; // State machine overrides.
            o.reportFlags = 0;
            o.debugFlags = aftSuppressed ? 0x101 : 0x01;

            if (out.size() < static_cast<size_t>(m_maxTouchCount)) out.push_back(o);
            if (nextTracks.size() < static_cast<size_t>(m_maxTouchCount)) nextTracks.push_back(t);
            continue;
        }

        // ---- New track ----
        TrackState t;
        t.id = AllocateId(nextTracks);
        if (t.id == 0) continue;
        t.x = o.x; t.y = o.y;
        t.area = o.area; t.signalSum = o.signalSum;
        t.sizeMm = EstimateSizeMm(o.area, o.signalSum);
        t.age = 1; t.missed = 0;
        o.isEdge = IsEdgeTouch(o.x,o.y,kCols,kRows,kEdgeMargin);

        // Reject weak/tiny new contacts.
        if (m_touchDownRejectEnabled) {
            const bool weak = (t.signalSum < m_touchDownRejectMinSignal);
            const bool tiny = (t.sizeMm < m_touchDownRejectMinSizeMm);
            const bool weakEdge = o.isEdge && (t.signalSum < m_touchDownEdgeRejectMinSignal);
            if ((weak && tiny) || weakEdge) continue;
        }
        t.downDebounceFrames = ComputeTouchDownDebounceFrames(o);
        t.upEventEmitted = false;
        if (stylusAftActive) {
            int hold = 0;
            if (ShouldStylusAftSuppress(o,t.age,stylusAftX,stylusAftY,hold)) {
                t.stylusSuppressFrames = std::max(0, hold - 1);
            }
        }
        m_nextIdSeed = (t.id % m_maxTouchCount) + 1;

        o.id = t.id;
        o.state = TouchStateDown;
        o.sizeMm = t.sizeMm;
        o.isReported = (t.stylusSuppressFrames <= 0);
        o.prevIndex = -1;
        o.debugFlags = 0x02;
        o.lifeFlags = TouchLifeNew;
        if (o.isEdge) o.lifeFlags |= TouchLifeEdge;
        if (t.downDebounceFrames > 0) {
            o.lifeFlags |= TouchLifeDebounced;
            t.downDebounceFrames -= 1;
        }
        o.reportEvent = TouchReportIdle;
        o.reportFlags = 0;

        if (out.size() < static_cast<size_t>(m_maxTouchCount)) out.push_back(o);
        if (nextTracks.size() < static_cast<size_t>(m_maxTouchCount)) nextTracks.push_back(t);
    }

    // ---- Unmatched previous tracks → Up ----
    for (size_t p = 0; p < preCount; ++p) {
        if (preMatched[p]) continue;
        TrackState t = m_tracks[p];
        t.missed += 1;
        if (t.stylusSuppressFrames > 0) t.stylusSuppressFrames -= 1;

        if (!t.upEventEmitted && t.missed > m_liftOffHoldFrames) {
            TouchContact up;
            up.id = t.id;
            up.x = t.x; up.y = t.y;
            up.state = TouchStateUp;
            up.area = t.area; up.signalSum = t.signalSum;
            up.sizeMm = t.sizeMm;
            up.isEdge = IsEdgeTouch(up.x,up.y,kCols,kRows,kEdgeMargin);
            up.isReported = true;
            up.prevIndex = static_cast<int>(p);
            up.debugFlags = 0x04;
            up.lifeFlags = TouchLifeLiftOff;
            if (up.isEdge) up.lifeFlags |= TouchLifeEdge;
            up.reportFlags = 0;
            up.reportEvent = TouchReportUp;
            if (out.size() < static_cast<size_t>(m_maxTouchCount)) out.push_back(up);
            t.upEventEmitted = true;
        }
        if (t.missed <= (m_liftOffHoldFrames + 1)) {
            t.vx = 0; t.vy = 0;
            if (nextTracks.size() < static_cast<size_t>(m_maxTouchCount))
                nextTracks.push_back(t);
        }
    }

    // ---- Rx ghost filter ----
    if (m_rxGhostFilterEnabled && out.size() > 1) {
        std::array<uint8_t, 21> removeById{};
        removeById.fill(0);
        for (size_t i=0; i<out.size(); ++i) {
            const auto& a = out[i];
            if (a.state==TouchStateUp||a.id<=0||a.id>m_maxTouchCount) continue;
            for (size_t j=i+1; j<out.size(); ++j) {
                const auto& b = out[j];
                if (b.state==TouchStateUp||b.id<=0||b.id>m_maxTouchCount) continue;
                const int ld = std::abs(static_cast<int>(std::lround(a.y))-
                                        static_cast<int>(std::lround(b.y)));
                if (ld > m_rxGhostLineDelta) continue;
                const TouchContact* strong=&a, *weak=&b;
                if (b.signalSum > a.signalSum) { strong=&b; weak=&a; }
                if (weak->signalSum >= static_cast<int>(static_cast<float>(strong->signalSum)*m_rxGhostWeakRatio))
                    continue;
                if (m_rxGhostOnlyNew && weak->state!=TouchStateDown) continue;
                removeById[weak->id] = 1;
            }
        }
        out.erase(std::remove_if(out.begin(),out.end(),[&](const TouchContact& c){
            if(c.state==TouchStateUp||c.id<=0||c.id>m_maxTouchCount) return false;
            return removeById[c.id]!=0;
        }),out.end());
        nextTracks.erase(std::remove_if(nextTracks.begin(),nextTracks.end(),[&](const TrackState& t){
            if(t.id<=0||t.id>m_maxTouchCount) return false;
            return removeById[t.id]!=0;
        }),nextTracks.end());
    }

    frame.contacts = std::move(out);
    m_tracks = std::move(nextTracks);
    return true;
}

// =============================================================================
// GetConfigSchema — IDT core params
// =============================================================================
std::vector<ConfigParam> TouchTracker::GetConfigSchema() const {
    std::vector<ConfigParam> schema = IFrameProcessor::GetConfigSchema();
    
    // --- Core Tracking ---
    schema.push_back(ConfigParam("UseHungarian", "Use Hungarian", 
        ConfigParam::Bool, const_cast<bool*>(&m_useHungarian)));
    schema.push_back(ConfigParam("MaxTrackDistance", "Max Track Dist", 
        ConfigParam::Float, const_cast<float*>(&m_maxTrackDistance), 1.0f, 20.0f));
    schema.push_back(ConfigParam("AlwaysMatchDistance", "Always Match Dist", 
        ConfigParam::Float, const_cast<float*>(&m_alwaysMatchDistance), 0.5f, 6.0f));
    schema.push_back(ConfigParam("PredictionScale", "Prediction Scale", 
        ConfigParam::Float, const_cast<float*>(&m_predictionScale), 0.0f, 2.0f));
    schema.push_back(ConfigParam("LiftOffHoldFrames", "LiftOff Hold", 
        ConfigParam::Int, const_cast<int*>(&m_liftOffHoldFrames), 0, 10));

    // --- Debounce & Rejection ---
    schema.push_back(ConfigParam("TouchDownDebounceFrames", "Down Debounce", 
        ConfigParam::Int, const_cast<int*>(&m_touchDownDebounceFrames), 0, 10));
    schema.push_back(ConfigParam("TouchDownRejectEnabled", "Enable Reject", 
        ConfigParam::Bool, const_cast<bool*>(&m_touchDownRejectEnabled)));
    schema.push_back(ConfigParam("TouchDownRejectMinSignal", "Reject Signal Th", 
        ConfigParam::Int, const_cast<int*>(&m_touchDownRejectMinSignal), 0, 500));

    // --- Stylus Interop ---
    schema.push_back(ConfigParam("StylusSuppressGlobalEnabled", "Pen Global Suppress", 
        ConfigParam::Bool, const_cast<bool*>(&m_stylusSuppressGlobalEnabled)));
    schema.push_back(ConfigParam("StylusSuppressLocalEnabled", "Pen Local Suppress", 
        ConfigParam::Bool, const_cast<bool*>(&m_stylusSuppressLocalEnabled)));
    schema.push_back(ConfigParam("StylusSuppressLocalDistance", "Suppress radius", 
        ConfigParam::Float, const_cast<float*>(&m_stylusSuppressLocalDistance), 0.5f, 10.0f));
    schema.push_back(ConfigParam("StylusAftEnabled", "Enable AFT (Anti-Falsing)", 
        ConfigParam::Bool, const_cast<bool*>(&m_stylusAftEnabled)));
    schema.push_back(ConfigParam("StylusAftRadius", "AFT Radius", 
        ConfigParam::Float, const_cast<float*>(&m_stylusAftRadius), 0.5f, 10.0f));
    schema.push_back(ConfigParam("StylusAftSuppressFrames", "AFT Suppress Frames", 
        ConfigParam::Int, const_cast<int*>(&m_stylusAftSuppressFrames), 0, 200));

    return schema;
}

// =============================================================================
// SaveConfig / LoadConfig — IDT core + stylus only
// =============================================================================
void TouchTracker::SaveConfig(std::ostream& os) const {
    IFrameProcessor::SaveConfig(os);
    os << "UseHungarian=" << (m_useHungarian?"1":"0") << "\n";
    os << "MaxTrackDistance=" << m_maxTrackDistance << "\n";
    os << "AlwaysMatchDistance=" << m_alwaysMatchDistance << "\n";
    os << "EdgeTrackBoost=" << m_edgeTrackBoost << "\n";
    os << "AccThresholdBoost=" << m_accThresholdBoost << "\n";
    os << "AccBoostSizeMm=" << m_accBoostSizeMm << "\n";
    os << "PredictionScale=" << m_predictionScale << "\n";
    os << "LiftOffHoldFrames=" << m_liftOffHoldFrames << "\n";
    os << "TouchDownDebounceFrames=" << m_touchDownDebounceFrames << "\n";
    os << "DynamicDebounceEnabled=" << (m_dynamicDebounceEnabled?"1":"0") << "\n";
    os << "TouchDownDebounceMaxExtra=" << m_touchDownDebounceMaxExtra << "\n";
    os << "TouchDownWeakSignalThreshold=" << m_touchDownWeakSignalThreshold << "\n";
    os << "TouchDownSmallSizeThresholdMm=" << m_touchDownSmallSizeThresholdMm << "\n";
    os << "TouchDownRejectEnabled=" << (m_touchDownRejectEnabled?"1":"0") << "\n";
    os << "TouchDownRejectMinSignal=" << m_touchDownRejectMinSignal << "\n";
    os << "TouchDownRejectMinSizeMm=" << m_touchDownRejectMinSizeMm << "\n";
    os << "TouchDownEdgeRejectMinSignal=" << m_touchDownEdgeRejectMinSignal << "\n";
    os << "FallbackSizeMm=" << m_fallbackSizeMm << "\n";
    os << "SizeAreaScale=" << m_sizeAreaScale << "\n";
    os << "SizeSignalScale=" << m_sizeSignalScale << "\n";
    os << "RxGhostFilterEnabled=" << (m_rxGhostFilterEnabled?"1":"0") << "\n";
    os << "RxGhostLineDelta=" << m_rxGhostLineDelta << "\n";
    os << "RxGhostWeakRatio=" << m_rxGhostWeakRatio << "\n";
    os << "RxGhostOnlyNew=" << (m_rxGhostOnlyNew?"1":"0") << "\n";
    os << "StylusSuppressGlobalEnabled=" << (m_stylusSuppressGlobalEnabled?"1":"0") << "\n";
    os << "StylusSuppressLocalEnabled=" << (m_stylusSuppressLocalEnabled?"1":"0") << "\n";
    os << "StylusSuppressLocalDistance=" << m_stylusSuppressLocalDistance << "\n";
    os << "StylusSuppressPenPeakThreshold=" << m_stylusSuppressPenPeakThreshold << "\n";
    os << "StylusSuppressTouchSignalKeep=" << m_stylusSuppressTouchSignalKeep << "\n";
    os << "StylusSuppressTouchAreaKeep=" << m_stylusSuppressTouchAreaKeep << "\n";
    os << "StylusAftEnabled=" << (m_stylusAftEnabled?"1":"0") << "\n";
    os << "StylusAftRecentFrames=" << m_stylusAftRecentFrames << "\n";
    os << "StylusAftRadius=" << m_stylusAftRadius << "\n";
    os << "StylusAftDebounceFrames=" << m_stylusAftDebounceFrames << "\n";
    os << "StylusAftWeakSignalThreshold=" << m_stylusAftWeakSignalThreshold << "\n";
    os << "StylusAftWeakSizeThresholdMm=" << m_stylusAftWeakSizeThresholdMm << "\n";
    os << "StylusAftSuppressFrames=" << m_stylusAftSuppressFrames << "\n";
    os << "StylusAftPalmSuppressFrames=" << m_stylusAftPalmSuppressFrames << "\n";
    os << "StylusAftPalmAreaThreshold=" << m_stylusAftPalmAreaThreshold << "\n";
    os << "StylusAftPalmSizeThresholdMm=" << m_stylusAftPalmSizeThresholdMm << "\n";
}

void TouchTracker::LoadConfig(const std::string& key, const std::string& value) {
    IFrameProcessor::LoadConfig(key, value);
    auto toBool = [](const std::string& v){ return v=="1"||v=="true"; };
    if      (key=="UseHungarian")            m_useHungarian = toBool(value);
    else if (key=="MaxTrackDistance")         m_maxTrackDistance = std::stof(value);
    else if (key=="AlwaysMatchDistance")      m_alwaysMatchDistance = std::stof(value);
    else if (key=="EdgeTrackBoost")           m_edgeTrackBoost = std::stof(value);
    else if (key=="AccThresholdBoost")        m_accThresholdBoost = std::stof(value);
    else if (key=="AccBoostSizeMm")           m_accBoostSizeMm = std::stof(value);
    else if (key=="PredictionScale")          m_predictionScale = std::stof(value);
    else if (key=="LiftOffHoldFrames")        m_liftOffHoldFrames = std::stoi(value);
    else if (key=="TouchDownDebounceFrames")  m_touchDownDebounceFrames = std::stoi(value);
    else if (key=="DynamicDebounceEnabled")   m_dynamicDebounceEnabled = toBool(value);
    else if (key=="TouchDownDebounceMaxExtra")m_touchDownDebounceMaxExtra = std::stoi(value);
    else if (key=="TouchDownWeakSignalThreshold") m_touchDownWeakSignalThreshold = std::stoi(value);
    else if (key=="TouchDownSmallSizeThresholdMm")m_touchDownSmallSizeThresholdMm = std::stof(value);
    else if (key=="TouchDownRejectEnabled")   m_touchDownRejectEnabled = toBool(value);
    else if (key=="TouchDownRejectMinSignal") m_touchDownRejectMinSignal = std::stoi(value);
    else if (key=="TouchDownRejectMinSizeMm") m_touchDownRejectMinSizeMm = std::stof(value);
    else if (key=="TouchDownEdgeRejectMinSignal") m_touchDownEdgeRejectMinSignal = std::stoi(value);
    else if (key=="FallbackSizeMm")           m_fallbackSizeMm = std::stof(value);
    else if (key=="SizeAreaScale")            m_sizeAreaScale = std::stof(value);
    else if (key=="SizeSignalScale")          m_sizeSignalScale = std::stof(value);
    else if (key=="RxGhostFilterEnabled")     m_rxGhostFilterEnabled = toBool(value);
    else if (key=="RxGhostLineDelta")         m_rxGhostLineDelta = std::stoi(value);
    else if (key=="RxGhostWeakRatio")         m_rxGhostWeakRatio = std::stof(value);
    else if (key=="RxGhostOnlyNew")           m_rxGhostOnlyNew = toBool(value);
    else if (key=="StylusSuppressGlobalEnabled")  m_stylusSuppressGlobalEnabled = toBool(value);
    else if (key=="StylusSuppressLocalEnabled")   m_stylusSuppressLocalEnabled = toBool(value);
    else if (key=="StylusSuppressLocalDistance")  m_stylusSuppressLocalDistance = std::max(0.1f,std::stof(value));
    else if (key=="StylusSuppressPenPeakThreshold") m_stylusSuppressPenPeakThreshold = std::max(1,std::stoi(value));
    else if (key=="StylusSuppressTouchSignalKeep")  m_stylusSuppressTouchSignalKeep = std::max(1,std::stoi(value));
    else if (key=="StylusSuppressTouchAreaKeep")    m_stylusSuppressTouchAreaKeep = std::max(1,std::stoi(value));
    else if (key=="StylusAftEnabled")         m_stylusAftEnabled = toBool(value);
    else if (key=="StylusAftRecentFrames")    m_stylusAftRecentFrames = std::clamp(std::stoi(value),1,240);
    else if (key=="StylusAftRadius")          m_stylusAftRadius = std::max(0.1f,std::stof(value));
    else if (key=="StylusAftDebounceFrames")  m_stylusAftDebounceFrames = std::clamp(std::stoi(value),1,32);
    else if (key=="StylusAftWeakSignalThreshold") m_stylusAftWeakSignalThreshold = std::max(1,std::stoi(value));
    else if (key=="StylusAftWeakSizeThresholdMm") m_stylusAftWeakSizeThresholdMm = std::max(0.1f,std::stof(value));
    else if (key=="StylusAftSuppressFrames")  m_stylusAftSuppressFrames = std::max(1,std::stoi(value));
    else if (key=="StylusAftPalmSuppressFrames")  m_stylusAftPalmSuppressFrames = std::max(1,std::stoi(value));
    else if (key=="StylusAftPalmAreaThreshold")   m_stylusAftPalmAreaThreshold = std::max(1,std::stoi(value));
    else if (key=="StylusAftPalmSizeThresholdMm") m_stylusAftPalmSizeThresholdMm = std::max(0.1f,std::stof(value));
}

} // namespace Engine
