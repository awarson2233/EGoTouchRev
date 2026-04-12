#include "TouchPipeline.h"
#include "Logger.h"

namespace Engine {

// ══════════════════════════════════════════════════════════════════════
// Process — linear orchestration of all 6 phases
// ══════════════════════════════════════════════════════════════════════
bool TouchPipeline::Process(HeatmapFrame& frame) {

    // ── Phase 1: Frame Parsing ──────────────────────────────────────
    m_frameParser.Process(frame);

    // ── Phase 2: Signal Conditioning ────────────────────────────────
    m_baseline.Process(frame);
    m_cmf.Process(frame);
    m_gridIIR.Process(frame);

    // ── Phase 3: Feature Extraction ─────────────────────────────────
    frame.contacts.clear();
    {
        std::lock_guard<std::mutex> lk(m_mtx);

        // 3.1 Macro Zone Detection (BFS connected components)
        m_macroZoneDet.Process(frame, m_peakDet.m_threshold);

        // 3.2 Palm Rejection — remove large/elongated zones
        m_palmReject.Process(m_macroZoneDet.GetMutableMacroZones(), frame);

        // 3.3 Peak Detection (local maxima within macro zones)
        m_peakDet.Detect(frame, m_macroZoneDet.GetMacroZones());

        // 3.4 Micro Zone Segmentation (Voronoi/Watershed)
        m_microZoneSeg.Process(frame, m_macroZoneDet.GetMacroZones(),
                               m_peakDet.GetPeaks());

        // ── Phase 4: Zone Processing & Contact Generation ──────────
        // 4.1 Zone expansion (BFS flood-fill from peaks → contacts)
        m_zoneExp.Process(frame, m_peakDet.GetPeaks(),
                          m_peakDet.m_threshold);

        // 4.2 Edge Compensation (LUT-based boundary correction)
        m_edgeComp.Process(frame.contacts,
                           m_zoneExp.GetEdgeInfos(),
                           m_zoneExp.m_edgeBounds);

        // 4.3 Touch Size calculation (signalSum → radius in mm)
        m_touchSize.Process(frame.contacts);

        // 4.4 Edge Rejection (suppress new touches at sensor boundary)
        m_edgeReject.Process(frame.contacts,
                             m_zoneExp.GetEdgeInfos(),
                             m_zoneExp.m_edgeBounds);

        // ── Update diagnostic caches ────────────────────────────────
        m_cachedPeakCount = static_cast<int>(m_peakDet.GetPeaks().size());
        m_cachedZoneCount = m_zoneExp.GetZoneCount();
        m_cachedContactCount = static_cast<int>(frame.contacts.size());

        // ── MacroZone → touchZones colormap for IPC visualization ──
        frame.touchZones.fill(0);
        const auto& mZones = m_macroZoneDet.GetMacroZones();
        for (size_t i = 0; i < mZones.size(); ++i) {
            uint8_t colorId = static_cast<uint8_t>((i % 10) + 1);
            for (int idx : mZones[i].pixels) {
                if (idx >= 0 && idx < 2400)
                    frame.touchZones[idx] = colorId;
            }
        }

        // peakZones already written by microZoneSeg
        frame.peakZones = m_microZoneSeg.GetPeakZones();

        // Populate frame.peaks for IPC/UI visualization
        frame.peaks.clear();
        for (const auto& pk : m_peakDet.GetPeaks()) {
            frame.peaks.push_back({pk.r, pk.c, pk.z, pk.id});
        }
    }

    m_tracker.Process(frame);
    m_coordFilter.Process(frame);
    // ── Phase 6: Gesture Recognition & Output ───────────────────────
    m_gesture.Process(frame);

    return true;
}

// ══════════════════════════════════════════════════════════════════════
// Thread-safe accessors
// ══════════════════════════════════════════════════════════════════════
std::vector<Touch::Peak> TouchPipeline::GetPeaks() const {
    std::lock_guard<std::mutex> lk(m_mtx);
    return m_peakDet.GetPeaks();
}

std::array<uint8_t, 2400> TouchPipeline::GetTouchZones() const {
    std::lock_guard<std::mutex> lk(m_mtx);
    return m_zoneExp.GetTouchZones();
}

std::array<uint8_t, 2400> TouchPipeline::GetZoneEdge() const {
    std::lock_guard<std::mutex> lk(m_mtx);
    return m_zoneExp.GetZoneEdge();
}

// ══════════════════════════════════════════════════════════════════════
// GetConfigSchema — unified from all sub-modules
// ══════════════════════════════════════════════════════════════════════
std::vector<ConfigParam> TouchPipeline::GetConfigSchema() const {
    std::vector<ConfigParam> s;

    // Phase 1: Frame Parser
    s.emplace_back("FrameParserEnabled", "Frame Parser Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_frameParser.m_enabled));

    // Phase 2: Baseline
    s.emplace_back("BaselineEnabled", "Baseline Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_baseline.m_enabled));
    s.emplace_back("BaselineValue", "Baseline Value",
                   ConfigParam::Int, const_cast<int*>(&m_baseline.m_baseline), 0, 65535);

    // Phase 2: CMF
    s.emplace_back("CMFEnabled", "CMF Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_cmf.m_enabled));
    s.emplace_back("CMFExclusionThreshold", "CMF Exclusion Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_cmf.m_exclusionThreshold), 50, 2000);
    s.emplace_back("CMFMaxCorrection", "CMF Max Correction",
                   ConfigParam::Int, const_cast<int*>(&m_cmf.m_maxCorrection), 10, 2000);

    // Phase 2: GridIIR
    s.emplace_back("GridIIREnabled", "Grid IIR Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_gridIIR.m_enabled));
    s.emplace_back("GateRatio", "Gate Ratio",
                   ConfigParam::Float, const_cast<float*>(&m_gridIIR.m_gateRatio), 0.02f, 0.30f);
    s.emplace_back("GateStaticFloor", "Gate Static Floor",
                   ConfigParam::Int, const_cast<int*>(&m_gridIIR.m_gateStaticFloor), 50, 500);
    s.emplace_back("DecayWeight", "Decay Weight",
                   ConfigParam::Int, const_cast<int*>(&m_gridIIR.m_decayWeight), 1, 256);
    s.emplace_back("DecayStep", "Decay Step",
                   ConfigParam::Int, const_cast<int*>(&m_gridIIR.m_decayStep), 0, 200);

    // Phase 3: PeakDetector
    s.emplace_back("PeakThreshold", "Peak Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_threshold), 1, 1000);
    s.emplace_back("SigTholdLimit", "Sig Thold Limit",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_sigTholdLimit), 1, 1000);
    s.emplace_back("Z8FilterEnabled", "Z8 Filter Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_z8Filter));
    s.emplace_back("Z1FilterEnabled", "Z1 Filter Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_z1Filter));
    s.emplace_back("PressureDriftFilter", "Pressure Drift Filter",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_pressureDriftFilter));
    s.emplace_back("EdgePeakFilter", "Edge Peak Filter",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_edgePeakFilter));
    s.emplace_back("EdgeThresholdEnabled", "Edge Threshold Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_peakDet.m_edgeThresholdEnabled));
    s.emplace_back("EdgeThreshold", "Edge Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_edgeThreshold), 1, 1000);
    s.emplace_back("Z8Radius", "Z8 Max Search Radius",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_z8Radius), 1, 5);
    s.emplace_back("MaxPeaks", "Peak Limit Cap",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_maxPeaks), 5, 100);
    s.emplace_back("PressureDriftDebounce", "Pressure Debounce Limit",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_pressureDriftDebounceLimit), 0, 10);
    s.emplace_back("MacroZoneMinArea", "MacroZone Min Area",
                   ConfigParam::Int, const_cast<int*>(&m_peakDet.m_macroZoneMinArea), 1, 20);

    // Phase 4: ZoneExpander
    s.emplace_back("DilateErode", "Dilate Erode Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_zoneExp.m_dilateErode));
    s.emplace_back("ZoneTholdScale", "Zone Thold Numer",
                   ConfigParam::Int, const_cast<int*>(&m_zoneExp.m_tholdScaleNumer), 0, 255);
    s.emplace_back("ZoneTholdShift", "Zone Thold Shift",
                   ConfigParam::Int, const_cast<int*>(&m_zoneExp.m_tholdScaleShift), 0, 15);
    s.emplace_back("MaxTouches", "Max Contact Outputs",
                   ConfigParam::Int, const_cast<int*>(&m_zoneExp.m_maxTouches), 1, 50);

    // Phase 4: EdgeCompensation
    s.emplace_back("ECEnabled", "Edge Compensation Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_edgeComp.m_enabled));
    s.emplace_back("ECBlendRange", "EC Blend Range",
                   ConfigParam::Float, const_cast<float*>(&m_edgeComp.m_ecBlendRange), 0.0f, 5.0f);

    // Phase 3: PalmRejector
    s.emplace_back("PalmEnabled", "Palm Rejection Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_palmReject.m_enabled));
    s.emplace_back("PalmAreaThreshold", "Palm Area Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_palmReject.m_areaThreshold), 5, 300);
    s.emplace_back("PalmSignalSumThreshold", "Palm SignalSum Threshold",
                   ConfigParam::Int, const_cast<int*>(&m_palmReject.m_signalSumThreshold), 1000, 500000);
    s.emplace_back("PalmDensityThresholdLow", "Palm Density Low Threshold",
                   ConfigParam::Float, const_cast<float*>(&m_palmReject.m_densityThresholdLow), 50.0f, 2000.0f);
    s.emplace_back("PalmAreaMinForDensity", "Palm Density Min Area",
                   ConfigParam::Int, const_cast<int*>(&m_palmReject.m_areaMinForDensity), 3, 100);
    s.emplace_back("PalmElongatedEnabled", "Elongated Press Reject",
                   ConfigParam::Bool, const_cast<bool*>(&m_palmReject.m_elongatedEnabled));
    s.emplace_back("PalmElongatedMinArea", "Elongated Min Area",
                   ConfigParam::Int, const_cast<int*>(&m_palmReject.m_elongatedMinArea), 3, 100);
    s.emplace_back("PalmElongatedAspectRatio", "Elongated Aspect Ratio",
                   ConfigParam::Float, const_cast<float*>(&m_palmReject.m_elongatedAspectRatio), 1.5f, 10.0f);

    // Phase 5: TouchTracker
    s.emplace_back("TrackerEnabled", "Tracker Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_enabled));
    s.emplace_back("UseHungarian", "Use Hungarian",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_useHungarian));
    s.emplace_back("MaxTrackDistance", "Max Track Dist",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_maxTrackDistance), 1.0f, 20.0f);
    s.emplace_back("AlwaysMatchDistance", "Always Match Dist",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_alwaysMatchDistance), 0.5f, 6.0f);
    s.emplace_back("PredictionScale", "Prediction Scale",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_predictionScale), 0.0f, 2.0f);
    s.emplace_back("LiftOffHoldEnabled", "LiftOff Hold Enable",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_liftOffHoldEnabled));
    s.emplace_back("LiftOffHoldFrames", "LiftOff Hold",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_liftOffHoldFrames), 0, 10);
    s.emplace_back("LiftOffPredictEnabled", "LiftOff Predict",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_liftOffPredictEnabled));
    s.emplace_back("LiftOffVelocityDecay", "LiftOff Vel Decay",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_liftOffVelocityDecay), 0.0f, 1.0f);
    s.emplace_back("TouchDownDebounceFrames", "Down Debounce",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownDebounceFrames), 0, 10);
    s.emplace_back("TouchDownRejectEnabled", "Enable Reject",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_touchDownRejectEnabled));
    s.emplace_back("TouchDownRejectMinSignal", "Reject Signal Th",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_touchDownRejectMinSignal), 0, 500);
    s.emplace_back("StylusSuppressGlobalEnabled", "Pen Global Suppress",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_stylusSuppressGlobalEnabled));
    s.emplace_back("StylusSuppressLocalEnabled", "Pen Local Suppress",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_stylusSuppressLocalEnabled));
    s.emplace_back("StylusSuppressLocalDistance", "Suppress radius",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_stylusSuppressLocalDistance), 0.5f, 10.0f);
    s.emplace_back("StylusAftEnabled", "Enable AFT (Anti-Falsing)",
                   ConfigParam::Bool, const_cast<bool*>(&m_tracker.m_stylusAftEnabled));
    s.emplace_back("StylusAftRadius", "AFT Radius",
                   ConfigParam::Float, const_cast<float*>(&m_tracker.m_stylusAftRadius), 0.5f, 10.0f);
    s.emplace_back("StylusAftSuppressFrames", "AFT Suppress Frames",
                   ConfigParam::Int, const_cast<int*>(&m_tracker.m_stylusAftSuppressFrames), 0, 200);

    // Phase 5: CoordinateFilter
    s.emplace_back("CoordFilterEnabled", "Coord Filter Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_coordFilter.m_enabled));
    s.emplace_back("OneEuroMinCutoff", "1 Euro Min Cutoff",
                   ConfigParam::Float, const_cast<float*>(&m_coordFilter.m_minCutoff), 0.1f, 20.0f);
    s.emplace_back("OneEuroBeta", "1 Euro Beta",
                   ConfigParam::Float, const_cast<float*>(&m_coordFilter.m_beta), 0.0f, 0.5f);
    s.emplace_back("OneEuroDCutoff", "1 Euro Deriv Cutoff",
                   ConfigParam::Float, const_cast<float*>(&m_coordFilter.m_dCutoff), 0.1f, 10.0f);

    // Phase 6: GestureStateMachine
    s.emplace_back("GestureEnabled", "Gesture SM Enabled",
                   ConfigParam::Bool, const_cast<bool*>(&m_gesture.m_enabled));
    s.emplace_back("PressCandidateFrames", "Press Candidate Frames",
                   ConfigParam::Int, const_cast<int*>(&m_gesture.m_pressCandidateFrames), 1, 10);
    s.emplace_back("DragThreshold", "Drag Threshold",
                   ConfigParam::Float, const_cast<float*>(&m_gesture.m_dragThreshold), 0.1f, 5.0f);
    s.emplace_back("LongPressFrames", "LongPress Frames",
                   ConfigParam::Int, const_cast<int*>(&m_gesture.m_longPressFrames), 10, 120);
    s.emplace_back("ReleasePendingFrames", "Release Pending Frames",
                   ConfigParam::Int, const_cast<int*>(&m_gesture.m_releasePendingFrames), 0, 10);
    s.emplace_back("BypassStateMachine", "Bypass State Machine",
                   ConfigParam::Bool, const_cast<bool*>(&m_gesture.m_bypassStateMachine));

    return s;
}

