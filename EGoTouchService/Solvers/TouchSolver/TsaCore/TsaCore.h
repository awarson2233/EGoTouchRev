#pragma once
// ══════════════════════════════════════════════════════════════════════
// TsaCore — TSACore 手指管线（TsaCore/src）对宿主的接口。
//
// 对应原 DLL 的 TSA_* 导出，但不再照搬它的 ABI：结构体只留 TSACore 读取的字段，查询直接返回值，
// 原来要连续几次调用才拼得出的结果（闭环请求、一个报点）合成一次。全局状态是进程级单例，
// 所有调用都必须在同一个线程上。
// ══════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <cstdint>

namespace Solvers::Tsa::Core {

/// AFE 能力（thp_afe_hw_cap 里 TSACore 读取的字段）。
struct AfeCaps {
    uint16_t numCol = 0;
    uint16_t numRow = 0;
    uint8_t numScanFreq = 0;
    const uint16_t* scanFreq = nullptr;
    uint8_t numScanRate = 0;
    const uint16_t* scanRate = nullptr;
    int32_t freqHopMode = 0;      // 1：由 TSACore 决定跳频
    int32_t calibrationMode = 0;  // 2：AFE 自动校准
};

/// 驱动的笔帧（g_StylusOut 里 TSACore 读取的字段）。
struct StylusFrame {
    const uint16_t* tx1 = nullptr;  // TX1 整屏矩阵；没有笔数据时为空
    const uint16_t* tx2 = nullptr;
    uint32_t status = 0;            // StylusStatus
};

/// 一帧 AFE 数据。rawData 为空表示驱动跳过了 master 的那一帧：TSACore 只跑笔与帧后处理。
struct AfeFrame {
    uint64_t timeMs = 0;
    const uint16_t* rawData = nullptr;  // 2400 × uint16，0x7ffe 零点；处理期间必须有效
    uint16_t scanFreq = 0;
    uint16_t scanRate = 0;
    uint32_t status = 0;
    const StylusFrame* stylus = nullptr;
};

/// 本帧的笔报告（业务侧 StylusSolver 解出，替代 ASA 解算本体）。status：bit0 悬停在场，bit1 书写。
/// 坐标是 ASA 坐标系（与 TSACore 的触点同向），量程为显示像素。
struct StylusReport {
    uint32_t status = 0;
    uint16_t x = 0;
    uint16_t y = 0;
    int16_t tiltX = 0;
    int16_t tiltY = 0;
    uint16_t pressure = 0;
};

/// 每帧处理后对 AFE 的请求（ApDaemon 里的 ctrl_buf）。
struct Requests {
    int scanFreqIndex = -1;  // TSA_GetAFEScanFreqIndex；-1 表示没有请求
    int scanRateIndex = -1;  // TSA_GetAFEScanRateIndex
    bool okToEnterIdle = false;
    bool reCalibNeeded = false;
};

/// 一个报点（TSA_Rpt*）。坐标量程是「显示像素 × resScale」。
struct ReportedTouch {
    int id = 0;            // 从 0 起
    bool down = false;     // 在场（TSA_RptIsTouchDown 非零）
    bool liftOff = false;  // 本帧抬起
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t pressure = 0;
};

/// 按机型名装入参数表并初始化（TSA_InitAFEProject_UI）。失败返回 false。
[[nodiscard]] bool Init(const char* project, const AfeCaps& caps);
/// 处理一帧（TSA_ProcessingAFEFrame）。ctrl 是宿主维护的控制字。
void ProcessFrame(const AfeFrame& frame, uint32_t ctrl);
/// 帧后的四项请求。
[[nodiscard]] Requests QueryRequests();

[[nodiscard]] uint32_t GetFeatures();
/// 返回 1 表示请求了不支持的特性。
uint32_t SetFeatures(uint32_t features);
/// 显示分辨率与报点坐标的放大倍数。
void SetResolution(uint16_t xRes, uint16_t yRes, uint16_t resScale);

[[nodiscard]] uint8_t TouchCount();
[[nodiscard]] ReportedTouch TouchAt(uint8_t index);
/// 本帧的 diff 网格（int16，驱动倒序后的方向）。
[[nodiscard]] const int16_t* DiffGrid();
/// 实际装入的机型参数表名（没有专表的机型落到 "DFLT000000"）。
[[nodiscard]] const char* PrmtName();

// ── 笔会话 ──────────────────────────────────────────────────────────
/// 本帧的笔报告，在 ProcessFrame 之前送入。
void SetStylusReport(const StylusReport& report);
void SetBtConnectStatus(uint32_t status);
void SetBtPenStatus(uint8_t status);
/// 蓝牙压力包到达（压力值只给 ASA 解算本体用，这里只记下蓝牙在线）。
void OnBtPressure();
void EnableScenario(uint32_t mask);
void DisableScenario(uint32_t mask);
void SetScreenRotateState(int state);
[[nodiscard]] bool InStylusMode();
[[nodiscard]] bool StylusInRange();

// ── 回放与诊断 ───────────────────────────────────────────────────────
/// 算法状态所在的全部内存（Common/Memory.cpp）。.bss 块的 va 是它在原 DLL 里的地址，堆缓冲的 va 为 0。
struct StateBlock {
    const uint8_t* data;
    uint64_t size;
    uint64_t va;
};
/// 返回块数；capacity 不够时只返回需要的块数，登记表溢出时返回 (size_t)-1。
size_t StateBlocks(StateBlock* out, size_t capacity);

/// 分段计时（Common/Profile.cpp）：构建时打开 EGO_TSACORE_PROFILE 才有数据。name 是调用表达式，
/// "TSA_ProcessingAFEFrame" 一项是整帧。
struct ProfileEntry {
    const char* name;
    uint64_t ns;
    uint64_t count;
};
/// 返回项数，只写入前 capacity 项。
size_t ProfileEntries(ProfileEntry* out, size_t capacity);
/// 分步计时的开关（默认开）。关掉后只记整帧，没有各段计时自身的开销。
void ProfileSteps(bool enable);

}  // namespace Solvers::Tsa::Core
