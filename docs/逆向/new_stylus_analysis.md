
---

# 文档目录

## 第一章：顶层入口函数 TSA_ASAProcess ✅（已输出）
- 1.1 函数签名与全局状态初始化
- 1.2 帧数据获取与多面板切换
- 1.3 状态位域 `status` (+0x1C) 全解析
- 1.4 三大主分支：NULL / DISABLED / NORMAL
- 1.5 正常分支内：PreInRange 保存与主处理
- 1.6 StylusRecheck 与触摸抑制逻辑
- 1.7 触摸抑制计数器与笔进入/退出
- 1.8 尾部处理

## 第二章：核心管线 ASA_MainProcess
- 2.1 时间戳与帧上下文构建
- 2.2 频率跳变预处理 HPP3_FreqShiftProcess
- 2.3 帧有效性判断 ValidJudgment（全分支覆盖）
- 2.4 释放抑制逻辑（`DAT_1823195c & 0x10` 分支）
- 2.5 HPP2/HPP3 协议分发与数据类型路由
- 2.6 坐标后处理管线 ASA_CoorPostProcess
- 2.7 动画状态机 AnimationProcess
- 2.8 帧缓存与校准 `memcpy(g_prevASOut, g_curASOut)`

## 第三章：ValidJudgment — 协议与模式校验（全7分支）
- 3.1 双协议同时设置错误（bit0 & bit1 同时为1）
- 3.2 无协议设置错误（bit0 & bit1 同时为0）
- 3.3 单协议无模式错误（bit2/3/4 全零）
- 3.4 单协议全模式设置错误（bit2/3/4 全为1）
- 3.5 正常路径：TX1/TX2 线数据 NULL 检查
- 3.6 各分支返回值与日志总结表

## 第四章：HPP3 数据处理管线 — 从传感器到坐标
- 4.1 数据类型路由 `g_flagDataType` switch-case（Line/IQLine/Grid/TiedGrid）
- 4.2 TX1 线数据处理 ASA_HPP3TX1LineDataProcesss
  - 4.2.1 CMF 处理与数据质量检查
  - 4.2.2 TX1/TX2 峰值检测 TX1LinePeaksProcess / TX2LinePeaksProcess
  - 4.2.3 噪声处理 HPP3_NoiseProcess / HPP3_NoisePostProcess
  - 4.2.4 信号更新与校准刷新 ASA_Refresh
  - 4.2.5 静态状态判定 ASAStaticStatusProcess
  - 4.2.6 落笔/抬笔/Out-of-Range 三分支
- 4.3 HPP3 后处理 ASA_HPP3Process
  - 4.3.1 压力处理 HPP3_PressureProcess
  - 4.3.2 无压感墨迹 NoPressInkProcess
  - 4.3.3 边缘坐标修正 EdgeCoorProcess

## 第五章：坐标计算算法
- 5.1 三角插值法 GetCoordinateByTriangleOf — 三点抛物线拟合
- 5.2 重心法 GetCoordinateByGravityOf — 加权质心算法
- 5.3 多阶曲线补偿 CoorMultiOrderFitCompensate — 三次多项式非线性校正
- 5.4 传感器TP图案补偿 CoorTpPatternCompensate
- 5.5 传感器间距映射 SensorPitchSizeMapDim1/Dim2
- 5.6 坐标各阶段命名约定与数据流图

## 第六章：坐标后处理滤波管线 ASA_CoorPostProcess
- 6.1 直线滤波器 LinearFilterProcess — 直线/曲线检测状态机（7态）
- 6.2 实时坐标缓冲 GetRealTimeCoor2Buf — 24帧环形缓冲区
- 6.3 三点均值滤波 Get3PointAvgFilter — 移动平均 + 一阶差分
- 6.4 坐标修正 CoorReviseProcess — TX2双频校正
- 6.5 速度计算 GetCoorSpeed — 欧氏距离与累积/瞬时速度
- 6.6 IIR 系数自适应 GetIIRCoef — 速度自适应平滑系数
- 6.7 IIR 坐标滤波 CoorFilterProcess / CoorIIRFilter — 一阶IIR低通
- 6.8 抖动抑制 AftCoorProcess — 起笔死区锁定算法
- 6.9 屏幕映射 FitToLcdScreen / GetClipReport — 传感器坐标到LCD像素

## 第七章：倾斜角算法 TiltProcess
- 7.1 TX1/TX2 坐标差求取与有效性校验
- 7.2 信号比计算 GetTX1TX2SignalRatio
- 7.3 坐标差 IIR 平滑 + 长度限幅
- 7.4 倾斜角查表 GetTiltByCoorDif
- 7.5 倾斜角输出滤波：5帧均值 + 1度抖动抑制 Tilt1DegreeJitFilter

## 第八章：压力管线
- 8.1 Bluetooth 压力源与映射 HPP3_PressureProcess
- 8.2 压力 IIR 滤波 PressureIIR
- 8.3 信号强度抑制 HPP3_SuppressBtPressBySignal
- 8.4 无压感墨迹学习 NoPressInkLearningProcess

## 第九章：频率跳变引擎 HPP3_FreqShiftProcess
- 9.1 蓝牙状态与频率更新 HPP3_BtStatusProcess / HPP3_UpdateFreq
- 9.2 频率合法性校验
- 9.3 TP-BT 频率匹配检测 HPP3_IsTPandBTMatch
- 9.4 频率搜索 SearchForNewFreq
- 9.5 频率跳变完成检测 CheckIsFreqShiftDone
- 9.6 `g_freqShift` 状态位域解析

## 第十章：动画状态机 AnimationProcess
- 10.1 热区方向判定（4个屏幕角落）
- 10.2 起笔/落笔位移计算
- 10.3 动画长度阈值与状态转换表

## 第十一章：关键结构体与全局变量总表
- 11.1 手写笔子帧结构体完整映射
- 11.2 g_curASOut / g_prevASOut 输出结构体（0xEC 字节）
- 11.3 坐标管线各阶段变量地址映射表
- 11.4 参数 Flash 偏移表（g_asaPrmtFlash）

## 第十二章：完整数据流图
- 12.1 Raw Sensor → Coordinate → Screen 全链路流水线图
- 12.2 各滤波器算法模型总结表（IIR/FIR/三角/重心/多项式）

---

# TSACore 模块 TSA_ASAProcess 手写笔管线算法深度逆向分析

## 第一章：顶层入口函数 TSA_ASAProcess

### 1.1 函数签名与全局状态初始化

```c
void TSA_ASAProcess(uint param_1)
```

**参数 `param_1`**：帧控制标志位。低 4 位 (`param_1 & 0xF`) 非零时，在函数末尾强制触发 `ASA_Reset()`，用于外部发起的管线重置请求。

**逻辑块 A — 初始化阶段：**

```c
// 获取传感器网格维度
uVar3 = (uint)g_gridRows;     // 行数（Dim2 方向传感器通道数）
uVar7 = (uint)g_gridCols;     // 列数（Dim1 方向传感器通道数）

// 获取信号差分指针，设置给 ASA 子系统
uVar5 = TSAMS_GetSSDifPtr();
ASA_SetSSDiffdata(uVar5, uVar3 + uVar7);  // 总通道数 = rows + cols

// 清除"当前帧异常退出"标志
g_isAsaCurFrameExitAbnormal = 0;

// 在全局状态字 g_tsaStaticPtr+0x10C 中设置 bit6 = 1
// 含义：标记"ASA 管线正在执行中"
*(g_tsaStaticPtr + 0x10c) |= 0x40;
```

> **物理含义**：`g_gridRows` / `g_gridCols` 对应触摸面板上两个正交方向（通常是 X/Y 或 Dim1/Dim2）的传感器电极数量。`SSDiff` 是"Self-Sensing Difference"的缩写，即自感差分数据，用于笔信号提取。

---

### 1.2 帧数据获取与多面板切换

```c
piVar6 = (int *)AFE_GetFrame();          // 获取当前 AFE(模拟前端) 帧
lVar1 = *(longlong *)(piVar6 + 0x10);    // 偏移 0x40 字节处: 手写笔子帧指针
ASA_MultiPanleSwitchProcess(lVar1);       // 多面板切换处理
```

**帧结构体偏移分析（`AFE_GetFrame()` 返回结构体）**：

| 偏移 | 类型 | 含义 |
|------|------|------|
| `+0x00` | `int` | 时间戳秒部分 |
| `+0x04` | `int` | 时间戳微秒部分 |
| `+0x08` | `int` | 帧序号 |
| `+0x40` (`piVar6+0x10`) | `longlong` (指针) | **手写笔子帧数据指针** |

**手写笔子帧结构体（`lVar1` 指向的结构体）关键偏移**：

| 偏移 | 类型 | 物理含义 | 证据来源 |
|------|------|----------|----------|
| `+0x00` | `pointer` | TX1 线数据指针 | `ValidJudgment` 中 `*param_1 == 0` 判断 |
| `+0x08` | `pointer` | TX2 线数据指针 | `ValidJudgment` 中 `param_1[1] == 0` 判断 |
| `+0x10` | `uint16` | TX1 频率 | 日志 `"tx1_freq:%4d"` |
| `+0x12` | `uint16` | TX2 频率 | 日志 `"tx2_freq:%4d"` |
| `+0x14` | `uint16` | 压力原始值 | 日志 `"press:%4d"` |
| `+0x18` | `uint32` | 按钮状态码 | 日志 `"btn:%4d"` |
| `+0x1C` | `uint32` | **状态位域（核心）** | 下文详述 |

---

### 1.3 状态位域 `status` (+0x1C) 全解析

这是整个管线最关键的路由字段，`ValidJudgment` 和 `ASA_MainProcess` 中的所有主要分支都由它驱动。

| Bit | 掩码 | 含义 | 来源 |
|-----|------|------|------|
| bit0 | `0x01` | HPP2 协议标志 | `ValidJudgment`: 必须选择一个协议 |
| bit1 | `0x02` | HPP3 协议标志 | 与 bit0 互斥，不能同时置 1 |
| bit2 | `0x04` | 模式: 有效数据标志 | "MODE ERROR" 检查 |
| bit3 | `0x08` | 模式: 有效数据标志 | |
| bit4 | `0x10` | 模式: 有效数据标志 / 释放抑制 | `ASA_MainProcess` 中释放逻辑 |
| bit5 | `0x20` | 频率跳变 TP 完成标志 | `HPP3_FreqShiftProcess` |
| bit6 | `0x40` | 频率跳变 BT 完成标志 | |
| bit7 | `0x80` | 频率跳变附加标志 | |

---

### 1.4 三大主分支：NULL / DISABLED / NORMAL

```
if (lVar1 == 0) {
    // ═══ 分支 1: 手写笔帧为 NULL ═══
    ASA_Reset();             // 全管线重置
    LOG("STYLUS FRAME NULL!!");
}
else if (*(int*)(lVar1 + 0x1C) == 0) {
    // ═══ 分支 2: status == 0，手写笔被禁用 ═══
    LOG("DISABLE STYLUS!");
    ASA_Reset();
}
else {
    // ═══ 分支 3: 正常处理流程（核心管线入口）═══
    // ... 详见下文
}
```

**分支 1 触发条件**：AFE 硬件未返回手写笔数据（可能是固件未使能手写笔扫描）。
**分支 2 触发条件**：手写笔帧存在但 status 全零，表示当前帧明确禁用了手写笔功能。

两者都执行完整的 `ASA_Reset()`，它会重置所有子模块：

```c
void ASA_Reset(void) {
    ButtonInit();           // 按钮状态机重置
    HPP3_FreqShiftInit();   // 频率跳变引擎重置
    PressureInit();         // 压力管线重置
    RawInit();              // 原始数据缓冲区清零
    PeaksInit();            // 峰值检测器重置
    GridPeaksInit();        // 网格峰值重置
    CoordinateInit();       // 坐标计算器重置
    ReportInit();           // 报告输出重置
    TiltInit();             // 倾斜角计算重置
    FilterInit();           // 所有滤波器状态清零
    CoorReviseInit();       // 坐标修正引擎重置
    NoPressInkInit();       // 无压感墨迹检测重置
    ASAStaticReset();       // 静态状态机重置
    ASACalibration_Init();  // 校准引擎重置
    AnimationInit();        // 动画状态机重置
}
```

---

### 1.5 正常分支内：PreInRange 保存与主处理

```c
// 在特定特性标志下，保存进入主处理前的 InRange 状态
if (((*(g_tsaStaticPtr + 0xFC) & 0x1000000) == 0) &&   // 非校准模式
     ((*(g_tsaStaticPtr + 0xFC) & 0x80000) != 0)) {     // 笔后触摸抑制使能
    g_preInRange = TSA_RptASAInRange();
}

// ★ 核心管线入口 ★
uVar5 = ASA_MainProcess(piVar6);
g_isAsaCurFrameExitAbnormal = (uint)uVar5;
```