// ══════════════════════════════════════════════════════════════════════
// SaveConfig
// ══════════════════════════════════════════════════════════════════════
void TouchPipeline::SaveConfig(std::ostream& out) const {
    // Phase 1
    out << "FrameParserEnabled=" << (m_frameParser.m_enabled?"1":"0") << "\n";
    // Phase 2: Baseline
    out << "BaselineEnabled=" << (m_baseline.m_enabled?"1":"0") << "\n";
    out << "BaselineValue=" << m_baseline.m_baseline << "\n";
    // Phase 2: CMF
    out << "CMFEnabled=" << (m_cmf.m_enabled?"1":"0") << "\n";
    out << "CMFDimensionMode=" << static_cast<int>(m_cmf.m_mode) << "\n";
    out << "CMFExclusionThreshold=" << m_cmf.m_exclusionThreshold << "\n";
    out << "CMFMaxCorrection=" << m_cmf.m_maxCorrection << "\n";
    // Phase 2: GridIIR
    out << "GridIIREnabled=" << (m_gridIIR.m_enabled?"1":"0") << "\n";
    out << "GateRatio=" << m_gridIIR.m_gateRatio << "\n";
    out << "GateStaticFloor=" << m_gridIIR.m_gateStaticFloor << "\n";
    out << "DecayWeight=" << m_gridIIR.m_decayWeight << "\n";
    out << "DecayStep=" << m_gridIIR.m_decayStep << "\n";
    out << "NoiseFloorCutoff=" << m_gridIIR.m_noiseFloorCutoff << "\n";
    out << "ResidualEnabled=" << (m_gridIIR.m_residualEnabled?"1":"0") << "\n";
    out << "ResidualAlpha=" << m_gridIIR.m_residualAlpha << "\n";
    // Phase 3: PeakDetector (same keys as old FeatureExtractor)
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
    // Phase 4: ZoneExpander
    out << "DilateErode=" << (m_zoneExp.m_dilateErode?"1":"0") << "\n";
    out << "ZoneTholdScale=" << m_zoneExp.m_tholdScaleNumer << "\n";
    out << "ZoneTholdShift=" << m_zoneExp.m_tholdScaleShift << "\n";
    out << "MaxTouches=" << m_zoneExp.m_maxTouches << "\n";
    // Phase 4: EdgeCompensation
    out << "ECEnabled=" << (m_edgeComp.m_enabled?"1":"0") << "\n";
    out << "ECBlendRange=" << m_edgeComp.m_ecBlendRange << "\n";
    // Phase 3: PalmRejector
    out << "PalmEnabled=" << (m_palmReject.m_enabled?"1":"0") << "\n";
    out << "PalmAreaThreshold=" << m_palmReject.m_areaThreshold << "\n";
    out << "PalmSignalSumThreshold=" << m_palmReject.m_signalSumThreshold << "\n";
    out << "PalmDensityThresholdLow=" << m_palmReject.m_densityThresholdLow << "\n";
    out << "PalmAreaMinForDensity=" << m_palmReject.m_areaMinForDensity << "\n";
    out << "PalmElongatedEnabled=" << (m_palmReject.m_elongatedEnabled?"1":"0") << "\n";
    out << "PalmElongatedMinArea=" << m_palmReject.m_elongatedMinArea << "\n";
    out << "PalmElongatedAspectRatio=" << m_palmReject.m_elongatedAspectRatio << "\n";
    // Phase 5: TouchTracker (same keys as old TouchTracker)
    out << "TrackerEnabled=" << (m_tracker.m_enabled?"1":"0") << "\n";
    out << "UseHungarian=" << (m_tracker.m_useHungarian?"1":"0") << "\n";
    out << "MaxTrackDistance=" << m_tracker.m_maxTrackDistance << "\n";
    out << "AlwaysMatchDistance=" << m_tracker.m_alwaysMatchDistance << "\n";
    out << "EdgeTrackBoost=" << m_tracker.m_edgeTrackBoost << "\n";
    out << "AccThresholdBoost=" << m_tracker.m_accThresholdBoost << "\n";
    out << "AccBoostSizeMm=" << m_tracker.m_accBoostSizeMm << "\n";
    out << "PredictionScale=" << m_tracker.m_predictionScale << "\n";
    out << "LiftOffHoldEnabled=" << (m_tracker.m_liftOffHoldEnabled?"1":"0") << "\n";
    out << "LiftOffHoldFrames=" << m_tracker.m_liftOffHoldFrames << "\n";
    out << "LiftOffPredictEnabled=" << (m_tracker.m_liftOffPredictEnabled?"1":"0") << "\n";
    out << "LiftOffVelocityDecay=" << m_tracker.m_liftOffVelocityDecay << "\n";
    out << "LiftOffHoldSpeedThreshold=" << m_tracker.m_liftOffHoldSpeedThreshold << "\n";
    out << "TouchDownDebounceFrames=" << m_tracker.m_touchDownDebounceFrames << "\n";
    out << "DynamicDebounceEnabled=" << (m_tracker.m_dynamicDebounceEnabled?"1":"0") << "\n";
    out << "TouchDownRejectEnabled=" << (m_tracker.m_touchDownRejectEnabled?"1":"0") << "\n";
    out << "TouchDownRejectMinSignal=" << m_tracker.m_touchDownRejectMinSignal << "\n";
    out << "StylusSuppressGlobalEnabled=" << (m_tracker.m_stylusSuppressGlobalEnabled?"1":"0") << "\n";
    out << "StylusSuppressLocalEnabled=" << (m_tracker.m_stylusSuppressLocalEnabled?"1":"0") << "\n";
    out << "StylusSuppressLocalDistance=" << m_tracker.m_stylusSuppressLocalDistance << "\n";
    out << "StylusAftEnabled=" << (m_tracker.m_stylusAftEnabled?"1":"0") << "\n";
    out << "StylusAftRecentFrames=" << m_tracker.m_stylusAftRecentFrames << "\n";
    out << "StylusAftRadius=" << m_tracker.m_stylusAftRadius << "\n";
    out << "StylusAftSuppressFrames=" << m_tracker.m_stylusAftSuppressFrames << "\n";
    // Phase 5: CoordinateFilter
    out << "CoordFilterEnabled=" << (m_coordFilter.m_enabled?"1":"0") << "\n";
    out << "OneEuroMinCutoff=" << m_coordFilter.m_minCutoff << "\n";
    out << "OneEuroBeta=" << m_coordFilter.m_beta << "\n";
    out << "OneEuroDCutoff=" << m_coordFilter.m_dCutoff << "\n";
    // Phase 6: GestureStateMachine
    out << "GestureEnabled=" << (m_gesture.m_enabled?"1":"0") << "\n";
    out << "PressCandidateFrames=" << m_gesture.m_pressCandidateFrames << "\n";
    out << "PressCandidateMinSignal=" << m_gesture.m_pressCandidateMinSignal << "\n";
    out << "PressCandidateMinSizeMm=" << m_gesture.m_pressCandidateMinSizeMm << "\n";
    out << "DragThreshold=" << m_gesture.m_dragThreshold << "\n";
    out << "LongPressFrames=" << m_gesture.m_longPressFrames << "\n";
    out << "LongPressMoveTolerance=" << m_gesture.m_longPressMoveTolerance << "\n";
    out << "ReleasePendingFrames=" << m_gesture.m_releasePendingFrames << "\n";
    out << "BypassStateMachine=" << (m_gesture.m_bypassStateMachine?"1":"0") << "\n";
}

