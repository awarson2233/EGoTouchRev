#include "FeatureExtractor.h"

namespace Engine {

bool FeatureExtractor::Process(HeatmapFrame& frame) {
    if (!m_enabled) return true;
    frame.contacts.clear();

    {
        std::lock_guard<std::mutex> lk(m_mtx);
        // Step 1: Macro Zone Detection (New Preprocessing)
        m_macroZoneDet.Process(frame, m_peakDet.m_threshold);

        // Step 1.5: Palm Rejection — remove palm-like MacroZones before peak detection
        m_palmReject.Process(m_macroZoneDet.GetMutableMacroZones(), frame);

        // Step 2: Peak detection (TSACore Peak_Process, scoped to remaining Macro Zones)
        m_peakDet.Detect(frame, m_macroZoneDet.GetMacroZones());

        // Step 2.5: Detailed Peak Zone Segmentation (Micro Zones)
        m_microZoneSeg.Process(frame, m_macroZoneDet.GetMacroZones(), m_peakDet.GetPeaks());

        // Step 3: Zone flood-fill + centroid → contacts (TSACore TZ_Process)
        m_zoneExp.Process(frame, m_peakDet.GetPeaks(), m_peakDet.m_threshold);
        // Step 4: Edge compensation (TSACore CTD_ECProcess)
        m_edgeComp.Process(frame.contacts,
                           m_zoneExp.GetEdgeInfos(),
                           m_zoneExp.m_edgeBounds);
        // Step 4: Touch size in mm (TSACore TS_Process)
        m_touchSize.Process(frame.contacts);
        // Step 5: Edge rejection (TSACore ER_Process)
        m_edgeReject.Process(frame.contacts,
                             m_zoneExp.GetEdgeInfos(),
                             m_zoneExp.m_edgeBounds);

        m_cachedPeakCount = static_cast<int>(m_peakDet.GetPeaks().size());
        m_cachedZoneCount = m_zoneExp.GetZoneCount();
        m_cachedContactCount = static_cast<int>(frame.contacts.size());
        
        // --- 供 IPC 传输给上位机显示的热力图元数据 ---
        frame.peaks.clear();
        for (const auto& pk : m_peakDet.GetPeaks()) {
            frame.peaks.push_back({pk.r, pk.c, pk.z, pk.id});
        }
        
        // 1. Mapping MacroZones to touchZones array for UI drawing
        frame.touchZones.fill(0);
        const auto& mZones = m_macroZoneDet.GetMacroZones();
        for (size_t i = 0; i < mZones.size(); ++i) {
            uint8_t colorId = static_cast<uint8_t>((i % 10) + 1); // Cycle colors
            for (int idx : mZones[i].pixels) {
                if (idx >= 0 && idx < 2400) {
                    frame.touchZones[idx] = colorId;
                }
            }
        }

        // 2. Mapping MicroZones (peakZones) from Segmenter
        frame.peakZones = m_microZoneSeg.GetPeakZones();

    }

    return true;
}

std::vector<ConfigParam> FeatureExtractor::GetConfigSchema() const {
    auto base = IFrameProcessor::GetConfigSchema();
    
    // PeakDetector
    base.emplace_back("PeakThreshold", "Peak Threshold", ConfigParam::Int, const_cast<int*>(&m_peakDet.m_threshold), 1, 1000);
    base.emplace_back("SigTholdLimit", "Sig Thold Limit", ConfigParam::Int, const_cast<int*>(&m_peakDet.m_sigTholdLimit), 1, 1000);
    base.emplace_back("Z8FilterEnabled", "Z8 Filter Enabled", ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_z8Filter));
    base.emplace_back("Z1FilterEnabled", "Z1 Filter Enabled", ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_z1Filter));
    base.emplace_back("PressureDriftFilter", "Pressure Drift Filter", ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_pressureDriftFilter));
    base.emplace_back("EdgePeakFilter", "Edge Peak Filter", ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_edgePeakFilter));
    base.emplace_back("EdgeThresholdEnabled", "Edge Threshold Enabled", ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_edgeThresholdEnabled));
    base.emplace_back("EdgeThreshold", "Edge Threshold", ConfigParam::Int, const_cast<int*>(&m_peakDet.m_edgeThreshold), 1, 1000);
    base.emplace_back("Z8Radius", "Z8 Max Search Radius", ConfigParam::Int, const_cast<int*>(&m_peakDet.m_z8Radius), 1, 5);
    base.emplace_back("MaxPeaks", "Peak Limit Cap", ConfigParam::Int, const_cast<int*>(&m_peakDet.m_maxPeaks), 5, 100);
    base.emplace_back("PressureDriftDebounce", "Pressure Debounce Limit", ConfigParam::Int, const_cast<int*>(&m_peakDet.m_pressureDriftDebounceLimit), 0, 10);
    base.emplace_back("MacroZoneMinArea", "MacroZone Min Area", ConfigParam::Int, const_cast<int*>(&m_peakDet.m_macroZoneMinArea), 1, 20);

    // ZoneExpander
    base.emplace_back("DilateErode", "Dilate Erode Enabled", ConfigParam::Bool, const_cast<bool*>(&m_zoneExp.m_dilateErode));
    base.emplace_back("ZoneTholdScale", "Zone Thold Numer", ConfigParam::Int, const_cast<int*>(&m_zoneExp.m_tholdScaleNumer), 0, 255);
    base.emplace_back("ZoneTholdShift", "Zone Thold Shift", ConfigParam::Int, const_cast<int*>(&m_zoneExp.m_tholdScaleShift), 0, 15);
    base.emplace_back("MaxTouches", "Max Contact Outputs", ConfigParam::Int, const_cast<int*>(&m_zoneExp.m_maxTouches), 1, 50);

    // EdgeCompensation
    base.emplace_back("ECEnabled", "Edge Compensation Enabled", ConfigParam::Bool, const_cast<bool*>(&m_edgeComp.m_enabled));
    base.emplace_back("ECBlendRange", "EC Blend Range", ConfigParam::Float, const_cast<float*>(&m_edgeComp.m_ecBlendRange), 0.0f, 5.0f);

    // PalmRejector
    base.emplace_back("PalmEnabled", "Palm Rejection Enabled", ConfigParam::Bool, const_cast<bool*>(&m_palmReject.m_enabled));
    base.emplace_back("PalmAreaThreshold", "Palm Area Threshold", ConfigParam::Int, const_cast<int*>(&m_palmReject.m_areaThreshold), 5, 300);
    base.emplace_back("PalmSignalSumThreshold", "Palm SignalSum Threshold", ConfigParam::Int, const_cast<int*>(&m_palmReject.m_signalSumThreshold), 1000, 500000);
    base.emplace_back("PalmDensityThresholdLow", "Palm Density Low Threshold", ConfigParam::Float, const_cast<float*>(&m_palmReject.m_densityThresholdLow), 50.0f, 2000.0f);
    base.emplace_back("PalmAreaMinForDensity", "Palm Density Min Area", ConfigParam::Int, const_cast<int*>(&m_palmReject.m_areaMinForDensity), 3, 100);
    base.emplace_back("PalmElongatedEnabled", "Elongated Press Reject", ConfigParam::Bool, const_cast<bool*>(&m_palmReject.m_elongatedEnabled));
    base.emplace_back("PalmElongatedMinArea", "Elongated Min Area", ConfigParam::Int, const_cast<int*>(&m_palmReject.m_elongatedMinArea), 3, 100);
    base.emplace_back("PalmElongatedAspectRatio", "Elongated Aspect Ratio", ConfigParam::Float, const_cast<float*>(&m_palmReject.m_elongatedAspectRatio), 1.5f, 10.0f);

    return base;
}

