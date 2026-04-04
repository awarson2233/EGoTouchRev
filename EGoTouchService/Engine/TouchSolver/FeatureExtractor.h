#pragma once

#include "IFrameProcessor.h"
#include "PeakDetector.h"
#include "ZoneExpander.h"
#include "EdgeCompensation.h"
#include "TouchSize.h"
#include "EdgeRejection.h"
#include <string>
#include <mutex>

namespace Engine {

// 触点搜寻协调器：调度 PeakDetector → ZoneExpander (TSACore TZ_Process)
class FeatureExtractor : public IFrameProcessor {
public:
    FeatureExtractor() = default;
    ~FeatureExtractor() override = default;

    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override {
        return "Feature Extractor (4.1/4.2)";
    }
    std::vector<ConfigParam> GetConfigSchema() const override;
    void SaveConfig(std::ostream& out) const override;
    void LoadConfig(const std::string& key,
                    const std::string& value) override;

    // 线程安全的公共访问 (加锁拷贝)
    std::vector<Peak> GetPeaks() const {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_peakDet.GetPeaks();
    }
    auto GetTouchZones() const {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_zoneExp.GetTouchZones();
    }
    auto GetZoneEdge() const {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_zoneExp.GetZoneEdge();
    }

    // 供外部直接调参 (仅在 UI 线程建议用)
    PeakDetector       m_peakDet;
    ZoneExpander       m_zoneExp;
    EdgeCompensator    m_edgeComp;
    TouchSizeCalculator m_touchSize;
    EdgeRejector       m_edgeReject;

    // UI 线程安全缓存 (由 Process 线程写入)
    int m_cachedPeakCount = 0;
    int m_cachedZoneCount = 0;
    int m_cachedContactCount = 0;

private:
    mutable std::mutex m_mtx;
};

} // namespace Engine