`ASA_MainProcess` 的返回值含义：

| 返回值 | 含义 |
|--------|------|
| 0 | 正常处理完成 |
| 1 | ValidJudgment 失败，已重置 |
| 3 | 帧被旁路（频率跳变中/数据异常） |
| 5 | 噪声帧/无有效数据 |

---

### 1.6 StylusRecheck 与触摸抑制逻辑

这是 `TSA_ASAProcess` 中最复杂的条件链：

```c
if (( (*(g_tsaStaticPtr+0x108) & 0x100) == 0 )    // 条件A: StylusRecheck 未使能
    || ( (*(lVar1+0x1C) & 2) == 0 )                // 条件B: 非 HPP3 协议
    || ( StylusRecheck_DisableRecheckInFreqShifting() != 0 )  // 条件C: 频率跳变期间禁用
    || ( StylusRecheck_EnterStylusMode() != 0 ))    // 条件D: 验证通过，确认进入笔模式
{
    // ═══ 进入此分支：正常的触摸抑制判断 ═══
    cVar2 = ASA_IsTouchNull(lVar1);
    if (cVar2 != 0) {
        g_asaDisableTouch = 3;   // 抑制触摸 3 帧
    }
}
else {
    // ═══ StylusRecheck 验证失败：笔信号实际是触摸 ═══
    ASA_Reset();
    g_asaDisableTouch = 0;       // 不抑制触摸
}
```

**逻辑解读**：
- 条件 A-D 是短路求值的 OR 链。只要任一条件为 true，就进入"正常笔模式"分支。
- 当 A=false（Recheck使能）、B=false（HPP3协议）、C=false（非频率跳变）且 D=false（`StylusRecheck_EnterStylusMode()` 返回 0，即验证不通过）时，认为笔信号是虚假的，执行 `ASA_Reset()` 并释放触摸抑制。

**`StylusRecheck_EnterStylusMode()` 的核心验证逻辑**：
1. 检查状态 bit4 是否置位（`status & 0x10`），否则直接返回 0
2. 检查 Recheck 功能是否使能
3. 检查 ASA 报告的 InRange 是否为 true
4. 检查 TX1/TX2 信号强度是否超过阈值
5. Windows Pad 特殊模式：扫描全部传感器数据，找最大峰值，< 600 则退出笔模式
6. 检查笔位置是否与触摸位置重叠（`StylusRecheck_CheckTouchOverlapped`）

---

### 1.7 触摸抑制计数器与笔进入/退出

```c
if (g_asaDisableTouch != 0) {
    g_asaDisableTouch--;     // 递减抑制帧计数

    if (/* 笔后触摸抑制模式 */) {
        cVar2 = TSA_RptASAInRange();

        // ─── 笔刚进入 Withdrawn 区域（InRange 从 0→1）───
        if (cVar2 == 1 && g_preInRange == 0) {
            LOG("Pen Enter Withdrawn");
            // 设置所有触摸点的 bit7 = "Withdrawn" 标志
            for (i = 0; i < *g_touchesPtr; i++) {
                *(g_touchesPtr + i * 0x168 + 0x208) |= 0x80;
            }
        }

        // ─── 笔持续在 InRange（InRange 1→1）───
        if (cVar2 == 1 && g_preInRange == 1) {
            PrevTouch_Init();    // 清空上帧触摸
            Touch_Init();        // 清空当前触摸
            PrevPeak_Init();     // 清空上帧峰值
        }
    }
    else {
        // HPP3 Touch Enable 未使能时直接清除触摸
        cVar2 = ASA_IsHpp3TouchEnableFeatureEnabled();
        if (cVar2 == 0) {
            PrevTouch_Init();
            Touch_Init();
            PrevPeak_Init();
        }
    }
    SideTouch_Clean();  // 清除侧边触摸
}
```

**触摸点结构体偏移**：
- `g_touchesPtr + i * 0x168 + 0x208`：每个触摸点大小 0x168 字节，偏移 0x208 处是状态标志字，bit7 (`0x80`) 表示"Withdrawn"（笔悬停抑制）。

---

### 1.8 尾部处理

```c
TSALog_OutputStylus();           // 输出调试日志

// 触发录制标志
if (ASA_NeedRecordTrigger()) {   // g_asaStatic == 1（落笔瞬间）
    *(g_tsaStaticPtr + 0xA4) |= 0x1000;  // 设置录制触发位
}

HardwareAnalyzer_ASA_Process();  // 硬件分析器（诊断）

// ═══ 外部强制重置 ═══
if ((param_1 & 0xF) != 0) {
    ASA_Reset();
}

// ═══ 频率跳变请求检测 ═══
if (TSA_IsASANeedTPFreqShift() != 0) {
    LOG("ASA WANT TP FREQ SHIFT!!");
}
if (TSA_IsASANeedBTFreqShift() != 0) {
    LOG("ASA WANT BT FREQ SHIFT!!");
}

// 清除 "ASA 正在执行" 标志 (bit6)
*(g_tsaStaticPtr + 0x10C) &= ~0x40;
```

---


## 第二章：核心管线 ASA_MainProcess

### 2.1 时间戳与帧上下文构建

```c
undefined8 ASA_MainProcess(int *param_1)
{
    // param_1 = AFE_GetFrame() 返回的帧指针

    // 构建毫秒级时间戳：秒 * 1000 + 微秒 / 1000
    DAT_18231a20 = (longlong)*param_1 * 1000 + (longlong)(param_1[1] / 1000);

    // 保存帧序号（uint16 截断）
    DAT_18231a14 = (undefined2)param_1[2];

    // 提取手写笔子帧指针（偏移 +0x40）
    lVar1 = *(longlong *)(param_1 + 0x10);

    // 保存全局帧指针，供后续子函数使用
    g_frame = param_1;
```

> **关键变量映射**：
> - `DAT_18231a20`：当前帧的绝对毫秒时间戳，用于频率跳变的时间窗口判断
> - `DAT_18231a14`：帧序号，用于直线滤波器的帧同步
> - `g_frame`：全局帧指针，在 `LinearFilterProcess` 等子函数中通过 `g_frame + 8` 读取帧序号

---

### 2.2 硬件版本与参数单元日志

```c
    if (g_tsaLogLoglevel != 0) {
        StrPrintf_Process(0, "HW VER IS %d, USING PRMT %d",
                          g_curStylusHWVersion, g_curPrmtUnit);
    }
```

`g_curStylusHWVersion` 来自蓝牙上报的笔硬件版本号，`g_curPrmtUnit` 是当前使用的参数单元索引。不同硬件版本使用不同的参数表（如 IIR 系数、压力曲线等）。

---

### 2.3 频率跳变预处理

```c
    // 保存当前帧 status 到全局
    DAT_18231958 = *(uint *)(lVar1 + 0x1C);

    // 仅当 HPP3 协议 (bit1=1) 时执行频率跳变
    if ((*(uint *)(lVar1 + 0x1C) & 2) != 0) {
        HPP3_FreqShiftProcess(lVar1);
    }
```

频率跳变在 `ValidJudgment` 之前执行，因为即使帧数据无效，也需要维护频率状态机。详细分析见第九章。

---

### 2.4 帧有效性判断 ValidJudgment

```c
    cVar2 = ValidJudgment(lVar1);
    if (cVar2 == 0) {
        // ═══ 分支 A: 无效帧 ═══
        cVar2 = ReleaseASAReportInFreqShifting();
        if (cVar2 == 0) {
            // 非频率跳变期间的无效帧 → 重置并返回 1
            ASA_Reset();
            uVar4 = 1;
        } else {
            // 频率跳变期间 → 保持上帧输出，返回 3（旁路）
            // ReleaseASAReportInFreqShifting() 内部:
            //   memcpy(&g_curASOut, &g_prevASOut, 0xEC);
            uVar4 = 3;
        }
    }
```

**`ReleaseASAReportInFreqShifting()` 的作用**：在频率跳变过程中，传感器数据不可靠，但仍需向上层报告笔位置以避免"笔消失"。此函数将上一帧的完整输出（0xEC 字节）复制到当前帧输出缓冲区，保持笔位置冻结。

---

### 2.5 释放抑制分支（上帧释放、当前帧无释放）

```c
    else if (((DAT_1823195c & 0x10) == 0) || ((DAT_18231958 & 0x10) != 0)) {
        // ═══ 分支 B: 正常数据处理路径 ═══
        // 条件：上帧 status 没有 bit4，或当前帧 status 有 bit4
        // 即：只要不是"上帧有释放标记但当前帧没有"的异常状态
        // ... 进入完整数据处理管线（见下文 2.6）
    }
    else {
        // ═══ 分支 C: 释放过渡态 ═══
        // 条件：上帧 bit4=1（释放中），当前帧 bit4=0（释放完成）
        // 即：笔从 in-range 过渡到 out-of-range 的最后一帧
        cVar2 = ReleaseASAReportExitStylus();
        if (cVar2 != 0) {
            DAT_1823195c = DAT_18231958;  // 更新上帧 status
        }
        g_isLastFrameBypass = 1;
        LOG("ASA RELEASE!");
        uVar4 = 3;   // 旁路
    }
```

> **物理含义**：`bit4 (0x10)` 是"释放阶段"标志。当笔从触摸面板移开时，固件会持续发送几帧带 `bit4=1` 的数据，然后切换到 `bit4=0`。这个分支处理的是释放的最后边沿——上一帧还在释放中，当前帧已经完成释放。`ReleaseASAReportExitStylus()` 负责生成平滑的离开轨迹。

---

### 2.6 正常数据处理管线（分支 B 内部）

这是 `ASA_MainProcess` 最核心的代码路径：

```c
    // ── 阶段 1: 静态状态预处理 ──
    ASAStaticStatusPreProcess(*(undefined4 *)(lVar1 + 0x1C));
    DAT_1823195c = DAT_18231958;   // 保存当前 status 为"上帧 status"

    // ── 阶段 2: 检查当前帧是否有有效数据 ──
    if ((*(uint *)(lVar1 + 0x1C) & 4) == 0) {
        // bit2 = 0：有传感器线数据（非纯触摸帧）

        // ── 阶段 3: 属性和静态预处理 ──
        ASAPropertyPreProcess();    // 清零 g_asaPrpt (0x7AC 字节)
        ASAStaticPreProcess();
        ASAOutClean();              // 清零当前输出缓冲区

        // ── 阶段 4: 协议分发 ──
        if ((DAT_18231958 & 2) == 0) {
            // ═══ HPP2 协议 ═══
            if ((DAT_18231958 & 1) != 0) {
                HPP2_UpdataStylus2Buf(lVar1);  // 更新原始数据到缓冲区
                iVar3 = HPP2_DataProcess();     // HPP2 数据处理
                if (iVar3 != 0) {
                    g_isLastFrameBypass = 1;
                    return 3;  // 处理失败，旁路
                }
            }
        }
        else {
            // ═══ HPP3 协议（主流路径）═══
            HPP3_UpdataStylus2Buf(lVar1);  // 更新原始数据到缓冲区
            iVar3 = HPP3_DataProcess();     // HPP3 数据处理（详见第四章）
            if (iVar3 != 0) {
                ReleaseASAReportInFreqShifting();
                g_isLastFrameBypass = 1;
                return 3;
            }
        }

        // ── 阶段 5: 坐标后处理管线 ──
        ASA_CoorPostProcess();    // 滤波→修正→IIR→抖动抑制→屏幕映射

        // ── 阶段 6: StylusRecheck 标志 ──
        DAT_18231a2d = StylusRecheck_EnterStylusMode();

        // ── 阶段 7: 动画处理 ──
        AnimationProcess();

        // ── 阶段 8: 频率跳变中的释放处理 ──
        ReleaseASAReportInFreqShifting();

        // ── 阶段 9: 保存当前帧为上帧 ──
        memcpy(&g_prevASOut, &g_curASOut, 0xEC);
        g_isLastFrameBypass = 0;

        // ── 阶段 10: 校准处理 ──
        ASACalibration_Process();

        uVar4 = 0;  // 正常完成
    }
    else {
        // bit2 = 1：纯 status 帧，无传感器数据
        ReleaseASAReportInFreqShifting();
        uVar4 = 3;
    }
```

---

### 2.7 HPP2 与 HPP3 数据类型路由

`HPP3_DataProcess()` 内部根据 `g_flagDataType` 分发到不同的数据处理函数：

```c
undefined8 HPP3_DataProcess(void)
{
    switch (g_flagDataType) {
        case 0:  // Line Data（线性传感器数据）
            iVar1 = ASA_HPP3TX1LineDataProcesss();
            break;
        case 1:  // IQ Line Data（IQ 调制线数据）
            iVar1 = ASA_HPP3TX1IQLineDataProcesss();
            break;
        case 2:  // Grid Data（网格传感器数据）
            iVar1 = ASA_HPP3TX1GridDataProcesss();
            break;
        case 3:  // Tied Grid Data（绑定网格数据）
            iVar1 = ASA_HPP3TX1TiedGridDataProcesss();
            break;
        default: // 默认回退到线性数据
            iVar1 = ASA_HPP3TX1LineDataProcesss();
    }

    if (iVar1 != 0) return 3;  // 数据处理失败

    ASA_HPP3Process();  // HPP3 后处理（压力/墨迹/边缘校正）
    return 0;
}
```