void FeatureExtractor::SaveConfig(std::ostream& out) const {
    IFrameProcessor::SaveConfig(out);
    out << "PeakThreshold=" << m_peakDet.m_threshold << "\n";
    out << "SigTholdLimit=" << m_peakDet.m_sigTholdLimit << "\n";
    out << "Z8FilterEnabled=" << (m_peakDet.m_z8Filter?"1":"0") << "\n";
    out << "Z1FilterEnabled=" << (m_peakDet.m_z1Filter?"1":"0") << "\n";
    out << "PressureDriftFilter=" << (m_peakDet.m_pressureDriftFilter?"1":"0") << "\n";
    out << "EdgePeakFilter=" << (m_peakDet.m_edgePeakFilter?"1":"0") << "\n";
    out << "EdgeThresholdEnabled=" << (m_peakDet.m_edgeThresholdEnabled?"1":"0") << "\n";
    out << "EdgeThreshold=" << m_peakDet.m_edgeThreshold << "\n";
    out << "Z8Radius=" << m_peakDet.m_z8Radius << "\n";
    out << "MaxPeaks=" << m_peakDet.m_maxPeaks << "\n";
    out << "PressureDriftDebounce=" << m_peakDet.m_pressureDriftDebounceLimit << "\n";
    out << "MacroZoneMinArea=" << m_peakDet.m_macroZoneMinArea << "\n";
    out << "DilateErode=" << (m_zoneExp.m_dilateErode?"1":"0") << "\n";
    out << "ZoneTholdScale=" << m_zoneExp.m_tholdScaleNumer << "\n";
    out << "ZoneTholdShift=" << m_zoneExp.m_tholdScaleShift << "\n";
    out << "MaxTouches=" << m_zoneExp.m_maxTouches << "\n";
    out << "ECEnabled=" << (m_edgeComp.m_enabled?"1":"0") << "\n";
    out << "ECBlendRange=" << m_edgeComp.m_ecBlendRange << "\n";
    out << "PalmEnabled=" << (m_palmReject.m_enabled?"1":"0") << "\n";
    out << "PalmAreaThreshold=" << m_palmReject.m_areaThreshold << "\n";
    out << "PalmSignalSumThreshold=" << m_palmReject.m_signalSumThreshold << "\n";
    out << "PalmDensityThresholdLow=" << m_palmReject.m_densityThresholdLow << "\n";
    out << "PalmAreaMinForDensity=" << m_palmReject.m_areaMinForDensity << "\n";
    out << "PalmElongatedEnabled=" << (m_palmReject.m_elongatedEnabled?"1":"0") << "\n";
    out << "PalmElongatedMinArea=" << m_palmReject.m_elongatedMinArea << "\n";
    out << "PalmElongatedAspectRatio=" << m_palmReject.m_elongatedAspectRatio << "\n";
}

