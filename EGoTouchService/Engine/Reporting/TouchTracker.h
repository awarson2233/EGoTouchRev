#pragma once

#include "IFrameProcessor.h"
#include <array>
#include <vector>

namespace Engine {

class TouchTracker : public IFrameProcessor {
public:
    TouchTracker() = default;
    ~TouchTracker() override = default;

    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "Touch Tracker (IDT)"; }

    std::vector<ConfigParam> GetConfigSchema() const override;
    void SaveConfig(std::ostream& out) const override;
    void LoadConfig(const std::string& key, const std::string& value) override;

private:
    struct TrackState {
        int id = 0;
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        int area = 0;
        int signalSum = 0;
        float sizeMm = 0.0f;
        int age = 0;
        int missed = 0;
        int downDebounceFrames = 0;
        bool upEventEmitted = false;
        int stylusSuppressFrames = 0;
    };

    static float DistanceSq(float x1, float y1, float x2, float y2);
    static bool IsEdgeTouch(float x, float y, int cols, int rows, float edgeMargin);
    static std::vector<int> SolveAssignment(const std::vector<std::vector<float>>& cost);

    float EstimateSizeMm(int area, int signalSum) const;
    int ComputeTouchDownDebounceFrames(const TouchContact& touch) const;
    int AllocateId(const std::vector<TrackState>& reservedNextTracks) const;
    bool ApplyStylusTouchSuppression(HeatmapFrame& frame);
    bool ResolveStylusAftContext(const HeatmapFrame& frame, float& outStylusX, float& outStylusY);
    bool ShouldStylusAftSuppress(const TouchContact& touch, int touchAge, float stylusX, float stylusY, int& outHoldFrames) const;

private:
    std::vector<TrackState> m_tracks;
    int m_nextIdSeed = 1;

    // ---- IDT core params ----
    int m_maxTouchCount = 20;
    float m_maxTrackDistance = 6.0f;
    float m_alwaysMatchDistance = 2.2f;
    float m_edgeTrackBoost = 1.5f;
    float m_accThresholdBoost = 4.0f;
    float m_accBoostSizeMm = 1.6f;
    float m_predictionScale = 1.0f;
    int m_liftOffHoldFrames = 1;
    int m_touchDownDebounceFrames = 0;
    bool m_dynamicDebounceEnabled = true;
    int m_touchDownDebounceMaxExtra = 2;
    int m_touchDownWeakSignalThreshold = 180;
    float m_touchDownSmallSizeThresholdMm = 1.3f;
    bool m_touchDownRejectEnabled = true;
    int m_touchDownRejectMinSignal = 55;
    float m_touchDownRejectMinSizeMm = 0.95f;
    int m_touchDownEdgeRejectMinSignal = 90;
    float m_fallbackSizeMm = 1.0f;
    float m_sizeAreaScale = 0.22f;
    float m_sizeSignalScale = 0.35f;
    bool m_rxGhostFilterEnabled = false;
    int m_rxGhostLineDelta = 0;
    float m_rxGhostWeakRatio = 0.5f;
    bool m_rxGhostOnlyNew = true;
    bool m_useHungarian = true;

    // ---- Stylus interop ----
    bool m_stylusSuppressGlobalEnabled = true;
    bool m_stylusSuppressLocalEnabled = true;
    float m_stylusSuppressLocalDistance = 2.5f;
    int m_stylusSuppressPenPeakThreshold = 1500;
    int m_stylusSuppressTouchSignalKeep = 6000;
    int m_stylusSuppressTouchAreaKeep = 12;

    bool m_stylusAftEnabled = true;
    int m_stylusAftRecentFrames = 24;
    float m_stylusAftRadius = 2.8f;
    int m_stylusAftDebounceFrames = 3;
    int m_stylusAftWeakSignalThreshold = 240;
    float m_stylusAftWeakSizeThresholdMm = 1.2f;
    int m_stylusAftSuppressFrames = 40;
    int m_stylusAftPalmSuppressFrames = 100;
    int m_stylusAftPalmAreaThreshold = 20;
    float m_stylusAftPalmSizeThresholdMm = 2.5f;
    int m_stylusFramesSinceActive = 1000000;
    float m_lastStylusX = 0.0f;
    float m_lastStylusY = 0.0f;
};

} // namespace Engine
