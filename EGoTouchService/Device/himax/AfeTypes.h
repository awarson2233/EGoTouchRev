#pragma once
#include <cstdint>

// ── AFE 命令类型 ──────────────────────────────────────────────────────────
// Himax 触控芯片 AFE (Analog Front-End) 控制命令
enum class AFE_Command : uint8_t {
    ClearStatus = 0,       // 清除状态
    EnableFreqShift,       // 启用频移
    DisableFreqShift,      // 禁用频移
    StartCalibration,      // 开始校准
    EnterIdle,             // 进入空闲
    ForceExitIdle,         // 强制退出空闲
    ForceToFreqPoint,      // 强制切换到指定频点
    ForceToScanRate,       // 强制切换到指定扫描率
    InitStylus,            // 手写笔连接初始化（EnableFreqShift + connect标志）
    SetStylusId,           // 手写笔类型绑定（pen_type → 频率表查询）
    DisconnectStylus,      // 手写笔断连清理（DisableFreqShift + 重置状态）
};

// AFE 扫描模式（值为帧间隔 ms，0=Turbo 无间隔）
enum class THP_AFE_MODE {
    Normal = 200,
    Idle = 500,
    Turbo = 0,
};

// AFE 命令结构体
struct command {
    AFE_Command type;
    uint8_t param = 0;
};
