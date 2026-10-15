#pragma once
// AFE 接口（原 src/Extension/TSA_AFE）：宿主送来的 AFE 能力与每帧状态、扫描频点 / 扫描率的选择
// （跳频与降扫描率），以及 AFE 自检模式（控制位 0x100000）里的频点遍历、IDLE 进出与接近自检。

#include <cstdint>

namespace Solvers::Tsa::Core::Afe {

/// thp_afe_hw_cap（HAL spec 0x011a00 布局），只列读取的字段。
struct Caps {
    uint16_t numCol;
    uint16_t numRow;
    uint8_t pad04[6];
    uint8_t numScanFreq;
    uint8_t pad0b[5];
    const uint16_t* scanFreq;
    uint8_t numScanRate;
    uint8_t pad19[7];
    const uint16_t* scanRate;
    uint8_t pad28[4];
    int32_t freqHopMode;      // 1：由 TSACore 决定跳频
    int32_t calibrationMode;  // 2：AFE 自动校准
    uint8_t pad34[0x60 - 0x34];
};
static_assert(sizeof(Caps) == 0x60);

/// AFE 帧（TSA_ProcessingAFEFrame 的第一参）。
struct Frame {
    uint32_t timestampSec;
    uint32_t timestampUsec;
    uint16_t sequence;
    uint8_t pad0a[6];
    uint16_t* rawData;
    uint16_t* lineData;
    uint64_t q20;
    uint64_t q28;
    uint16_t scanFreq;
    uint16_t scanRate;
    uint32_t status;          // kStatus*
    uint32_t gesture;
    uint8_t pad3c[4];
    const void* stylusData;
    uint64_t q48;
    uint64_t q50;
    uint32_t d58;
    uint8_t pad5c[4];
};
static_assert(sizeof(Frame) == 0x60);

[[nodiscard]] bool Malloc();
void Init();
/// 宿主的能力 / 帧拷进本模块的副本（帧的互容与自容数据再拷进自己的缓冲）。
void SetCaps(const Caps* caps);
void SetFrame(const Frame* frame);
/// 宿主的 AFE 结构（+0x40 是版本名）。
void SetInfo(const void* info);
void SetMaxScanRate(uint16_t rate);
[[nodiscard]] Frame* GetFrame();
[[nodiscard]] const Caps* GetCaps();
[[nodiscard]] uint32_t FrameStatus();

/// 本帧开始：按帧上报的频点 / 扫描率更新当前索引。
void PreProcess();
/// 本帧结束：手动跳频时决定下一个频点与扫描率。
void Process();
/// 宿主查询要切到的频点 / 扫描率索引：返回 false 时 *index 是要切到的索引。
[[nodiscard]] bool GetNextScanFreqIndex(uint8_t* index);
[[nodiscard]] bool GetNextScanRateIndex(uint8_t* index);

/// 自检模式的一帧（取代 TSA_Processing）。
void AutoCheckProcess(const uint16_t* data, uint16_t size, uint32_t ctrl, uint64_t timeMs);
[[nodiscard]] bool IsInAutoCheck();
void ClearAutoCheckFlag();
[[nodiscard]] bool AutoCheckToEnterIdle();

[[nodiscard]] uint16_t ColNum();
[[nodiscard]] uint16_t RowNum();
[[nodiscard]] bool IsInIdle();
[[nodiscard]] bool IsInSideIdle();
[[nodiscard]] bool IsProximityScreenOff();
[[nodiscard]] bool IsRecalRequested();
[[nodiscard]] bool IsCoreTouchDetected();
[[nodiscard]] bool IsRawJump();
[[nodiscard]] bool IsRawUnstable();
[[nodiscard]] bool IsManualFreqHopping();
[[nodiscard]] bool IsAutoCalibration();
[[nodiscard]] bool IsAllFreqNoisy();
[[nodiscard]] bool IsFreqShiftDone();
[[nodiscard]] bool IsFrequentlyHopping();
/// 以下三项只对 ALPS491000 / ALPS491600 机型成立。
[[nodiscard]] bool IsAlwaysNoisy();
[[nodiscard]] bool IsRawUnifiedToTargetValue();
[[nodiscard]] bool IsIndividualBaselineForEachFreq();

}  // namespace Solvers::Tsa::Core::Afe