**数据类型说明**：

| `g_flagDataType` | 名称 | 含义 | 对应 HPP2 |
|---|---|---|---|
| 0 | Line | 一维线性传感器数据（TX/RX 分离） | `ASA_HPP2LineDataProcesss` |
| 1 | IQ Line | 带 IQ 调制的线性数据（含相位信息） | `ASA_HPP2IQLineDataProcesss` |
| 2 | Grid | 二维网格数据（交叉点扫描） | `ASA_HPP2GridDataProcesss` |
| 3 | Tied Grid | 绑定网格（多通道复用） | `ASA_HPP2TiedGridDataProcesss` |

> **Line vs Grid**：Line 模式下传感器输出 `g_gridRows + g_gridCols` 个一维信号强度值（每个电极一个），坐标需要通过峰值检测 + 插值算出。Grid 模式下传感器输出 `g_gridRows × g_gridCols` 的二维矩阵，坐标通过重心法直接计算。

---

### 2.8 输出缓冲区 g_curASOut / g_prevASOut

每帧的完整输出保存在 `g_curASOut` 结构体中（大小 0xEC = 236 字节），处理完成后通过 `memcpy` 复制到 `g_prevASOut` 供下帧参考。

这两个缓冲区在频率跳变时被用于"冻结"输出（`ReleaseASAReportInFreqShifting` 将 `prev` 拷贝回 `cur`），在释放边沿时也被用于生成平滑退出轨迹。

---

### 2.9 ASA_MainProcess 完整流程图

```
ASA_MainProcess(frame)
│
├─ 构建时间戳 DAT_18231a20
├─ 提取手写笔子帧 lVar1
├─ 保存全局 status → DAT_18231958
│
├─ [HPP3] HPP3_FreqShiftProcess(lVar1)
│
├─ ValidJudgment(lVar1)
│   ├─ [返回0] → 无效帧
│   │   ├─ [频率跳变中] → 冻结输出, return 3
│   │   └─ [否则] → ASA_Reset(), return 1
│   │
│   └─ [返回1] → 有效帧
│       ├─ [释放过渡态] → ReleaseASAReportExitStylus, return 3
│       │
│       └─ [正常处理]
│           ├─ ASAStaticStatusPreProcess
│           ├─ ASAPropertyPreProcess (清零)
│           ├─ ASAStaticPreProcess
│           ├─ ASAOutClean
│           │
│           ├─ [HPP2] HPP2_UpdataStylus2Buf → HPP2_DataProcess
│           ├─ [HPP3] HPP3_UpdataStylus2Buf → HPP3_DataProcess
│           │   ├─ [Line]      ASA_HPP3TX1LineDataProcesss
│           │   ├─ [IQ Line]   ASA_HPP3TX1IQLineDataProcesss
│           │   ├─ [Grid]      ASA_HPP3TX1GridDataProcesss
│           │   ├─ [TiedGrid]  ASA_HPP3TX1TiedGridDataProcesss
│           │   └─ ASA_HPP3Process (压力/墨迹/边缘)
│           │
│           ├─ ASA_CoorPostProcess
│           │   ├─ LinearFilterProcess   (直线滤波)
│           │   ├─ GetRealTimeCoor2Buf   (环形缓冲)
│           │   ├─ Get3PointAvgFilter    (三点均值)
│           │   ├─ CoorReviseProcess     (TX2校正)
│           │   ├─ GetCoorSpeed          (速度计算)
│           │   ├─ GetIIRCoef            (自适应系数)
│           │   ├─ CoorFilterProcess     (IIR滤波)
│           │   ├─ AftCoorProcess        (抖动抑制)
│           │   └─ FitToLcdScreen        (屏幕映射)
│           │
│           ├─ AnimationProcess
│           ├─ ReleaseASAReportInFreqShifting
│           ├─ memcpy(prev ← cur, 0xEC)
│           ├─ ASACalibration_Process
│           └─ return 0
```

---

### 2.10 返回值总结表

| 返回值 | 触发条件 | 帧处理结果 |
|--------|----------|------------|
| **0** | 数据有效，管线正常完成 | g_curASOut 包含新数据 |
| **1** | ValidJudgment 失败 + 非频率跳变 | ASA_Reset() 已执行 |
| **3** | 频率跳变中 / 释放过渡态 / 数据处理失败 | 输出被冻结或旁路 |
| **5** | 噪声帧（`DAT_18231976 == 1`）或无有效信号 | g_curASOut = g_prevASOut |

其中返回值 5 的触发路径在 `ASA_HPP3TX1LineDataProcesss` 内部：

```c
// DAT_18231976 == 1 表示噪声抑制模块判定当前帧为纯噪声
if (DAT_18231976 == 1) {
    memcpy(&g_curASOut, &g_prevASOut, 0xEC);  // 保持上帧
    ASAPropertyPostProcess();
    return 5;
}
// DAT_18231950 == 0 && DAT_18231954 == 0 表示首次无信号
else if (DAT_18231950 == 0) {
    if (DAT_18231954 == 0) {
        ASAStaticPostProcess();
        ASAPropertyPostProcess();
        return 5;
    }
    // DAT_18231954 != 0 表示上帧有信号，当前帧丢失 → 释放
    else {
        ReleaseASAReportExitStylus();
        LOG("ASA RELEASE! out range");
        return 3;
    }
}
```

---

Edited new_stylus_analysis.md

## 第三章：ValidJudgment — 协议与模式校验（全7分支）

`ValidJudgment` 是管线的"门卫"函数，它检查手写笔子帧的 `status` 位域（偏移 +0x1C）是否处于合法的协议和模式组合。返回 1 表示帧有效，返回 0 表示帧无效。

### 3.1 总体判定树

```
status bits:  [bit0=HPP2] [bit1=HPP3] [bit2=mode] [bit3=mode] [bit4=mode]

ValidJudgment(stylusFrame)
│
├─ bit0=1 AND bit1=1 ?
│   └─ YES → 分支①: "PROTOCOL SET ERROR! MUST SELECT ONE PROTOCOL" → return 0
│
├─ bit0=0 AND bit1=0 ?
│   └─ YES → 分支②: "PROTOCOL SET ERROR! MUST SET PROTOCOL" → return 0
│
│  （到此处：协议有效，恰好设置了 HPP2 或 HPP3 之一）
│
├─ bit2=0 AND bit3=0 AND bit4=0 ?
│   └─ YES → 分支③: "MODE ERROR!" → return 0
│
├─ bit2=1 AND bit3=1 AND bit4=1 ?
│   └─ YES → 分支④: "MODE ERROR!" → return 0
│
│  （到此处：协议和模式都合法）
│
├─ TX1 线数据指针 == NULL AND bit2=0 ?
│   └─ YES → 分支⑤: "STYLUS FRAME TX1 LINE DATA NULL" → return 0
│
├─ [仅日志] TX2 线数据指针 == NULL AND bit1=1(HPP3) AND bit2=0 ?
│   └─ YES → 分支⑥: LOG "STYLUS FRAME TX2 LINE DATA NULL" (不返回0!)
│
└─ 分支⑦: 所有检查通过 → return 1
```

---

### 3.2 分支①：双协议冲突

```c
if (((*(uint *)(param_1 + 0x1C) & 1) != 0) &&    // bit0 = 1 (HPP2)
    ((*(uint *)(param_1 + 0x1C) & 2) != 0)) {     // bit1 = 1 (HPP3)
    LOG("PROTOCOL SET ERROR! MUST SELECT ONE PROTOCOL");
    return 0;
}
```

**触发条件**：固件同时设置了 HPP2 和 HPP3 协议标志，这是不被允许的。
**影响**：`ASA_MainProcess` 收到返回值 0，如果不在频率跳变中则执行 `ASA_Reset()`。

---

### 3.3 分支②：无协议设置

```c
if (((*(uint *)(param_1 + 0x1C) & 1) == 0) &&    // bit0 = 0
    ((*(uint *)(param_1 + 0x1C) & 2) == 0)) {     // bit1 = 0
    LOG("PROTOCOL SET ERROR! MUST SET PROTOCOL");
    return 0;
}
```

**触发条件**：固件没有设置任何协议标志。通常发生在固件初始化未完成时。

---

### 3.4 分支③：无模式设置

```c
if (((*(uint *)(param_1 + 0x1C) & 4) == 0) &&    // bit2 = 0
    ((*(uint *)(param_1 + 0x1C) & 8) == 0) &&     // bit3 = 0
    ((*(uint *)(param_1 + 0x1C) & 0x10) == 0)) {  // bit4 = 0
    LOG("MODE ERROR!");
    return 0;
}
```

**触发条件**：协议选对了，但 bit2/3/4 全零——没有告知算法当前帧应使用哪种模式处理。

---

### 3.5 分支④：全模式设置（冲突）

```c
if (((*(uint *)(param_1 + 0x1C) & 4) != 0) &&    // bit2 = 1
    ((*(uint *)(param_1 + 0x1C) & 8) != 0) &&     // bit3 = 1
    ((*(uint *)(param_1 + 0x1C) & 0x10) != 0)) {  // bit4 = 1
    LOG("MODE ERROR!");
    return 0;
}
```

**触发条件**：bit2/3/4 全部置 1，同样是非法组合。模式位应当只能设置其中一个或两个的合法子集。

---

### 3.6 分支⑤：TX1 线数据为空

```c
if ((*param_1 == 0) &&                             // TX1 数据指针 == NULL
    ((*(uint *)(param_1 + 0x1C) & 4) == 0)) {      // bit2 = 0（非纯Grid模式）
    LOG("STYLUS FRAME TX1 LINE DATA NULL");
    return 0;
}
```

**触发条件**：在需要 TX1 线数据的模式下（bit2=0），TX1 数据指针为空。这通常意味着固件扫描失败或内存分配失败。

> **注意**：当 `bit2=1` 时（Grid 模式），TX1 线数据可以为空，因为 Grid 模式使用二维矩阵数据而非线数据。

---

### 3.7 分支⑥：TX2 线数据为空（仅日志，不拒绝）

```c
if (((*(uint *)(param_1 + 0x1C) & 2) != 0) &&     // HPP3 协议
    (param_1[1] == 0) &&                            // TX2 数据指针 == NULL
    ((*(uint *)(param_1 + 0x1C) & 4) == 0)) {      // 非 Grid 模式
    LOG("STYLUS FRAME TX2 LINE DATA NULL");
    // 注意：这里没有 return 0！
}
```

**关键差异**：TX2 为空只记录日志**但不拒绝帧**。这是因为 TX2 是用于倾斜角计算的第二频率通道，即使缺失也可以只用 TX1 计算坐标（此时倾斜角保持上一帧值）。

---

### 3.8 分支⑦：所有校验通过

```c
return 1;  // 帧有效，进入后续数据处理管线
```

---

### 3.9 全分支总结表

| 分支 | 条件 | bit0 | bit1 | bit2 | bit3 | bit4 | 数据 | 返回值 | 日志 |
|---|---|---|---|---|---|---|---|---|---|
| ① | 双协议 | 1 | 1 | × | × | × | × | 0 | MUST SELECT ONE |
| ② | 无协议 | 0 | 0 | × | × | × | × | 0 | MUST SET PROTOCOL |
| ③ | 无模式 | 0/1 | 0/1 | 0 | 0 | 0 | × | 0 | MODE ERROR |
| ④ | 全模式 | 0/1 | 0/1 | 1 | 1 | 1 | × | 0 | MODE ERROR |
| ⑤ | TX1 空 | 0/1 | 0/1 | 0 | 0/1 | 0/1 | TX1=NULL | 0 | TX1 NULL |
| ⑥ | TX2 空 | × | 1 | 0 | 0/1 | 0/1 | TX2=NULL | **1** | TX2 NULL (仅日志) |
| ⑦ | 通过 | 0/1 | 0/1 | 合法 | 合法 | 合法 | 有效 | 1 | — |

---

## 第四章：HPP3 数据处理管线 — 从传感器到坐标

### 4.1 ASA_HPP3TX1LineDataProcesss 概述

这是 HPP3 协议 Line 模式下最核心的数据处理函数，把原始传感器信号转化为坐标、倾斜角。它的内部流程分为两遍扫描：

