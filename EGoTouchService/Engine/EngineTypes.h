#pragma once
#include <vector>
#include <cstdint>
#include <array>

namespace Engine {

enum TouchContactState : int {
    TouchStateDown = 0,
    TouchStateMove = 1,
    TouchStateUp = 2,
};

enum TouchLifeFlagBits : uint32_t {
    TouchLifeMapped = 1u << 0,
    TouchLifeNew = 1u << 1,
    TouchLifeLiftOff = 1u << 2,
    TouchLifeEdge = 1u << 3,
    TouchLifeDebounced = 1u << 4,
    TouchLifeAlwaysMatch = 1u << 5,
};

enum TouchReportEventCode : int {
    TouchReportIdle = 1,
    TouchReportDown = 2,
    TouchReportMove = 4,
    TouchReportUp = 0x20,
};

// 触摸点结构体 (用于 Stage 2 连通域计算)
struct TouchContact {
    int id = 0;
    float x = 0.0f;
    float y = 0.0f;
    int state = 0; // 0=Down, 1=Update, 2=Up
    int area = 0;  // 连通域大小或强度
    int signalSum = 0; // 区域信号总和(对齐 TS 的 SigSum 语义)

    // Extended fields for TS/TE/TouchReport-aligned processing.
    float sizeMm = 0.0f;
    bool isEdge = false;
    bool isReported = true;
    int prevIndex = -1;
    int debugFlags = 0;

    // TS/TE/TouchReport-aligned state mirrors
    uint32_t lifeFlags = 0;
    uint32_t reportFlags = 0;
    int reportEvent = 0;
};

struct TouchPacket {
    bool valid = false;
    uint8_t reportId = 0x01;
    uint8_t length = 0x20;
    std::array<uint8_t, 32> bytes{};
};

struct TouchPeak {
    int r = 0;
    int c = 0;
    int16_t z = 0;
    uint8_t id = 0;
};

// Represents a connected component in the heatmap greater than a global threshold
struct MacroZone {
    std::vector<int> pixels; // 1D indices (r * cols + c)
    int area = 0;
};

struct StylusSolvePoint {
    bool valid = false;
    float x = 0.0f;
    float y = 0.0f;
    uint16_t reportX = 0;
    uint16_t reportY = 0;
    uint16_t pressure = 0;
    uint16_t rawPressure = 0;
    uint16_t mappedPressure = 0;
    uint16_t peakTx1 = 0;
    uint16_t peakTx2 = 0;
    bool tiltValid = false;
    int16_t preTiltX = 0;
    int16_t preTiltY = 0;
    int16_t tiltX = 0;
    int16_t tiltY = 0;
    float tiltMagnitude = 0.0f;
    float tiltAzimuthDeg = 0.0f;
    float tx1X = 0.0f;
    float tx1Y = 0.0f;
    float tx2X = 0.0f;
    float tx2Y = 0.0f;
    float confidence = 0.0f;
};

struct StylusPacket {
    bool valid = false;
    uint8_t reportId = 0x08;
    uint8_t length = 17;
    std::array<uint8_t, 17> bytes{};
};

struct StylusFrameData {
    bool slaveValid = false;
    bool checksumOk = false;
    uint8_t slaveWordOffset = 0;
    uint16_t checksum16 = 0;
    bool tx1BlockValid = false;
    bool tx2BlockValid = false;
    std::array<uint16_t, 166> slaveWords{};

    int16_t tx1Matrix[40][60]{};
    int16_t tx2Matrix[40][60]{};

    uint32_t status = 0;
    uint16_t tx1Freq = 0x00A1;
    uint16_t tx2Freq = 0x0018;
    uint16_t pressure = 0;
    uint32_t button = 0;
    uint16_t nextTx1Freq = 0x00A1;
    uint16_t nextTx2Freq = 0x0018;
    bool masterMetaValid = false;
    uint8_t masterMetaBaseWord = 0xFF;
    uint16_t masterMetaTx1Freq = 0;
    uint16_t masterMetaTx2Freq = 0;
    uint16_t masterMetaPressure = 0;
    uint32_t masterMetaButton = 0;
    uint32_t masterMetaStatus = 0;
    uint32_t rawButton = 0;
    uint8_t buttonSource = 0; // 0=None, 1=MasterMeta, 2=SlaveWord

    // ASA/HPP process mirror fields for debug/alignment.
    uint8_t asaMode = 0;        // 0=None, 1=HPP2, 2=HPP3
    uint8_t dataType = 0;       // 0=Line, 1=IQLine, 2=Grid, 3=TiedGrid
    uint8_t processResult = 5;  // 0=Output, 1=InvalidReset, 3=Release, 5=Bypass
    bool validJudgmentPassed = false;
    bool freqShiftReleasing = false;
    bool modeExitRelease = false;
    bool noPressInkActive = false;
    bool hpp3NoiseInvalid = false;
    bool hpp3NoiseDebounce = false;
    bool hpp3Dim1SignalValid = false;
    bool hpp3Dim2SignalValid = false;
    uint8_t hpp3RatioWarnCountX = 0;
    uint8_t hpp3RatioWarnCountY = 0;
    uint16_t hpp3SignalAvgX = 0;
    uint16_t hpp3SignalAvgY = 0;
    uint8_t hpp3SignalSampleCount = 0;