// ══════════════════════════════════════════════════════════════════════
// LoadConfig — key/value dispatch (compatible with old config keys)
// ══════════════════════════════════════════════════════════════════════
void TouchPipeline::LoadConfig(const std::string& key,
                                const std::string& value) {
    auto toBool = [](const std::string& v) { return v=="1"||v=="true"; };
    // Phase 1
    if      (key=="FrameParserEnabled")      m_frameParser.m_enabled = toBool(value);
    // Phase 2: Baseline
    else if (key=="BaselineEnabled")         m_baseline.m_enabled = toBool(value);
    else if (key=="BaselineValue")           m_baseline.m_baseline = std::stoi(value);
    // Phase 2: CMF
    else if (key=="CMFEnabled")              m_cmf.m_enabled = toBool(value);
    else if (key=="CMFDimensionMode")        m_cmf.m_mode = static_cast<Touch::CMFProcessor::DimensionMode>(std::stoi(value));
    else if (key=="CMFExclusionThreshold")   m_cmf.m_exclusionThreshold = std::stoi(value);
    else if (key=="CMFMaxCorrection")        m_cmf.m_maxCorrection = std::stoi(value);
    // Phase 2: GridIIR
    else if (key=="GridIIREnabled")          m_gridIIR.m_enabled = toBool(value);
    else if (key=="GateRatio")               m_gridIIR.m_gateRatio = std::stof(value);
    else if (key=="GateStaticFloor")         m_gridIIR.m_gateStaticFloor = std::stoi(value);
    else if (key=="DecayWeight")             m_gridIIR.m_decayWeight = std::stoi(value);
    else if (key=="DecayStep")               m_gridIIR.m_decayStep = std::stoi(value);
    else if (key=="NoiseFloorCutoff")        m_gridIIR.m_noiseFloorCutoff = std::stoi(value);
    else if (key=="ResidualEnabled")         m_gridIIR.m_residualEnabled = toBool(value);
    else if (key=="ResidualAlpha")           m_gridIIR.m_residualAlpha = std::stof(value);
    // Phase 3: PeakDetector
    else if (key=="PeakThreshold")           m_peakDet.m_threshold = std::stoi(value);
    else if (key=="SigTholdLimit")           m_peakDet.m_sigTholdLimit = std::stoi(value);
    else if (key=="Z8FilterEnabled")         m_peakDet.m_z8Filter = toBool(value);
    else if (key=="Z1FilterEnabled")         m_peakDet.m_z1Filter = toBool(value);
    else if (key=="PressureDriftFilter")     m_peakDet.m_pressureDriftFilter = toBool(value);
    else if (key=="EdgePeakFilter")          m_peakDet.m_edgePeakFilter = toBool(value);
    else if (key=="EdgeThresholdEnabled")    m_peakDet.m_edgeThresholdEnabled = toBool(value);
    else if (key=="EdgeThreshold")           m_peakDet.m_edgeThreshold = std::stoi(value);
    else if (key=="Z8Radius")                m_peakDet.m_z8Radius = std::stoi(value);
    else if (key=="MaxPeaks")                m_peakDet.m_maxPeaks = std::stoi(value);
    else if (key=="PressureDriftDebounce")   m_peakDet.m_pressureDriftDebounceLimit = std::stoi(value);
    else if (key=="MacroZoneMinArea")        m_peakDet.m_macroZoneMinArea = std::stoi(value);
    // Phase 4: ZoneExpander
    else if (key=="DilateErode")             m_zoneExp.m_dilateErode = toBool(value);
    else if (key=="ZoneTholdScale")          m_zoneExp.m_tholdScaleNumer = std::stoi(value);
    else if (key=="ZoneTholdShift")          m_zoneExp.m_tholdScaleShift = std::stoi(value);
    else if (key=="MaxTouches")              m_zoneExp.m_maxTouches = std::stoi(value);
    // Phase 4: EdgeCompensation
    else if (key=="ECEnabled")               m_edgeComp.m_enabled = toBool(value);
    else if (key=="ECBlendRange")            m_edgeComp.m_ecBlendRange = std::stof(value);
    // Phase 3: PalmRejector
    else if (key=="PalmEnabled")             m_palmReject.m_enabled = toBool(value);
    else if (key=="PalmAreaThreshold")       m_palmReject.m_areaThreshold = std::stoi(value);
    else if (key=="PalmSignalSumThreshold")  m_palmReject.m_signalSumThreshold = std::stoi(value);
    else if (key=="PalmDensityThresholdLow") m_palmReject.m_densityThresholdLow = std::stof(value);
    else if (key=="PalmAreaMinForDensity")   m_palmReject.m_areaMinForDensity = std::stoi(value);
    else if (key=="PalmElongatedEnabled")    m_palmReject.m_elongatedEnabled = toBool(value);
    else if (key=="PalmElongatedMinArea")    m_palmReject.m_elongatedMinArea = std::stoi(value);
    else if (key=="PalmElongatedAspectRatio")m_palmReject.m_elongatedAspectRatio = std::stof(value);
    // Phase 5: TouchTracker
    else if (key=="TrackerEnabled")          m_tracker.m_enabled = toBool(value);
    else if (key=="UseHungarian")            m_tracker.m_useHungarian = toBool(value);
    else if (key=="MaxTrackDistance")        m_tracker.m_maxTrackDistance = std::stof(value);
    else if (key=="AlwaysMatchDistance")     m_tracker.m_alwaysMatchDistance = std::stof(value);
    else if (key=="EdgeTrackBoost")          m_tracker.m_edgeTrackBoost = std::stof(value);
    else if (key=="AccThresholdBoost")       m_tracker.m_accThresholdBoost = std::stof(value);
    else if (key=="AccBoostSizeMm")          m_tracker.m_accBoostSizeMm = std::stof(value);
    else if (key=="PredictionScale")         m_tracker.m_predictionScale = std::stof(value);
    else if (key=="LiftOffHoldEnabled")      m_tracker.m_liftOffHoldEnabled = toBool(value);
    else if (key=="LiftOffHoldFrames")       m_tracker.m_liftOffHoldFrames = std::stoi(value);
    else if (key=="LiftOffPredictEnabled")   m_tracker.m_liftOffPredictEnabled = toBool(value);
    else if (key=="LiftOffVelocityDecay")    m_tracker.m_liftOffVelocityDecay = std::stof(value);
    else if (key=="LiftOffHoldSpeedThreshold")m_tracker.m_liftOffHoldSpeedThreshold = std::stof(value);
    else if (key=="TouchDownDebounceFrames") m_tracker.m_touchDownDebounceFrames = std::stoi(value);
    else if (key=="DynamicDebounceEnabled")  m_tracker.m_dynamicDebounceEnabled = toBool(value);
    else if (key=="TouchDownRejectEnabled")  m_tracker.m_touchDownRejectEnabled = toBool(value);
    else if (key=="TouchDownRejectMinSignal")m_tracker.m_touchDownRejectMinSignal = std::stoi(value);
    else if (key=="StylusSuppressGlobalEnabled")  m_tracker.m_stylusSuppressGlobalEnabled = toBool(value);
    else if (key=="StylusSuppressLocalEnabled")   m_tracker.m_stylusSuppressLocalEnabled = toBool(value);
    else if (key=="StylusSuppressLocalDistance")  m_tracker.m_stylusSuppressLocalDistance = std::stof(value);
    else if (key=="StylusAftEnabled")        m_tracker.m_stylusAftEnabled = toBool(value);
    else if (key=="StylusAftRecentFrames")   m_tracker.m_stylusAftRecentFrames = std::stoi(value);
    else if (key=="StylusAftRadius")         m_tracker.m_stylusAftRadius = std::stof(value);
    else if (key=="StylusAftSuppressFrames") m_tracker.m_stylusAftSuppressFrames = std::stoi(value);
    // Phase 5: CoordinateFilter
    else if (key=="CoordFilterEnabled")      m_coordFilter.m_enabled = toBool(value);
    else if (key=="OneEuroMinCutoff")        m_coordFilter.m_minCutoff = std::stof(value);
    else if (key=="OneEuroBeta")             m_coordFilter.m_beta = std::stof(value);
    else if (key=="OneEuroDCutoff")          m_coordFilter.m_dCutoff = std::stof(value);
    // Phase 6: GestureStateMachine
    else if (key=="GestureEnabled")          m_gesture.m_enabled = toBool(value);
    else if (key=="PressCandidateFrames")    m_gesture.m_pressCandidateFrames = std::stoi(value);
    else if (key=="PressCandidateMinSignal") m_gesture.m_pressCandidateMinSignal = std::stoi(value);
    else if (key=="PressCandidateMinSizeMm") m_gesture.m_pressCandidateMinSizeMm = std::stof(value);
    else if (key=="DragThreshold")           m_gesture.m_dragThreshold = std::stof(value);
    else if (key=="LongPressFrames")         m_gesture.m_longPressFrames = std::stoi(value);
    else if (key=="LongPressMoveTolerance")  m_gesture.m_longPressMoveTolerance = std::stof(value);
    else if (key=="ReleasePendingFrames")    m_gesture.m_releasePendingFrames = std::stoi(value);
    else if (key=="BypassStateMachine")      m_gesture.m_bypassStateMachine = toBool(value);
}

} // namespace Engine