```
第一遍扫描（基线刷新前）:
  HPP3_CMFProcess()           → 共模滤波
  HPP3_StylusDataQualityProcess()  → 数据质量评估
  TX1LinePeaksProcess()       → TX1 峰值检测
  [TX2有效] TX2LinePeaksProcess()  → TX2 峰值检测
  HPP3_NoiseProcess()         → 噪声检测
  复制当前数据到上帧缓冲区（0xA0 个 uint16）
  UpdateLineSignal()          → 信号强度更新
  HPP3_CoordinateProcess()    → 坐标计算（第一次）
  保存校准前坐标: preCalibPos = (DAT_18231130, DAT_18231134)
  [校准模式] ASA_RefreshLineTest()
  ASA_Refresh()               → 基线刷新

第二遍扫描（基线刷新后）:
  TX1LinePeaksProcess()       → TX1 峰值检测（基线已更新）
  [TX2有效] TX2LinePeaksProcess()
  HPP3_NoiseProcess()
  UpdateLineSignal()
  HPP3_NoisePostProcess()     → 噪声后处理
  ASAStaticStatusProcess()    → 静态状态判定（落笔/抬笔/悬停）
```

> **为什么要做两遍？** 第一遍用原始信号计算坐标用于校准参考点，然后 `ASA_Refresh()` 更新基线（自适应背景减除）。第二遍用更新后的基线重新检测峰值，得到更准确的信号强度，用于噪声判断和状态判定。

---

### 4.2 双遍扫描后的三路分支

```c
// ═══ 分支 A: 噪声帧 ═══
if (DAT_18231976 == 1) {
    // 噪声抑制模块判定当前帧为纯噪声
    memcpy(&g_curASOut, &g_prevASOut, 0xEC);  // 冻结输出
    ASAPropertyPostProcess();
    return 5;
}

// ═══ 分支 B: 无信号（DAT_18231950 == 0）═══
else if (DAT_18231950 == 0) {
    if (DAT_18231954 == 0) {
        // 上帧也无信号：首次进入无信号状态
        ASAStaticPostProcess();
        ASAPropertyPostProcess();
        return 5;
    }
    else {
        // 上帧有信号，当前帧丢失 → 笔离开
        ReleaseASAReportExitStylus();
        LOG("ASA RELEASE! out range");
        return 3;
    }
}

// ═══ 分支 C: 有效信号 ═══
else {
    HPP3_CoordinateProcess();     // 第二次坐标计算（用更新后的基线）
    g_aftCalibPosX = DAT_18231130;
    g_aftCalibPosY = DAT_18231134;

    if (g_flagTX2NotNull == 0) {
        // TX2 无效 → 尝试用 Grid TX1 计算倾斜角
        if (GridTx1Valid()) {
            TiltKeepLastFrame();  // 保持上帧倾斜角
        }
    }
    else {
        TiltProcess();            // TX1+TX2 双频计算倾斜角
    }
    return 0;
}
```

**关键变量**：
- `DAT_18231950`：当前帧是否有有效 InRange 信号（0=无，非0=有）
- `DAT_18231954`：上帧的 InRange 状态
- `DAT_18231976`：噪声帧标记（1=噪声）

---

### 4.3 HPP3 后处理 ASA_HPP3Process

在 `HPP3_DataProcess()` 的数据处理完成后，执行后处理管线：

```c
void ASA_HPP3Process(void) {
    HPP3_PressureProcess();           // 压力映射与IIR滤波
    NoPressInkProcess();              // 无压力墨迹检测
    HPP3_PostPressureProcess();       // 压力后处理
    EdgeCoorProcess();                // 边缘坐标高速修正
    EdgeCoorPostProcess();            // 边缘坐标后处理
    HPP3_ASAStaticStatusPostProcess();// 静态状态后处理
}
```

---

### 4.4 坐标计算 HPP3_CoordinateProcess 详解

这是从峰值数据到坐标的核心转换函数：

```c
void HPP3_CoordinateProcess(void) {
    // ── 步骤 1: 选择坐标算法 ──
    if ((DAT_1820d630 & 1) == 0) {
        // 三角插值法（默认，精度更高）
        g_coors[0] = GetCoordinateByTriangleOf(2);  // TX1 Dim1 坐标
        g_coors[1] = GetCoordinateByTriangleOf(3);  // TX1 Dim2 坐标
    } else {
        // 重心法（Grid 模式或特殊配置）
        g_coors[0] = GetCoordinateByGravityOf(2);
        g_coors[1] = GetCoordinateByGravityOf(3);
    }

    // ── 步骤 2: TX2 坐标（用于倾斜角）──
    if ((DAT_1820d630 & 4) != 0) {
        g_coors[6] = GetCoordinateByGravityOf(4);   // TX2 Dim1
        g_coors[7] = GetCoordinateByGravityOf(5);   // TX2 Dim2
    }

    // ── 步骤 3: 多阶曲线补偿（三次多项式非线性校正）──
    g_coors[6] = CoorMultiOrderFitCompensate(g_coors[6], &DAT_1820d638);
    g_coors[7] = CoorMultiOrderFitCompensate(g_coors[7], &DAT_1820d658);

    // ── 步骤 4: TP 图案补偿 ──
    CoorTpPatternCompensate();

    // ── 步骤 5: 信号刷新 ──
    if (*(char*)(g_asaPrmtFlash + 0xA84) == 0) {
        RefreshTx1Signal();       // 标准信号刷新
    } else {
        RefreshTx1SignalByPR();   // 基于 PR（Peak Ratio）的信号刷新
    }

    // ── 步骤 6: 传感器间距 → 内部坐标映射 ──
    // 将传感器 pitch 坐标映射到 0x400 为单位的内部坐标系
    DAT_18231a50 = SensorPitchSizeMapDim1(g_coors[0], 0x400);  // TX1 X
    DAT_18231a68 = SensorPitchSizeMapDim2(g_coors[1], 0x400);  // TX1 Y (存反了? 偏移 +0x24)
    DAT_18231a44 = DAT_18231a50;   // 原始坐标备份
    DAT_18231a74 = DAT_18231a68;   // 原始坐标备份

    DAT_18231a64 = SensorPitchSizeMapDim1(g_coors[6], 0x400);  // TX2 X
    DAT_18231a88 = SensorPitchSizeMapDim2(g_coors[7], 0x400);  // TX2 Y
    DAT_18231a60 = DAT_18231a64;
    DAT_18231a84 = DAT_18231a88;
}
```

**坐标系统**：
- 传感器原始坐标：以传感器 pitch 为单位，整数部分 = 电极索引，小数部分 = 电极间位置（乘以 `0x400` = 1024）
- 内部坐标：`电极索引 × 0x400 + 亚像素偏移`，例如坐标 `0x1800` = 第 6 根电极正中心

---

### 4.5 压力管线 HPP3_PressureProcess

```c
void HPP3_PressureProcess(void) {
    DAT_18231964 = 0;    // 清除"有压力"标记

    if (sRamffffffffc47f07fe == 0) {
        // 蓝牙原始压力值 == 0
        DAT_18231b18 = 0;    // 输出压力 = 0
        local_1a = 0;
    }
    else {
        // ── 压力映射 ──
        local_1a = GetPressInMapOrder();           // 获取映射后的中间值
        DAT_18231b18 = HPP3_GetPressureMapping(local_1a);  // 查压力曲线表

        // ── 压力 IIR 滤波（仅在连续按压时）──
        if (DAT_18231b18 != 0 && DAT_18231c18 != 0) {
            PressureIIR(0x40);   // IIR 系数 = 64/256 = 0.25（强平滑）
        }
    }

    g_btPressCnt++;    // 蓝牙压力帧计数

    // ── 信号强度抑制 ──
    HPP3_SuppressBtPressBySignal();    // 当传感器信号弱时抑制蓝牙压力

    if (DAT_18231b18 != 0) {
        DAT_18231964 = 1;    // 标记"当前帧有压力"
    }
}
```

**压力数据流**：
```
蓝牙原始压力 (sRamffffffffc47f07fe)
    → GetPressInMapOrder()         // 排序/选择映射
    → HPP3_GetPressureMapping()    // 查表：原始值 → 0~4095 标准化压力
    → PressureIIR(0x40)            // IIR 低通滤波（α=0.25）
    → HPP3_SuppressBtPressBySignal() // 信号弱时压制
    → DAT_18231b18                 // 最终输出压力
```

**关键变量**：
- `DAT_18231b18`：当前帧输出压力值
- `DAT_18231c18`：上帧输出压力值（用于 IIR 滤波和落笔/抬笔检测）
- `DAT_18231964`：InkPresent 标志（0=无墨迹，1=有墨迹）

---

### 4.6 边缘坐标高速修正 EdgeCoorProcess

当笔从触摸面板边缘离开时，由于传感器响应延迟，最后报告的位置可能偏离实际离开位置。此函数检测这种情况并修正：

```c
void EdgeCoorProcess(void) {
    g_needCoor2EdgeHighSpeed = 0;

    // 计算当前帧与上帧的 X 坐标差
    uVar2 = abs(DAT_18231b44 - DAT_18231b50);

    // 触发条件（X 方向）：
    // 1. g_firstRelease != 0（允许释放修正）
    // 2. DAT_18230a84 == 1（Dim1 边缘标记）
    // 3. DAT_18231b18 == 0（当前无压力 → 抬笔）
    // 4. DAT_18231c18 != 0（上帧有压力 → 刚抬笔）
    // 5. uVar2 > 0x200（坐标突变 > 512 个内部单位）
    // 6. 上帧坐标在面板中间区域（排除已在边缘的情况）
    if (满足上述全部条件) {
        g_needCoor2EdgeHighSpeed = 1;   // 启用边缘高速修正
        g_firstRelease = 0;
    }

    // Y 方向同理...

    if (DAT_18231b18 != 0) {
        g_firstRelease = 1;   // 有压力时重新允许边缘修正
    }

    // 如果触发了边缘修正，用上帧压力替换当前压力
    // 防止抬笔瞬间坐标跳变
    if (bVar1) {
        DAT_18231b18 = DAT_18231c18;
    }
}
```

> **算法意图**：笔离开面板边缘时，传感器最后几帧的数据会急剧衰减，导致计算出的坐标向面板中心回弹。通过检测"坐标突变 + 抬笔 + 边缘附近"的组合条件，将坐标锁定在抬笔前的最后有效位置。阈值 `0x200` 约等于半个传感器间距。

---

Edited new_stylus_analysis.md

## 第五章：坐标计算算法

### 5.1 三角插值法 GetCoordinateByTriangleOf

这是默认的高精度坐标算法，基于三点抛物线拟合：

```c
ulonglong GetCoordinateByTriangleOf(uint param_1)
{
    // param_1: 维度索引 (2=TX1_Dim1, 3=TX1_Dim2)
    // lVar4: 对应维度的线数据数组指针
    // bVar1: 数据长度（通道数）
    // bVar2: 峰值所在索引

    lVar4 = g_lineDataPtr[param_1];     // 线数据数组
    bVar1 = g_lineDataLen[param_1];     // 数据长度
    bVar2 = g_peakIdx[param_1];         // 峰值索引（信号最大的电极）

    if (bVar2 >= bVar1) {
        return 0x7FFFFFFF;   // 无峰值 → 返回无效坐标
    }

    // ═══ 三种情况 ═══

    if (bVar2 == bVar1 - 1) {
        // ── 情况 A: 峰值在最后一个电极（右边缘）──
        // 使用 peak, peak-1, peak-2 三点，反向拟合
        iVar5 = TriangleAlgEdge(
            data[bVar2],      // 峰值
            data[bVar2-1],    // 左邻
            data[bVar2-2],    // 左左邻
            edgeParam1,       // 边缘校正参数
            edgeParam2
        );
        return bVar1 * 0x400 - iVar5;  // 从右端反算坐标
    }
    else if (bVar2 == 0) {
        // ── 情况 B: 峰值在第一个电极（左边缘）──
        // 使用 peak, peak+1, peak+2 三点
        return TriangleAlgEdge(
            data[0],          // 峰值
            data[1],          // 右邻
            data[2],          // 右右邻
            edgeParam1,
            edgeParam3
        );
    }
    else {
        // ── 情况 C: 峰值在中间（常规路径）──
        // 使用 peak-1, peak, peak+1 三点抛物线插值
        iVar5 = TriangleAlgUsing3Point(
            data[bVar2-1],    // 左邻
            data[bVar2],      // 峰值
            data[bVar2+1]     // 右邻
        );
        return iVar5 + bVar2 * 0x400;
    }
}
```

**算法模型 — 三点抛物线插值**：

`TriangleAlgUsing3Point(L, C, R)` 实现的数学公式为：

```
         (L - R) × 0x200
offset = ─────────────────────
         2C - L - R
```

这是经典的**抛物线峰值插值（Parabolic Peak Interpolation）**：假设传感器信号在峰值附近近似于二次抛物线 `y = ax² + bx + c`，利用三个等间距采样点 `(L, C, R)` 拟合出抛物线顶点的亚电极位置。

