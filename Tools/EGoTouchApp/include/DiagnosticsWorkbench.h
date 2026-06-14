#pragma once

#include "SolverTypes.h"
#include <filesystem>
#include <string>

namespace App {

class ServiceProxy;

class DiagnosticsWorkbench {
public:
    explicit DiagnosticsWorkbench(ServiceProxy* proxy);
    ~DiagnosticsWorkbench();

    void Render();

private:
    // Layout
    void SetupDockLayout(unsigned int dockId);
    void DrawStatusBar();
    void DrawControlPanel();
    void DrawInspectorPanel();
    void DrawLogPanel();
    
    void DrawFilterControls();
    void DrawBehaviorControls();
    void DrawOutputControls();

    void DrawSlaveHeatmap(); // TX1 / TX2 9x9 Heatmap

    // Content panels (drawn inside Inspector tabs)
    void DrawStylusControlPanel();
    void DrawStylusOverviewPanel();
    void DrawStylusServicePolicyPanel();
    void DrawStylusPipelineConfigPanel();
    void DrawStylusCoordinatePanel();
    void DrawStylusPacketDetails();
    void DrawHeatmap();
    void DrawCoordinateTable();
    void DrawTouchInspectorPanel();
    void DrawTouchOverviewPanel();
    void DrawTouchPipelineConfigPanel();
    void DrawTouchPacketDetails();
    void DrawTouchContactSummaryTable();
    void DrawStylusPanel();
    void DrawMasterSuffixTable();
    void DrawSlaveSuffixTable();

    // Tool panels
    void DrawBtMcuPanel();
    void DrawSystemEventsPanel();
    void DrawDvrPanel();
    void DrawApplyConfigResultStatus() const;

    void ExitPlaybackToLivePreview();
    void ExportCurrentFrameCsv(bool isAutoCapture = false);
    void ExportSelectedDvrDatasetToCsv();

private:
    ServiceProxy* m_proxy;
    Solvers::HeatmapFrame m_currentFrame;

    // GUI state
    bool m_autoRefresh = true;
    bool m_renderVisualization = true;
    bool m_showTouchDebugPanel = true;
    bool m_showStylusDebugPanel = true;
    bool m_showStylusControlPanel = false;
    bool m_showMasterSuffixTable = true;
    bool m_showSlaveSuffixTable = true;
    bool m_fullscreen = false;                     // 按下 F11 时切换
    int m_heatmapScale = 10;
    float m_colorRange = 1000.0f;

    // Docking layout
    bool m_dockLayoutApplied = false;
    int m_activeInspectorTab = 0;
    int m_touchConfigModuleIndex = 0;
    int m_stylusConfigModuleIndex = 0;

    // Export
    int m_autoExportTargetPeaks = 0;
    int m_lastPeakCount = 0;
    int m_lastContactCount = 0;
    // Auto-capture mode: 0=disabled, 1=on peak appear, 2=on touch drop (contact disappears)
    int m_autoCaptureMode = 0;



    // Playback UI
    std::filesystem::path m_dvrImportDirectory{"C:/ProgramData/EGoTouchRev/exports"};
    std::string m_lastDvrImportStatus;
    std::string m_lastCsvExportStatus;
};

} // namespace App