    // StylusRecheck/TSA_ASAProcess suppression mirror fields.
    bool recheckEnabled = false;
    bool recheckPassed = true;
    bool recheckOverlap = false;
    uint16_t recheckThreshold = 0;
    bool touchNullLike = false;
    bool touchSuppressActive = false;
    uint8_t touchSuppressFrames = 0; // Remaining hold frames after current frame.

    // Signals used by StylusRecheck-like gates.
    uint16_t signalX = 0;
    uint16_t signalY = 0;
    uint16_t maxRawPeak = 0;

    StylusSolvePoint point{};
    StylusPacket packet{};

    // Phase 6: AnimationProcess state output (Idle/PenDown/Writing/Lifting)
    uint8_t animState = 0;

    // Diagnostic: which pipeline stage produced this result
    // 0=ok, 1=slaveParseFail, 2=tx1Invalid, 3=noPeak, 4=coordFail, 5=noiseReject
    uint8_t pipelineStage = 0;

    // ── Pipeline Diagnostics (shared POD struct, single-copy from StylusPipeline) ──
    // Layout matches StylusPipeline::DbgCoordBreakdown exactly.
    struct StylusDiagnostics {
        // Stage 1: Coordinate solver
        uint16_t anchorRow = 0;
        uint16_t anchorCol = 0;
        int32_t  rawDim1   = 0;
        int32_t  rawDim2   = 0;
        int32_t  finalDim1 = 0;
        int32_t  finalDim2 = 0;
        float    centerOff = 0.f;
        float    pointX    = 0.f;
        float    pointY    = 0.f;
        bool     valid     = false;
        // Stage 2: Post-processing metrics
        float    speedInstant  = 0.f;
        float    speedShortAvg = 0.f;
        float    speedFullAvg  = 0.f;
        float    iirCoef       = 0.f;
        bool     isHover       = false;
        bool     isEdge        = false;
        // Stage 3: Tilt
        float    tiltDiffX  = 0.f;
        float    tiltDiffY  = 0.f;
        // Stage 4: Pressure / Signal
        uint16_t peakSignal     = 0;
        uint16_t rawPressure    = 0;
        uint16_t mappedPressure = 0;
        // Stage 5: VHF state
        uint8_t  vhfPenState      = 0;
        uint8_t  linearFilterState = 0;
        // Stage 6: P3/P4 Extended diagnostics
        uint16_t signalRatio       = 0;
        bool     freqShiftFreezing = false;
        bool     exitSmoothed      = false;
        bool     cmfEnabled        = false;
        bool     coorReviserActive = false;
        float    coorRevDeltaX     = 0.f;
        float    coorRevDeltaY     = 0.f;
        bool     tiltAnomalyDamped = false;
        bool     sigSuppressActive = false;
        uint8_t  penLifecycle      = 0;
        bool     wasInking         = false;
        int32_t  avg3PtDim1        = 0;
        int32_t  avg3PtDim2        = 0;
    };
    StylusDiagnostics diag{};
};

// 整个管线中流转的帧结构体
struct HeatmapFrame {
    // 原始下发的 5063 字节数据 （通常在解析后可以释放掉或清空）
    std::vector<uint8_t> rawData;
    
    // 40 x 60 的热力图矩阵, 数据类型 int16_t (便于基线减去后支持负数的死区操作)
    int16_t heatmapMatrix[40][60];
    
    // 从 heatmap 中解析出来的触控点列表
    std::vector<TouchContact> contacts;
    std::array<TouchPacket, 2> touchPackets{};

    // 识别出的原始波峰和区域连通图映射（供 IPC 可视化）
    std::vector<TouchPeak> peaks;
    std::array<uint8_t, 2400> touchZones{}; // Now used for MacroZones
    std::array<uint8_t, 2400> peakZones{};  // Used for MicroZones (per-peak segmented regions)

    // Stylus data parsed from slave overlay and solved in StylusProcessor.
    StylusFrameData stylus;

    // 时间戳或其他元数据
    uint64_t timestamp;

    // 帧采集元数据：master 是否在本帧被实际读取（false = 2:1 交错跳过）
    bool masterWasRead = true;

    HeatmapFrame() : timestamp(0) {
        // 初始化矩阵全0
        for (int i=0; i<40; ++i) {
            for (int j=0; j<60; ++j) {
                heatmapMatrix[i][j] = 0;
            }
        }
    }
};

} // namespace Engine