- 分子 `(L - R)` 反映峰值的不对称性
- 分母 `(2C - L - R)` 反映峰值的"尖锐度"
- 结果乘以 `0x200 = 512`（半个 pitch），得到以 1/1024 pitch 为单位的亚像素偏移

**边缘处理**：`TriangleAlgEdge` 处理峰值在面板边缘的情况，此时只能取单侧两个邻居，使用修正后的边缘校正参数 `edgeParam` 来补偿边缘效应导致的系统偏差。

---

### 5.2 重心法 GetCoordinateByGravityOf

用于 Grid 模式或作为备选算法：

```c
int GetCoordinateByGravityOf(uint param_1)
{
    // 准备重心计算数据
    local_20 = g_lineDataPtr[param_1];
    local_21 = g_lineDataLen[param_1];

    // 清零临时缓冲区（20 × 8 字节）
    memset(local_d8, 0, sizeof(local_d8));

    // 获取虚拟边缘值（用于面板边缘补偿）
    local_32 = GetFictiousEdge(param_1, peakIdx);

    // 根据 TX1/TX2 选择不同的数据更新策略
    if (param_1 == 4 || param_1 == 5) {
        // TX2: 使用 TX2 专用重心数据
        local_e8 = UpdateTX2GravityData(local_20, local_d8, local_21, noiseFloor);
    } else {
        // TX1: 使用 TX1 重心数据，带虚拟边缘
        local_e8 = UpdateTX1GravityData(local_20, local_d8, local_21, noiseFloor, local_32);
    }

    // 计算加权重心
    iVar1 = Gravity(local_d8, (byte)local_e8);

    // 返回坐标 = 峰值索引 × pitch + 重心偏移
    return local_e8._1_1_ * 0x400 + iVar1;
}
```

**算法模型 — 加权质心法（Centroid / Center of Gravity）**：

```
         Σ(i × w[i])
coor = ──────────────
           Σ(w[i])
```

其中 `w[i]` 是每个传感器通道的信号强度（减去噪声底之后）。`GetFictiousEdge` 在面板边缘处生成虚拟的"镜像"传感器数据，防止边缘处的重心因信号截断而向中心偏移。

重心法的优点是对噪声更鲁棒，缺点是在信号分布不对称时精度不如三点插值。

---

### 5.3 多阶曲线补偿 CoorMultiOrderFitCompensate

用于补偿传感器非线性误差的三次多项式校正：

```c
int CoorMultiOrderFitCompensate(int param_1, double *param_2)
{
    // param_1: 输入坐标
    // param_2: 4 个双精度系数 [c0, c1, c2, c3]

    // 计算在当前 pitch 内的局部偏移
    int remainder = param_1 % 0x400;       // pitch 内偏移 (0~1023)

    // 折叠到半 pitch（利用对称性）
    int local_c;
    if (remainder < 0x201) {               // 0x201 = 513
        local_c = 0x200 - remainder;       // 左半段：镜像
    } else {
        local_c = remainder - 0x200;       // 右半段：直接
    }

    // 三次多项式计算补偿量
    //   compensation = c0 + c1*x + c2*x² + c3*x³
    int compensation = (int)(
        param_2[0] +                        // c0: 常数偏移
        param_2[1] * (double)local_c +      // c1: 线性项
        param_2[2] * (double)local_c * (double)local_c +  // c2: 二次项
        param_2[3] * (double)local_c * (double)local_c * (double)local_c  // c3: 三次项
    );

    // 根据折叠方向确定补偿符号
    if (remainder >= 0x201) {
        compensation = -compensation;       // 右半段取反
    }

    return param_1 + compensation;
}
```

**算法模型 — 分段三次多项式非线性校正**：

传感器电极以固定 pitch 排列，但由于电场边缘效应，在每个 pitch 周期内的响应并非完美线性。这个函数对每个 pitch 周期内的坐标施加一个**对称的三次多项式补偿**：

```
补偿曲线（以 pitch 中心为原点，左右对称）：

    ↑ 补偿量
    │    ╱╲
    │   ╱  ╲        c3*x³ + c2*x² + c1*x + c0
    │  ╱    ╲
    ├─╱──────╲─── x (pitch 内偏移)
    │ 0   512  1024
```

`param_2` 指向的 4 个 `double` 系数 `[c0, c1, c2, c3]` 存储在 Flash 参数区 `DAT_1820d638`（X 方向）和 `DAT_1820d658`（Y 方向），在工厂校准时确定。

---

### 5.4 坐标各阶段变量对应表

根据 `ASA_MainProcess` 中的日志字符串和数据流追踪：

```
"TEST X,%d, %d, Prsse:%d, linear:%d,%d, coor_revise:%d,%d, aft_flt:%d,%d, aft_jit:%d,%d"
```

| 日志标签 | X 变量 | Y 变量 | 含义 |
|---|---|---|---|
| `(无标签，前两个)` | `DAT_18231a44` | `DAT_18231a68` | 原始映射坐标 |
| `Prsse` | `DAT_18231b18` | — | 压力值 |
| `linear` | `DAT_18231a48` | `DAT_18231a6c` | 直线滤波后 |
| `coor_revise` | `DAT_18231a4c` | `DAT_18231a70` | 坐标修正后 |
| `aft_flt` | `DAT_18231a50` | `DAT_18231a74` | IIR 滤波后 |
| `aft_jit` | `DAT_18231a54` | `DAT_18231a78` | 抖动抑制后（最终） |

完整数据流：

```
HPP3_CoordinateProcess
  → DAT_18231a44 / DAT_18231a68   (raw mapped)
       │
  LinearFilterProcess
  → DAT_18231a48 / DAT_18231a6c   (after linear filter)
       │
  CoorReviseProcess
  → DAT_18231a4c / DAT_18231a70   (after coor revise)
       │
  CoorFilterProcess (IIR)
  → DAT_18231a50 / DAT_18231a74   (after IIR filter)
       │
  AftCoorProcess (jitter)
  → DAT_18231a54 / DAT_18231a78   (after jitter suppression)
       │
  FitToLcdScreen (GetClipReport)
  → 最终 LCD 像素坐标
```

---

## 第六章：坐标后处理滤波管线 ASA_CoorPostProcess

```c
void ASA_CoorPostProcess(void) {
    LinearFilterProcess();      // ① 直线滤波器
    GetRealTimeCoor2Buf();      // ② 坐标环形缓冲
    Get3PointAvgFilter();       // ③ 三点均值滤波
    CoorReviseProcess();        // ④ TX2 坐标修正
    GetCoorSpeed();             // ⑤ 速度计算
    GetIIRCoef();               // ⑥ 自适应 IIR 系数
    CoorFilterProcess();        // ⑦ IIR 坐标滤波
    AftCoorProcess();           // ⑧ 抖动抑制
    FitToLcdScreen();           // ⑨ 屏幕映射
    DAT_18231b28 = DAT_18231950;
    ASAStaticPostProcess();
    ASAPropertyPostProcess();
}
```

---

### 6.1 直线滤波器 LinearFilterProcess — 7态状态机

目的：检测笔是否在画直线，如果是则约束坐标到直线轨迹上，大幅减少直线绘制时的手抖。

```c
void LinearFilterProcess(void) {
    // 保存滤波前坐标
    DAT_18231a48 = DAT_18231a44;   // X_linear = X_raw
    DAT_18231a6c = DAT_18231a68;   // Y_linear = Y_raw

    if (DAT_18231b18 == 0 || !ASA_IsHpp3LinearFilterFeatureEnabled()) {
        // 无压力或功能未使能 → 重置状态机
        g_asaStraightLineBufCnt = 0;
        g_asaShortDisBufCnt = 0;
        DAT_182319a0 = 0;   // 状态归零
        return;
    }

    // 超过 20 帧缓冲后，开始更新直线拟合参数
    if (g_asaStraightLineBufCnt > 0x13) {
        UpdateStraightLinePrmt(&DAT_182319e8, 1, 400 - g_asaStraightLineBufCnt);
        UpdateStraightLinePrmt(&DAT_182319c0, 0, 400 - g_asaStraightLineBufCnt);
    }

    // ═══ 7态状态机 ═══
    switch (DAT_182319a0) {
        case 0: DAT_182319a0 = 1; break;          // 初始化 → 等待
        case 1: DAT_182319a0 = 2; break;          // 等待 → 收集
        case 2: DAT_182319a0 = 3; break;          // 收集 → 曲线判定
        case 3: CurveLineProcess(); break;         // 曲线模式处理
        case 4: EnterStraightLineProcess(); break; // 进入直线模式
        case 5: StraightLineProcess(); break;      // 直线模式处理
        case 6: ExitStraightLineProcess(); break;  // 退出直线模式
        default: DAT_182319a0 = 3; break;          // 异常恢复
    }

    // 缓存坐标和帧序号
    BufStraightPaintPoint(DAT_18231a44, DAT_18231a68, frameSeq);
    BufShortDistancePoint(DAT_18231a44, DAT_18231a68, frameSeq);

    // 边界裁剪
    DAT_18231a48 = clamp(DAT_18231a48, 0, g_gridCols * 0x400);
    DAT_18231a6c = clamp(DAT_18231a6c, 0, g_gridRows * 0x400);
}
```

**状态转换图**：

```
  ┌─────── 0:Init ──→ 1:Wait ──→ 2:Collect ──→ 3:CurveLine ─────┐
  │                                                    │           │
  │                                              偏差小/趋势稳    │
  │                                                    ↓           │
  │                                              4:EnterStraight   │
  │                                                    │           │
  │                                                    ↓           │
  │                                              5:StraightLine ──┤
  │                                                    │      偏差大│
  │                                                    ↓           │
  │                                              6:ExitStraight ──┘
  │                                                    │
  └──────── 抬笔(pressure=0) ─────────────────────────┘
```

**`UpdateStraightLinePrmt` 的含义**：对缓冲区中最近 N 帧的坐标进行**最小二乘线性拟合**，计算出直线参数 `A`（斜率）和 `B`（截距），以及拟合残差 `dis`、最大偏差 `maxdis`。日志中的 `"LineFit: fY,A,B,dis,maxdis"` 对应这些拟合参数。

---

### 6.2 坐标环形缓冲 GetRealTimeCoor2Buf

```c
void GetRealTimeCoor2Buf(void) {
    // 将历史坐标向后移动（24帧环形缓冲）
    for (i = 0x17; i > 0; i--) {
        xBuf[i] = xBuf[i-1];    // DAT_1820d8c8 数组
        yBuf[i] = yBuf[i-1];    // DAT_1820d928 数组
    }
    // 存入当前帧坐标
    xBuf[0] = g_coors[0];   // DAT_1820d8c8 = TX1_Dim1 原始坐标
    yBuf[0] = g_coors[1];   // DAT_1820d928 = TX1_Dim2 原始坐标

    // 落笔瞬间（当前有压力，上帧无压力）：保存起笔坐标
    if (DAT_18231b18 != 0 && DAT_18231c18 == 0) {
        g_coorBuf = g_coors[0];
        DAT_1820d8c4 = g_coors[1];
    }
}
```

> 24帧深度的环形缓冲区供后续的速度计算、三点均值滤波使用。`0x7FFFFFFF` 标记无效帧。

---

### 6.3 三点均值滤波 Get3PointAvgFilter

```c
void Get3PointAvgFilter(void) {
    // 移位 3 点均值缓冲区
    for (i = 0x17; i > 0; i--) {
        avgXBuf[i] = avgXBuf[i-1];
        avgYBuf[i] = avgYBuf[i-1];
    }

    if (xBuf[2] == 0x7FFFFFFF) {
        // 不足 3 帧有效数据 → 不滤波，直接传递
        avgXBuf[0] = xBuf[0];
        avgYBuf[0] = yBuf[0];
    } else {
        // ── 三点移动平均 ──
        avgXBuf[0] = (xBuf[0] + xBuf[1] + xBuf[2]) / 3;
        avgYBuf[0] = (yBuf[0] + yBuf[1] + yBuf[2]) / 3;
    }

    // ── 一阶差分（加速度估计）──
    // 公式: 3*x[0] - 3*x[1] + x[2] (等价于二阶差分的一阶近似)
    g_coors[2] = xBuf[0]*3 - xBuf[1]*3 + xBuf[2];   // X 差分
    g_coors[3] = yBuf[0]*3 - yBuf[1]*3 + yBuf[2];    // Y 差分（存储方式有偏移）
}
```

**数学分析**：

`g_coors[2]` 实际上是 `3(x[0] - x[1]) + (x[2] - x[1])` 的计算误差形式，真正意图是计算**数值二阶导数**（加速度），但代码中的表达式与标准二阶差分 `x[0] - 2*x[1] + x[2]` 不同。结合系数 `3, -3, 1`，这更像是一个**预测滤波器**：基于最近三帧趋势预测下一帧位置，用于后续的抖动判定。

---

### 6.4 坐标修正 CoorReviseProcess

