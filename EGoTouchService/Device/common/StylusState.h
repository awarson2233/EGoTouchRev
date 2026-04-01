#pragma once
#include <cstdint>

// ── 手写笔频率命令对 ─────────────────────────────────────────────────────────
// 每支笔在芯片频率表中有两个频点命令（由 set_stylus_id 时绑定）
struct StylusFreqPair {
    uint16_t freq0_cmd = 0;   // 频点0命令（标准频率，默认扫描）
    uint16_t freq1_cmd = 0;   // 频点1命令（240Hz 高频扫描）
};

// ── 用户态驱动侧手写笔状态 ────────────────────────────────────────────────────
// 由 StylusFreqManager 维护，供 ProcessStylusStatus 使用
struct StylusState {
    bool           connected        = false; // 手写笔已通过 BTMCU 连接
    uint8_t        pen_id           = 0;     // 注册的笔 ID（0-3 主表，5=默认）
    StylusFreqPair freqPair;                 // 当前笔的频率命令对
    uint8_t        freqIdx          = 0;     // 当前使用的频点索引（0 或 1）
    uint8_t        switchPolicy     = 0;     // 0=禁用自动切换, 2=启用噪声触发切换
    uint8_t        switchTargetIdx  = 0;     // 目标频点（噪声判断后写入）
    bool           switchReqPending = false; // 切换请求待芯片 IPC 0xBA 确认

    // ── 频率切换防抖 (原厂 HPP3_FreqShiftProcess cooldown = 1000ms) ──
    uint32_t       frameCounter      = 0;     // 帧计数器（全局递增）
    uint32_t       switchReqFrame    = 0;     // 发出切换请求时的帧号
    uint32_t       lastSwitchFrame   = 0;     // 上次完成切换的帧号

    // ── BT MCU 心跳跟踪 (原厂 ASA_SetBluetoothFreq 每帧调用) ──
    uint8_t        btFreq1           = 0;     // BT MCU 报告的 TX1 频率
    uint8_t        btFreq2           = 0;     // BT MCU 报告的 TX2 频率
    bool           btActive          = false; // BT MCU 仍然活跃
    uint32_t       btLastSeenFrame   = 0;     // 最后一次收到 BT 频率的帧号
    bool           btPressureCleared = false; // 1s 超时后是否已清压力

    // ── BT-TP 频率不匹配 debounce (原厂 HPP3_FreqShiftProcess 30ms) ──
    uint32_t       btMismatchFrame   = 0;     // 首次检测到 BT-TP 不匹配的帧号
    bool           btMismatchActive  = false; // 是否处于不匹配 debounce 中
};
