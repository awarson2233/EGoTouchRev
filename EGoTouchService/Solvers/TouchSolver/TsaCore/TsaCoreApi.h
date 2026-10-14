#pragma once
// ══════════════════════════════════════════════════════════════════════
// TsaCoreApi — 宿主用到的 TSACoreRaw 导出，以及宿主要填的三个 AFE 结构体。
//
// raw/include 下的头文件带上千个镜像地址宏，不能进 C++ 翻译单元；这里只按
// 原 DLL 的导出签名重新声明用得到的那几个。结构体布局对应 AFE HAL spec
// 0x011a00（AFE_SetCapsNormal / AFE_SetFrameNormal 的拷贝范围）。
// ══════════════════════════════════════════════════════════════════════

#include <cstdint>

extern "C" {

void TSACoreRaw_InitImage(void);

uint64_t TSA_InitAFEProject_UI(int64_t projectName, int64_t afeStruct, int64_t caps);
int      TSA_ProcessingAFEFrame(void* frame, uint32_t ctrl, uint64_t timestampMs);

uint32_t TSA_GetFeatures(void);
uint32_t TSA_SetFeatures(uint32_t features);
void     TSA_SetXRes(uint16_t res);
void     TSA_SetYRes(uint16_t res);
uint64_t TSA_SetResScale(uint16_t scale);

uint32_t TSA_RptTouchNum(void);
int      TSA_RptTouchID(uint8_t index);
uint32_t TSA_RptIsTouchDown(uint8_t index);
uint32_t TSA_RptIsLiftOff(uint8_t index);
uint64_t TSA_RptTouchXPos(uint8_t index);
uint64_t TSA_RptTouchYPos(uint8_t index);
uint32_t TSA_RptTouchPressure(uint8_t index);
uint64_t TSA_RptDiffPtr(void);

// 闭环请求（ApDaemon 每帧处理后查询）。两个 GetAFEScan*Index 返回 1 表示没有请求，
// 返回 0 时 *outIndex 是要切到的索引。
uint64_t TSA_GetAFEScanFreqIndex(int64_t outIndex);
uint64_t TSA_GetAFEScanRateIndex(int64_t outIndex);
uint8_t  TSA_IsOKToEnterIDLEMode(void);
uint8_t  TSA_IsReCalibNeeded(void);

// 笔会话状态（ApDaemon 在笔状态字变化、蓝牙压力包到达时调用）。
void     TSA_SetASABluetoothConnectStatus(uint32_t status);
void     TSA_SetASABluetoothPenStatus(uint8_t status);
void     TSA_SetASABluetoothPressure(uint16_t p0, uint16_t p1, uint16_t p2, uint16_t p3);
void     TSA_EnableScenario(uint32_t mask);
void     TSA_DisableScenario(uint32_t mask);
void     TSA_SetScreenRotateState(int state);
uint64_t TSA_InStylusMode(void);
uint64_t TSA_RptASAInRange(void);

// 宿主桥（raw/Rejection/ASAHostBridge.c）：本帧的笔报告，在 TSA_ProcessingAFEFrame 之前送入。
// statusFlags：bit0 悬停在场，bit1 书写。坐标是 ASA 坐标系（与 TSACore 的触点同向），量程为显示像素。
void     TSAHost_SetStylusReport(uint32_t statusFlags, uint16_t x, uint16_t y, int16_t tiltX, int16_t tiltY,
                                 uint16_t pressure);

}  // extern "C"

namespace Solvers::Tsa {

/// thp_afe_hw_cap，0x60 字节。只列 TSACore 实际读取的字段，其余保持 0。
struct alignas(8) AfeCaps {
    uint16_t numCol;            // +0x00
    uint16_t numRow;            // +0x02
    uint8_t  pad04[6];
    uint8_t  numScanFreq;       // +0x0a
    uint8_t  pad0b[5];
    const uint16_t* scanFreq;   // +0x10
    uint8_t  numScanRate;       // +0x18
    uint8_t  pad19[7];
    const uint16_t* scanRate;   // +0x20
    uint8_t  pad28[0x60 - 0x28];
};
static_assert(sizeof(AfeCaps) == 0x60);

/// AFE_Frame +0x40 指向的笔帧（驱动的 g_StylusOut），0x20 字节。
struct alignas(8) StylusFrame {
    const uint16_t* tx1;        // +0x00  TX1 整屏矩阵；没有笔数据时为空
    const uint16_t* tx2;        // +0x08
    uint16_t mainFreq;          // +0x10
    uint16_t auxFreq;           // +0x12
    uint16_t pressure;          // +0x14
    uint8_t  pad16[2];
    uint32_t buttonBits;        // +0x18
    uint32_t status;            // +0x1c  StylusStatus
};
static_assert(sizeof(StylusFrame) == 0x20);

/// AFE_Frame，TSA_ProcessingAFEFrame 的第一参。
struct alignas(8) AfeFrame {
    uint32_t timestampSec;      // +0x00
    uint32_t timestampUsec;     // +0x04
    uint16_t sequence;          // +0x08
    uint8_t  pad0a[6];
    const uint16_t* rawData;    // +0x10  2400 × uint16，0x7ffe 零点；驱动跳过 master 的帧为空
    const uint16_t* lineData;   // +0x18  自容数据；本机型未启用，必须为空
    uint64_t q20;
    uint64_t q28;
    uint16_t scanFreq;          // +0x30
    uint16_t scanRate;          // +0x32
    uint32_t status;            // +0x34
    uint32_t gesture;           // +0x38
    uint8_t  pad3c[4];
    const StylusFrame* stylusData;  // +0x40
    uint64_t q48;
    uint64_t q50;
    uint32_t d58;
    uint8_t  pad5c[4];
};
static_assert(sizeof(AfeFrame) == 0x60);

/// TSA_InitAFEProject_UI 的第二参。TSACore 保存这个指针（g_thpAFEStruct），运行期经
/// frame 读 status / 扫描频率，经 caps 读行列数，所以对象必须活到进程结束。
struct alignas(8) AfeHostStruct {
    const void* info;           // +0x00
    const AfeCaps* caps;        // +0x08
    const AfeFrame* frame;      // +0x10
    uint8_t pad18[0x40 - 0x18];
    char name[32];              // +0x40
};
static_assert(sizeof(AfeHostStruct) == 0x60);

}  // namespace Solvers::Tsa