void FeatureExtractor::LoadConfig(const std::string& key,
                                  const std::string& value) {
    IFrameProcessor::LoadConfig(key, value);
    if (key == "PeakThreshold")
        m_peakDet.m_threshold = std::stoi(value);
    else if (key == "SigTholdLimit")
        m_peakDet.m_sigTholdLimit = std::stoi(value);
    else if (key == "Z8FilterEnabled")
        m_peakDet.m_z8Filter = (value == "1");
    else if (key == "Z1FilterEnabled")
        m_peakDet.m_z1Filter = (value == "1");
    else if (key == "PressureDriftFilter")
        m_peakDet.m_pressureDriftFilter = (value == "1");
    else if (key == "EdgePeakFilter")
        m_peakDet.m_edgePeakFilter = (value == "1");
    else if (key == "EdgeThresholdEnabled")
        m_peakDet.m_edgeThresholdEnabled = (value == "1");
    else if (key == "EdgeThreshold")
        m_peakDet.m_edgeThreshold = std::stoi(value);
    else if (key == "Z8Radius")
        m_peakDet.m_z8Radius = std::stoi(value);
    else if (key == "MaxPeaks")
        m_peakDet.m_maxPeaks = std::stoi(value);
    else if (key == "PressureDriftDebounce")
        m_peakDet.m_pressureDriftDebounceLimit = std::stoi(value);
    else if (key == "MacroZoneMinArea")
        m_peakDet.m_macroZoneMinArea = std::stoi(value);
    else if (key == "DilateErode")
        m_zoneExp.m_dilateErode = (value == "1");
    else if (key == "ZoneTholdScale")
        m_zoneExp.m_tholdScaleNumer = std::stoi(value);
    else if (key == "ZoneTholdShift")
        m_zoneExp.m_tholdScaleShift = std::stoi(value);
    else if (key == "MaxTouches")
        m_zoneExp.m_maxTouches = std::stoi(value);
    else if (key == "ECEnabled")
        m_edgeComp.m_enabled = (value == "1");
    else if (key == "ECBlendRange")
        m_edgeComp.m_ecBlendRange = std::stof(value);
    else if (key == "PalmEnabled")
        m_palmReject.m_enabled = (value == "1");
    else if (key == "PalmAreaThreshold")
        m_palmReject.m_areaThreshold = std::stoi(value);
    else if (key == "PalmSignalSumThreshold")
        m_palmReject.m_signalSumThreshold = std::stoi(value);
    else if (key == "PalmDensityThresholdLow")
        m_palmReject.m_densityThresholdLow = std::stof(value);
    else if (key == "PalmAreaMinForDensity")
        m_palmReject.m_areaMinForDensity = std::stoi(value);
    else if (key == "PalmElongatedEnabled")
        m_palmReject.m_elongatedEnabled = (value == "1");
    else if (key == "PalmElongatedMinArea")
        m_palmReject.m_elongatedMinArea = std::stoi(value);
    else if (key == "PalmElongatedAspectRatio")
        m_palmReject.m_elongatedAspectRatio = std::stof(value);
}

} // namespace Engine