```c
void CoorReviseProcess(void) {
    // 初始化：直接传递
    DAT_18231a4c = DAT_18231a48;    // X_revise = X_linear
    DAT_18231a70 = DAT_18231a6c;    // Y_revise = Y_linear

    if (!ASA_IsHpp3CoorReviseFeatureEnabled()) return;

    // 抬笔时重置修正引擎
    if (DAT_18231c18 != 0 && DAT_18231b18 == 0) {
        LOG("Stylus up! CoorReviseInit!");
        CoorReviseInit();
    }

    if (g_flagTX2Start == 0) {
        DAT_18231965 = 0;    // TX2 未启动
    } else {
        // TX2 可用时执行双频修正
        if (g_flagTX2NotNull != 0) {
            CoorReviseCalculation();  // 计算 TX1-TX2 坐标差异
        }
        CoorReviseWork();             // 应用修正
    }
}
```

**原理**：TX1 和 TX2 使用不同频率驱动笔，理论上应该得到相同的坐标。两者的差异来源于传感器非理想性（如串扰、EMI）。`CoorReviseCalculation` 计算 TX1/TX2 坐标差，`CoorReviseWork` 利用这个差值修正 TX1 坐标，提升精度。

---

### 6.5 速度计算 GetCoorSpeed

```c
void GetCoorSpeed(void) {
    local_14 = 0;  // 累积路程

    for (i = 1; i < 0x18; i++) {     // 遍历 24 帧缓冲
        if (xBuf[i] == 0x7FFFFFFF) break;  // 无效帧停止

        // ── 逐帧欧氏距离 ──
        dx = xBuf[i-1] - xBuf[i];
        dy = yBuf[i-1] - yBuf[i];
        segDist = sqrt((dx*dx + dy*dy) * 100);  // ×100 提高精度
        local_14 += segDist;                     // 累积路程

        // ── 总位移（起点到当前帧）──
        totalDx = xBuf[0] - xBuf[i];
        totalDy = yBuf[0] - yBuf[i];
        totalDist = sqrt((totalDx*totalDx + totalDy*totalDy) * 100);

        // 存储
        accumPath[i] = local_14 / 10;    // 累积路程（÷10 归一化）
        totalPath[i] = totalDist / 10;   // 总位移
        linearity[i] = (local_14 * 100) / totalDist;  // 曲率指标
    }

    // 计算最终速度指标
    if (local_9 != 0) {
        DAT_1820dc38 = accumPath[1];              // 瞬时速度（最近1帧）
        DAT_1820dc40 = accumPath[local_9] / local_9;  // 平均速度
        DAT_1820dc3c = (local_9 > 3) ? accumPath[3]/3 : DAT_1820dc40;  // 3帧短期速度
    }
}
```

**速度指标用途**：
- `DAT_1820dc38`（瞬时速度）：用于 `GetIIRCoef` 自适应 IIR 系数
- `DAT_1820dc3c`（短期速度）：3帧窗口的平均速度
- `linearity`（曲率）：`累积路程/总位移`，值越接近 100 越是直线

---

### 6.6 IIR 系数自适应 GetIIRCoef

根据笔移动速度动态调整 IIR 平滑系数：

```c
void GetIIRCoef(void) {
    // 根据 InRange 状态选择基础系数
    if ((DAT_18231950 & 6) == 0) {
        // 无 InRange → 使用强平滑系数（悬停态）
        highCoef = g_asaPrmtFlash[0xA5F];  // 高速系数
        lowCoef  = g_asaPrmtFlash[0xA5E];  // 低速系数
        speedThreshold = 20;
    } else {
        // 有 InRange → 使用弱平滑系数（书写态）
        highCoef = g_asaPrmtFlash[0xA5D];
        lowCoef  = g_asaPrmtFlash[0xA5C];
        speedThreshold = 10;
    }

    // 边缘区域强制减半（减少滤波延迟）
    if (DAT_18230a84 != 0 || DAT_18230c34 != 0) {
        highCoef = g_asaPrmtFlash[0xA5F] >> 1;
        lowCoef  = g_asaPrmtFlash[0xA5E] >> 1;
    }

    // ═══ 速度自适应插值 ═══
    if (speed >= 0xCD) {           // 高速 (≥205)
        coef = highCoef;           // 使用高速系数（弱平滑→低延迟）
    }
    else if (speed < speedThreshold) {
        coef = lowCoef;            // 低速：强平滑→抑制抖动
    }
    else {
        // 中间速度：线性插值
        coef = lowCoef + (highCoef - lowCoef) * (speed - threshold) / (0xCC - threshold);
    }

    g_coorIIRCoef = coef;
    LOG("IIR: Speed[%d,%d,%d] Coef[%d]", speed, speed3, speedAvg, coef);
}
```

**算法模型 — 速度自适应滤波**：

```
IIR系数
    ↑
 high│─────────────────────────────╱
    │                          ╱
    │                       ╱    线性插值区
    │                    ╱
 low │──────────────╱
    ├────────────┼───────────────┼──→ 速度
    0         threshold(10/20)  0xCC(204)
```

- **低速**（<10/20）：强平滑（`lowCoef` 大），抑制手部自然抖动
- **高速**（>204）：弱平滑（`highCoef` 小），保证跟踪实时性
- **中速**：线性插值，平滑过渡

---

### 6.7 IIR 坐标滤波核心 CoorIIRFilter

```c
uint CoorIIRFilter(int prevValue, int curValue, ushort coef) {
    // IIR 一阶低通滤波：
    //   output = (coef × curValue + (N - coef) × prevValue) / N
    // 其中 N = g_asaPrmtFlash[0xA60]（分母，通常为 256）

    byte N = *(byte*)(g_asaPrmtFlash + 0xA60);

    return (coef * curValue + (N - coef) * prevValue) / N;
}
```

**数学公式**：

```
y[n] = α × x[n] + (1 - α) × y[n-1]

其中 α = coef / N
```

这是标准的**一阶 IIR 低通滤波器（Exponential Moving Average）**。

- `coef` = `g_coorIIRCoef`（由 `GetIIRCoef` 速度自适应计算）
- `N` = 参数 Flash 中的 `0xA60` 偏移处字节值（分母常量，通常为 256）
- 当 `coef = N` 时，`α = 1`，无滤波（直接输出当前值）
- 当 `coef → 0` 时，`α → 0`，极强平滑（几乎不响应新输入）

**精度处理**：输入值左移 8 位（`<< 8`），滤波后右移 8 位（`>> 8`），余数部分（`& 0xFF`）保存在 `g_curASOut` 中作为下帧的进位，实现**定点小数 IIR 滤波**，避免浮点运算。

---

### 6.8 抖动抑制 AftCoorProcess — 起笔死区锁定

```c
void AftCoorProcess(void) {
    if ((DAT_18231a28 & 0x20) != 0) {
        // 功能标志禁用 → 直接传递
        DAT_18231a54 = DAT_18231a50;
        DAT_18231a78 = DAT_18231a74;
        return;
    }

    // ── 根据区域选择阈值 ──
    if (在边缘区域) {
        threshX = (prmt[0xA58] * gridCols * 0x400) / totalPitchX;  // 边缘阈值
        threshY = (prmt[0xA59] * gridRows * 0x400) / totalPitchY;
    } else {
        threshX = (prmt[0xA5A] * gridCols * 0x400) / totalPitchX;  // 中心阈值
        threshY = (prmt[0xA5B] * gridRows * 0x400) / totalPitchY;
    }

    // ── 落笔瞬间：记录起笔位置 ──
    if (DAT_18231b18 != 0 && DAT_18231c18 == 0) {
        startX = DAT_18231a50;
        startY = DAT_18231a74;
        flagLockX = 1;
        flagLockY = 1;
    }

    // ── X 轴死区锁定 ──
    if (flagLockX) {
        if (abs(DAT_18231a50 - startX) > threshX) {
            flagLockX = 0;   // 超出死区 → 解锁
        }
        if (flagLockX) {
            coorOffsetX = DAT_18231a50 - startX;  // 记录偏差
        }
    }

    // ── Y 轴同理 ──
    // ...

    // ── 应用偏移补偿 ──
    DAT_18231a54 = clamp(DAT_18231a50 - coorOffsetX, 0, gridCols * 0x400);
    DAT_18231a78 = clamp(DAT_18231a74 - coorOffsetY, 0, gridRows * 0x400);

    LOG("LOCK: flag[%d,%d] thold[%d,%d] offset[%d,%d]",
        flagLockX, flagLockY, threshX, threshY, coorOffsetX, coorOffsetY);
}
```

**算法原理 — 起笔死区（Landing Dead Zone）**：

笔刚落下时，由于手指肌肉的不稳定性和传感器的初始化延迟，最初几帧的坐标通常会有明显跳动。此算法：

1. **记录落笔点** `(startX, startY)`
2. **在死区内锁定**：只要坐标偏离落笔点不超过阈值 (`threshX/Y`)，就把偏移量记录为 `coorOffset`
3. **输出补偿**：实际输出 = 当前坐标 - 累积偏移，相当于将微小的抖动"吸收"掉
4. **解锁**：一旦坐标真正移动超过阈值（用户开始书写），解锁并停止补偿

这是一种**自适应起笔着陆平滑算法**，在 Apple Pencil、Wacom 等专业手写笔中广泛使用。

---

### 6.9 屏幕映射 FitToLcdScreen / GetClipReport

```c
void FitToLcdScreen(void) {
    for (i = 0; i < 9; i++) {
        // X 方向映射
        xOut[i] = GetClipReport(xIn[i], g_gridCols, totalPitchX,
                                xOffset, xEnd, xStart);
        // Y 方向映射
        yOut[i] = GetClipReport(yIn[i], g_gridRows, totalPitchY,
                                yOffset, yEnd, yStart);
    }
    UpdateRptDis();  // 更新距离报告
}

int GetClipReport(int coor, ushort gridSize, ushort totalPitch,
                  short offset, short end, ushort start)
{
    // 线性映射: sensorCoor → screenPixel
    //   screen = (coor × (totalPitch + end + offset)) / gridSize + 0x200
    int mapped = (int)(((offset + totalPitch + end) * coor) / gridSize);

    // 四舍五入到整数像素 (÷1024)
    int pixel = (mapped + 0x200) >> 10;

    // 减去起始偏移
    pixel -= offset;

    // 裁剪到有效范围
    pixel = clamp(pixel, 0, totalPitch - 1);

    // 加上起始像素
    return start + pixel;
}
```

**映射公式**：

```
LCD_pixel = start + clamp(
    round((coor × (totalPitch + margin)) / gridSize) - offset,
    0, totalPitch - 1
)
```

参数 `xStart/xEnd/xOffset` 等存储在 Flash 参数区 `DAT_1820d604 ~ DAT_1820d60E`，定义了传感器坐标到 LCD 像素的线性映射关系和边距裁剪区域。循环 9 次是因为管线内维护了 9 组坐标（TX1 各阶段 + TX2 各阶段），全部需要映射到屏幕空间。

---

Edited new_stylus_analysis.md

## 第七章：倾斜角算法 TiltProcess

### 7.1 整体架构与有效性检查

`TiltProcess` 利用 TX1 和 TX2 两个不同频率通道计算出的坐标差异来推导笔的倾斜角。由于笔的物理结构，笔尖在不同倾斜角度下对两个频率的电磁响应位置不同，形成一个可测量的 "坐标差向量"。

```c
void TiltProcess(void) {
    if (g_flagHPP3Protocol == 0) return;   // 仅 HPP3 支持

    // ── 步骤 0: 初始化检查 ──
    if (((DAT_18231c28 & 2) == 0 && (DAT_18231c28 & 4) == 0) || DAT_18231c18 == 0) {
        TiltInit();   // 无有效倾斜数据或上帧无压力 → 重置
    }

    // ── 步骤 1: 数据有效性校验（按数据类型分四路）──

    // Grid 模式 (flagDataType == 2)
    if (g_flagDataType == 2) {
        if (!GridTx1Valid() || !GridTx2Valid()) {
            TiltKeepLastFrame();   // TX1 或 TX2 峰值无效 → 保持上帧
            return;
        }
    }
    // Line 模式 (flagDataType == 0)
    else if (g_flagDataType == 0) {
        if (!LineTx1Valid() || !LineTx2Valid()) {
            TiltKeepLastFrame();
            return;
        }
    }
    // IQ Line 模式 (flagDataType == 1)
    else if (g_flagDataType == 1) {
        if (!LineTx1Valid() || !LineTx2Valid()) {
            TiltKeepLastFrame();
            return;
        }
    }
    // Tied Grid 模式 (flagDataType == 3)
    else if (g_flagDataType == 3) {
        if (!LineTx1Valid() || !LineTx2Valid()) {
            TiltKeepLastFrame();
            return;
        }
    }

    // ═══ 所有校验通过，执行倾斜角计算 ═══
    g_flagTX2Start = 1;
```

