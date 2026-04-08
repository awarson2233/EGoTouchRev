#pragma once

#include "ServiceProxy.h"
#include "GuiLogSink.h"
#include "SystemStateMonitor.h"
#include <string>
#include <vector>

namespace App {

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
    void DrawTouchSolverPanel();
    void DrawTouchTrackingPanel();
    void DrawStylusControlPanel();
    void DrawHeatmap();
    void DrawCoordinateTable();
    void DrawStylusPanel();
    void DrawMasterSuffixTable();
    void DrawSlaveSuffixTable();

    // Tool panels (drawn inside Inspector tabs)
    void DrawBtMcuPanel();
    void DrawSystemEventsPanel();

    void ExportCurrentFrameToCSV(bool isAutoCapture = false);

private:
    ServiceProxy* m_proxy;
    Engine::HeatmapFrame m_currentFrame;

    // GUI state
    bool m_autoRefresh = true;
    bool m_renderVisualization = true;
    bool m_showTouchDebugPanel = true;
    bool m_showStylusDebugPanel = true;
    bool m_showTouchSolverPanel = true;
    bool m_showTouchTrackingPanel = true;
    bool m_showStylusControlPanel = false;
    bool m_showMasterSuffixTable = true;
    bool m_showSlaveSuffixTable = true;
    bool m_fullscreen = false;                     // 按下 F11 时切换
    bool m_showSlaveHeatmap = false;               // Slider for 9x9 Slave Heatmap
    int m_heatmapScale = 10;
    float m_colorRange = 1000.0f;

    // Docking layout
    bool m_dockLayoutApplied = false;
    int m_activeInspectorTab = 0;

    // Export
    bool m_exportHeatmap = true;
    bool m_exportMasterStatus = false;
    bool m_exportSlaveStatus = false;
    int m_autoExportTargetPeaks = 0;
    int m_lastPeakCount = 0;
    int m_lastContactCount = 0;
    // Auto-capture mode: 0=disabled, 1=on peak appear, 2=on touch drop (contact disappears)
    int m_autoCaptureMode = 0;

    // AFE control
    int m_afeIdleParam = 0;
    int m_afeCalibrationParam = 0;
    int m_afeClearStatusParam = 1;
    int m_afeForceFreqIdx = 0;
    int m_afeForceScanRateIdx = 0;
    bool m_scanRateIs240Hz = false;
    std::string m_lastAfeActionStatus = "No command sent";
};

} // namespace App