> **设计说明**：四种数据类型使用了几乎相同的检查逻辑（Grid 用 `GridTx1Valid`/`GridTx2Valid`，其余用 `LineTx1Valid`/`LineTx2Valid`），但编译器没有合并。这是 switch-case 展开后的结果。

---

### 7.2 信号比与坐标差计算

```c
    // ── 步骤 2: TX1/TX2 信号强度比 ──
    uVar2 = GetTX1TX2SignalRatio();
    BufTX1TX2SignalRatio(uVar2);              // 缓存到环形缓冲
    g_signalRatio = GetTX1TX2RatioAverage(3); // 3帧平均信号比

    // ── 步骤 3: 坐标差限幅阈值 ──
    sVar3 = GetTX1TX2LenLimit();   // 返回最大允许的坐标差长度

    // ── 步骤 4: 原始坐标差 ──
    // local_a = TX2_X - TX1_X（Dim1 方向坐标差）
    local_a = (short)DAT_18231148 - (short)DAT_18231130;
    // local_c = TX2_Y - TX1_Y（Dim2 方向坐标差）
    local_c = (short)DAT_1823114c - (short)DAT_18231134;

    LOG("d[%d,%d]", local_a, local_c);
```

**物理含义**：
- `DAT_18231130 / DAT_18231134`：TX1 计算出的坐标（X, Y）
- `DAT_18231148 / DAT_1823114c`：TX2 计算出的坐标（X, Y）
- 两者的差值 `(local_a, local_c)` 就是**双频坐标差向量**，其方向反映笔的倾斜方向，大小反映倾斜角度。

---

### 7.3 坐标差稳定性滤波

```c
    // ── 步骤 5: 基于历史缓冲的坐标差稳定替代 ──
    // 计算另一组参考坐标差（基于峰值缓冲区的历史数据）
    sVar4 = (short)peakBufDim1[peakIdx] - (short)DAT_18231130;
    sVar5 = (short)peakBufDim2[peakIdx] - (short)DAT_18231134;

    // 如果峰值索引发生变化且参考坐标差超限，使用 IIR 平滑
    if ((peakIdxChanged) &&
        (abs(sVar4) > sVar3 || abs(sVar5) > sVar3) &&
        (g_coorDifBufCnt != 0))
    {
        if (abs(local_a) > sVar3 || abs(local_c) > sVar3) {
            // 当前帧坐标差也超限 → 完全使用历史值
            local_a = g_coordifdim1Buf;
            local_c = g_coordifdim2Buf;
        }
        else {
            // 当前帧未超限 → IIR 混合 (7/8 历史 + 1/8 当前)
            local_a = (short)((local_a + g_coordifdim1Buf * 7) >> 3);
            local_c = (short)((local_c + g_coordifdim2Buf * 7) >> 3);
        }
    }

    // ── 步骤 6: 首帧限幅 ──
    if (abs(local_a) > sVar3 || abs(local_c) > sVar3) {
        if (g_coorDifBufCnt == 0) {
            // 首帧且超限 → 裁剪到最大值
            local_a = clamp(local_a, -sVar3, sVar3);
            local_c = clamp(local_c, -sVar3, sVar3);
        } else {
            // 非首帧且超限 → 使用历史值
            local_a = g_coordifdim1Buf;
            local_c = g_coordifdim2Buf;
        }
    }
```

> **IIR 混合权重 7/8**：`(x + buf*7) >> 3` 等价于 `(1/8)*x + (7/8)*buf`，这是一个 α=0.125 的强平滑 IIR 滤波，用于抑制坐标差的帧间跳动。

---

### 7.4 坐标差到倾斜角的转换

```c
    // ── 步骤 7: 缓冲并取 5 帧移动平均 ──
    BufTX1TX2CoorDif(local_a, local_c);
    local_a = GetTX1TX2CoorDifAverage(5, 0);   // Dim1 坐标差 5帧均值
    local_c = GetTX1TX2CoorDifAverage(5, 1);   // Dim2 坐标差 5帧均值

    g_coordifdim1Buf = local_a;
    g_coordifdim2Buf = local_c;

    // ── 步骤 8: 坐标差 → 倾斜角查表 ──
    DAT_18231b10 = GetTiltByCoorDif(local_a, 0);   // X 倾斜角（°×10）
    DAT_18231b12 = GetTiltByCoorDif(local_c, 1);   // Y 倾斜角

    // ── 步骤 9: 向量长度限幅 ──
    local_e = Misc_SqrtUint32(local_c*local_c + local_a*local_a);
    if (local_e == 0) local_e = 1;

    // 如果向量长度超过限幅阈值，等比缩放
    if (sVar3 < local_e) {
        local_a = (short)((sVar3 * local_a) / local_e);
        local_c = (short)((sVar3 * local_c) / local_e);
    }

    // 用限幅后的坐标差重新查表
    DAT_18231b10 = GetTiltByCoorDif(local_a, 0);
    DAT_18231b12 = GetTiltByCoorDif(local_c, 1);
```

**`GetTiltByCoorDif` 的含义**：这是一个查表函数，将坐标差值映射到倾斜角度。由于笔结构和传感器的非线性关系，坐标差到角度并非简单比例关系，需要通过工厂校准后的查找表来转换。

**向量限幅**：坐标差向量 `(local_a, local_c)` 的欧氏长度不能超过 `sVar3`。超过时按比例缩放：`new = old × (limit / |old|)`，保持方向不变但限制最大角度。

---

### 7.5 倾斜角输出滤波

```c
    // ── 步骤 10: 缓冲倾斜角 ──
    BufDim1Dim2Tilt(DAT_18231b10, DAT_18231b12);

    // ── 步骤 11: 平滑输出 ──
    if (DAT_18231c18 == 0 || DAT_18231c14 == 0) {
        // 首帧落笔或首帧有效 → 直接输出
        DAT_18231b14 = DAT_18231b10;
        DAT_18231b16 = DAT_18231b12;
    } else {
        // 连续帧 → 5帧移动平均
        DAT_18231b14 = GetTiltAverage(5, 0);
        DAT_18231b16 = GetTiltAverage(5, 1);
    }

    // ── 步骤 12: 1度抖动抑制 ──
    DAT_18231b14 = Tilt1DegreeJitFilter(DAT_18231c14, DAT_18231b14);
    DAT_18231b16 = Tilt1DegreeJitFilter(DAT_18231c16, DAT_18231b16);

    // 保存为下帧参考
    g_tiltdim1Buf = DAT_18231b14;
    g_tiltdim2Buf = DAT_18231b16;

    LOG("tOut[%d,%d]", DAT_18231b14, DAT_18231b16);
```

**`Tilt1DegreeJitFilter` 的原理**：如果当前帧与上帧的倾斜角差异恰好为 1 个单位（即 ±0.1°，因为内部以 0.1° 为单位），则保持上帧值不变。这防止了倾斜角在两个相邻量化级之间不断跳动。

**倾斜角变量映射**：

| 变量 | 含义 |
|------|------|
| `DAT_18231b10` | 当前帧 X 倾斜角（滤波前） |
| `DAT_18231b12` | 当前帧 Y 倾斜角（滤波前） |
| `DAT_18231b14` | 输出 X 倾斜角（滤波后）→ `TSA_RptASAXTilt()` |
| `DAT_18231b16` | 输出 Y 倾斜角（滤波后）→ `TSA_RptASAYTilt()` |
| `DAT_18231c14` | 上帧 X 倾斜角 |
| `DAT_18231c16` | 上帧 Y 倾斜角 |

---

## 第八章：压力管线

### 8.1 压力数据流完整路径

```
蓝牙原始 4 字节压力数据
  ├─ uRamffffffffc47f07f8  (byte 0: 原始值 A)
  ├─ uRamffffffffc47f07fa  (byte 1: 原始值 B)
  ├─ uRamffffffffc47f07fc  (byte 2: 原始值 C)
  └─ sRamffffffffc47f07fe  (byte 3: 最终选择值)
           │
           ▼
    GetPressInMapOrder()       ─── 从 4 字节中选择有效值并排序
           │
           ▼
    HPP3_GetPressureMapping()  ─── 查表映射到 0~4095 标准化压力
           │
           ▼
    PressureIIR(0x40)          ─── IIR 低通 (α=0.25)，仅连续按压
           │
           ▼
    HPP3_SuppressBtPressBySignal()  ─── 信号弱时压制压力
           │
           ▼
    DAT_18231b18               ─── 输出压力 → TSA_RptASAPressure()
```

### 8.2 压力 IIR 滤波 PressureIIR

```c
// PressureIIR(0x40) 的等效公式：
// output = (0x40 * current + (0x100 - 0x40) * previous) / 0x100
//        = (64 * current + 192 * previous) / 256
//        = 0.25 * current + 0.75 * previous
```

α = 64/256 = 0.25 意味着：每帧只接受 25% 的新压力值，75% 保持上帧。这是一个**截止频率很低的一阶 IIR 低通滤波器**，能有效平滑蓝牙传输中的压力抖动，但会引入约 3-4 帧的延迟。

### 8.3 信号弱压力抑制

`HPP3_SuppressBtPressBySignal()` 检测传感器信号强度。当 TX1/TX2 接收到的信号强度低于阈值时（笔可能离面板太远或传感器异常），即使蓝牙报告有压力也会被压制为 0。这防止了"空中写字"的幽灵笔迹。

### 8.4 无压感墨迹 NoPressInkProcess

```c
void NoPressInkProcess(void) {
    if (!ASA_IsHpp3NoPressInkFeatureEnabled()) return;

    // 学习模式检查
    if (!ASA_IsHpp3NoPressTLearnedFeatureEnabled() || g_noPressPara != 0) {
        NoPressInkHandle();   // 直接处理
    } else {
        DAT_18231965 = 0;     // 学习未完成，禁用无压感墨迹
    }

    // 学习引擎
    if (ASA_IsHpp3NoPressTLearnedFeatureEnabled()) {
        if (g_noPressPara == 0) {
            NoPressInkLearningPrepareProcess();  // 准备学习
        } else {
            NoPressInkLearningProcess();         // 执行学习
        }
    }
}
```

**无压感墨迹（No-Press Ink）**的应用场景：某些手写笔没有压力传感器（或蓝牙未连接），但仍需要提供"书写"体验。此模块通过分析 TX1/TX2 的信号强度和笔与面板的距离来**推断**笔是否在接触面板，生成虚拟的 "InkPresent" 标志 (`DAT_18231965`)。

学习引擎 (`NoPressInkLearningProcess`) 通过在线自适应调整阈值，适应不同笔和面板的信号特性。

---

## 第九章：频率跳变引擎 HPP3_FreqShiftProcess

### 9.1 频率管理架构

HPP3 协议使用两个发射频率 TX1/TX2 来驱动手写笔。当检测到干扰（如 LCD 刷新噪声、其他笔的频率冲突）时，需要动态切换频率。

```c
void HPP3_FreqShiftProcess(undefined8 param_1) {
    HPP3_BtStatusProcess();     // 更新蓝牙连接状态
    HPP3_UpdateFreq(param_1);   // 从帧数据中提取当前频率

    // ── 频率合法性检查 ──
    if (DAT_1815e60a == 0 || DAT_1815e60a > 0xF1 ||
        DAT_1815e60b == 0 || DAT_1815e60b > 0xF1 ||
        (DAT_1815e604 == 1 &&
         (DAT_1815e60c == 0 || DAT_1815e60c > 0xF1 ||
          DAT_1815e60d == 0 || DAT_1815e60d > 0xF1)))
    {
        LOG("TP BT freq illegal!");
        return;   // 频率非法，不执行跳变
    }
```

**频率合法范围**：`1 ~ 0xF1 (241)`。0 和 >241 是非法值。

### 9.2 频率变化检测与倾斜参数清除

```c
    // 频率发生变化时，清除倾斜角参数（因为不同频率下倾斜校准不同）
    if (DAT_1815e60a != DAT_1815e608 || DAT_1815e60b != DAT_1815e609) {
        DAT_1815e608 = DAT_1815e60a;
        DAT_1815e609 = DAT_1815e60b;
        ClearTiltPrmt();    // 必须重新校准倾斜
    }
```

### 9.3 TP-BT 频率匹配检测

```c
    uVar2 = GetRealtime();   // 获取当前时间

    // 条件：蓝牙连接(DAT_1815e604==1) + TP/BT频率不匹配 + 超过30ms + 上次跳变冷却期过
    if (DAT_1815e604 == 1 &&
        !HPP3_IsTPandBTMatch() &&
        (DAT_1815e6e8 + 30) < uVar2 &&
        (g_freqShift == 0 || DAT_1815e6c8 + 1000 < uVar2))
    {
        // 触发 TP 频率跳变
        g_freqShift = 3;      // bit0=1(跳变中) + bit1=1(TP需要跳)
        DAT_1815e6c8 = uVar2; // 记录跳变开始时间
        LOG("TP BT freq NOT match!");
    }
```

### 9.4 BT 频率反向跳变

```c
    // 当蓝牙已连接 + TP/BT 匹配 + 之前有蓝牙跳变请求(bit2) + 蓝牙跳变未完成(!bit4)
    if (g_asaPrmtStylus[0x26E] != 0 &&
        DAT_1815e604 == 1 &&
        HPP3_IsTPandBTMatch() &&
        (g_freqShift & 4) != 0 && (g_freqShift & 0x10) == 0 &&
        DAT_1815e6d8 + 1000 < uVar2)
    {
        g_freqShift = 1;      // 重新触发仅 TP 跳变
        LOG("BT freq NOT switch!");
    }
```

### 9.5 自主频率搜索

```c
    // TP 侧主动搜索新频率（不依赖蓝牙）
    if (g_asaHpp3FreqShiftEnable != 0 &&
        (g_freqShift & 1) == 0 &&
        SearchForNewFreq())   // 检测到干扰需要换频
    {
        g_freqShift |= 1;    // 标记跳变中
        DAT_1815e6c8 = uVar2;
    }

    // ── 跳变完成检测 ──
    if ((g_freqShift & 1) != 0 && CheckIsFreqShiftDone()) {
        g_freqShift = 0;   // 跳变完成，清除所有标志
        // 记录全链路时间戳用于诊断
        LOG("Start:%lld, tpStart:%lld, btStart:%lld, tpDone:%lld, btDone:%lld, done:%lld", ...);
    }
```

### 9.6 g_freqShift 位域解析

| Bit | 掩码 | 含义 |
|-----|------|------|
| bit0 | `0x01` | 频率跳变正在进行中 |
| bit1 | `0x02` | TP 频率需要跳变 |
| bit2 | `0x04` | BT（蓝牙）频率需要跳变 |
| bit4 | `0x10` | BT 跳变已完成确认 |

### 9.7 频率跳变期间的输出控制

```c
    // 构建当前帧的频率状态标志
    DAT_18231960 = 0;
    if (DAT_18231958 & 0x20) DAT_18231960 |= 0x20;   // TP 跳变完成
    if ((DAT_18231958 & 0x40) && g_freqShift == 0)
        DAT_18231960 |= 0x40;                          // BT 跳变完成（仅非跳变中）
    if (DAT_18231958 & 0x80) DAT_18231960 |= 0x80;    // 附加标志
```

`DAT_18231960` 被 `NeedASAReportInFreqShifting()` 读取。当频率跳变进行中时，坐标输出被冻结为上一帧值（`memcpy(cur ← prev)`），防止跳变瞬间产生的噪声坐标被传递给上层。

---

## 第十章：动画状态机 AnimationProcess

### 10.1 热区方向判定

`AnimationProcess` 实现一个基于笔在面板角落区域的手势识别状态机，用于触发系统级动画（如唤醒、截屏等）。

```c
void AnimationProcess(void) {
    // ── 落笔瞬间记录起始位置 ──
    if (DAT_18231b18 != 0 && DAT_18231c18 == 0) {
        startX = DAT_18231a50;
        startY = DAT_18231a74;
    }

    // ── 根据屏幕方向选择坐标变换和热区 ──
    switch (*(int*)(g_tsaStaticPtr + 0x264)) {
        case 0:  // 0°旋转（默认）
            diffX = startX - curX;      // 右上角热区
            diffY = curY - startY;
            hotArea = {右上角};
            break;
        case 1:  // 90°旋转
            diffX = curX - startX;      // 左上角热区
            diffY = curY - startY;
            hotArea = {左上角};
            break;
        case 2:  // 180°旋转
            diffX = curX - startX;      // 左下角热区
            diffY = startY - curY;
            hotArea = {左下角};
            break;
        case 3:  // 270°旋转
            diffX = startX - curX;      // 右下角热区
            diffY = startY - curY;
            hotArea = {右下角};
            break;
    }
```

**热区定义**：每个方向的热区大小为 `0x800` (2048) 个内部坐标单位 ≈ 2 个传感器间距。只有笔从热区内开始移动时才会触发动画状态转换。

### 10.2 状态转换表

```c
    if (isTriggerHotArea()) {
        int length = AnimalLength();   // 计算从起点的位移长度

        if (DAT_18231b18 == 0) {
            // ═══ 抬笔态 ═══
            if (length < 2000)      → state = 0x00  // 短滑 → 无动画
            else if (length < 4000) → state = 0x04  // 中滑 → 过渡（仅从非0态进入）
            else                    → state = 0x10/0x00 // 长滑 → 完成/重置
        }
        else {
            // ═══ 落笔态 ═══
            if (length < 2000)      → state = 0x01  // 短 → 开始
            else if (length < 4000) → state = 0x02  // 中 → 进行中
            else                    → state = 0x08  // 长 → 触发
        }
    }
    g_asaAnimalPreState = g_asaAnimalState;
```

| 状态值 | 含义 | 上层读取接口 |
|--------|------|-------------|
| `0x00` | 无动画 / 重置 | `TSA_GetASAAnimationState()` |
| `0x01` | 笔落下，短距离 | |
| `0x02` | 笔落下，中距离 | |
| `0x04` | 抬笔过渡 | |
| `0x08` | 长距离触发 | |
| `0x10` | 动画完成 | |

---

## 第十一章：关键结构体与全局变量总表

### 11.1 手写笔子帧结构体（lVar1 指向）

```
偏移    大小    名称                    物理含义
────────────────────────────────────────────────────
+0x00   8 byte  pTx1LineData           TX1 一维线数据指针
+0x08   8 byte  pTx2LineData           TX2 一维线数据指针
+0x10   2 byte  tx1Freq                TX1 发射频率码
+0x12   2 byte  tx2Freq                TX2 发射频率码
+0x14   2 byte  rawPressure            蓝牙原始压力值
+0x18   4 byte  buttonStatus           按钮状态位域
+0x1C   4 byte  status                 帧状态位域（核心路由字段）
```

### 11.2 坐标管线各阶段地址映射

```
地址            变量名(推测)        坐标阶段            方向
──────────────────────────────────────────────────────────
DAT_18231a44    rawMappedX          原始映射坐标         X (Dim1)
DAT_18231a68    rawMappedY          原始映射坐标         Y (Dim2)
DAT_18231a48    linearFilteredX     直线滤波后           X
DAT_18231a6c    linearFilteredY     直线滤波后           Y
DAT_18231a4c    coorRevisedX        坐标修正后           X
DAT_18231a70    coorRevisedY        坐标修正后           Y
DAT_18231a50    iirFilteredX        IIR 滤波后           X
DAT_18231a74    iirFilteredY        IIR 滤波后           Y
DAT_18231a54    jitterSuppressedX   抖动抑制后(最终)     X
DAT_18231a78    jitterSuppressedY   抖动抑制后(最终)     Y
DAT_18231a60    tx2RevisedX         TX2 修正后           X
DAT_18231a84    tx2RevisedY         TX2 修正后           Y
DAT_18231a64    tx2RawMappedX       TX2 原始映射         X
DAT_18231a88    tx2RawMappedY       TX2 原始映射         Y
```

### 11.3 参数 Flash 偏移表（g_asaPrmtFlash）

| 偏移 | 类型 | 用途 |
|------|------|------|
| `+0xA58` | byte | 边缘区域 X 抖动阈值 |
| `+0xA59` | byte | 边缘区域 Y 抖动阈值 |
| `+0xA5A` | byte | 中心区域 X 抖动阈值 |
| `+0xA5B` | byte | 中心区域 Y 抖动阈值 |
| `+0xA5C` | byte | 书写态低速 IIR 系数 |
| `+0xA5D` | byte | 书写态高速 IIR 系数 |
| `+0xA5E` | byte | 悬停态低速 IIR 系数 |
| `+0xA5F` | byte | 悬停态高速 IIR 系数 |
| `+0xA60` | byte | IIR 分母常量 N（通常 256） |
| `+0xA84` | byte | 信号刷新模式（0=标准, 1=PR） |

---

## 第十二章：完整数据流图

### 12.1 Raw Sensor → Screen 全链路

```
AFE_GetFrame()
  │
  ├─ 主帧 (触摸)
  └─ 手写笔子帧 (偏移 +0x40)
       │
       ├─ TX1 Line Data [gridCols 个 uint16]
       ├─ TX2 Line Data [gridRows 个 uint16]
       ├─ tx1Freq, tx2Freq
       ├─ rawPressure
       └─ status (路由位域)
              │
    ┌─────────┴──────────────┐
    │                        │
    ▼                        ▼
 ValidJudgment()      HPP3_FreqShiftProcess()
    │                        │
    ▼                        │
 HPP3_DataProcess()          │
    │                        │
    ├─ HPP3_CMFProcess          (共模滤波)
    ├─ TX1/TX2 PeaksProcess     (峰值检测)
    ├─ HPP3_NoiseProcess        (噪声检测)
    ├─ ASA_Refresh              (基线更新)
    ├─ HPP3_CoordinateProcess   (坐标计算)
    │   ├─ GetCoordinateByTriangleOf  (三点抛物线)
    │   ├─ GetCoordinateByGravityOf   (加权重心)
    │   ├─ CoorMultiOrderFitCompensate (三次多项式)
    │   └─ SensorPitchSizeMap         (间距映射)
    ├─ TiltProcess              (倾斜角)
    │   ├─ TX1-TX2 坐标差
    │   ├─ IIR 平滑 + 向量限幅
    │   ├─ GetTiltByCoorDif (查表)
    │   └─ 5帧均值 + 1度抖动抑制
    │
    ▼
 ASA_HPP3Process
    ├─ HPP3_PressureProcess     (压力映射+IIR)
    ├─ NoPressInkProcess        (无压感墨迹)
    └─ EdgeCoorProcess          (边缘修正)
         │
         ▼
 ASA_CoorPostProcess
    ├─ LinearFilterProcess      (直线7态状态机)
    ├─ GetRealTimeCoor2Buf      (24帧环形缓冲)
    ├─ Get3PointAvgFilter       (三点均值)
    ├─ CoorReviseProcess        (TX2双频修正)
    ├─ GetCoorSpeed             (欧氏距离速度)
    ├─ GetIIRCoef               (速度自适应α)
    ├─ CoorFilterProcess        (一阶IIR低通)
    ├─ AftCoorProcess           (起笔死区锁定)
    └─ FitToLcdScreen           (屏幕像素映射)
         │
         ▼
 AnimationProcess               (角落手势状态机)
         │
         ▼
 TSA_RptASA*()                  (输出到上层)
    ├─ TSA_RptASAXPos()         → LCD X 坐标
    ├─ TSA_RptASAYPos()         → LCD Y 坐标
    ├─ TSA_RptASAPressure()     → 标准化压力 (0~4095)
    ├─ TSA_RptASAXTilt()        → X 倾斜角
    ├─ TSA_RptASAYTilt()        → Y 倾斜角
    ├─ TSA_RptASAInRange()      → 笔是否在检测范围
    ├─ TSA_RptASAInk()          → 墨迹标志
    └─ TSA_RptASAButtonStatus() → 按钮状态
```

### 12.2 滤波器算法模型总结

| 滤波器 | 算法类型 | 数学模型 | 用途 |
|--------|---------|---------|------|
| 三点抛物线插值 | 非递归 | `(L-R)×512 / (2C-L-R)` | 亚电极坐标精度 |
| 加权重心法 | 非递归 | `Σ(i×w)/Σ(w)` | Grid 模式坐标 |
| 三次多项式补偿 | 分段非线性 | `c0+c1x+c2x²+c3x³` | 传感器非线性校正 |
| 直线拟合滤波 | 最小二乘 | `y = Ax + B` | 直线绘制抖动抑制 |
| 三点移动平均 | FIR | `(x[0]+x[1]+x[2])/3` | 高频噪声平滑 |
| 速度自适应 IIR | 一阶 IIR | `α·x + (1-α)·y_prev` | 坐标/压力平滑 |
| 起笔死区锁定 | 非线性 | 偏移补偿 + 阈值解锁 | 落笔点抖动消除 |
| 1度抖动抑制 | 量化重保持 | `|Δ|≤1 ? prev : cur` | 倾斜角量化跳动 |
| 坐标差 7/8 IIR | 一阶 IIR | `x/8 + 7·buf/8` | 倾斜角坐标差平滑 |
| 5帧移动平均 | FIR | `Σx[0..4] / 5` | 倾斜角输出平滑 |
| 向量长度限幅 | 非线性 | `v×(limit/|v|)` | 倾斜角最大值限制 |

---

以上是文档全部 12 章的完整输出。整篇分析覆盖了 `TSA_ASAProcess` 及其调用的所有关键子函数，包括每个条件分支的触发条件、算法数学模型、结构体偏移和数据流追踪。