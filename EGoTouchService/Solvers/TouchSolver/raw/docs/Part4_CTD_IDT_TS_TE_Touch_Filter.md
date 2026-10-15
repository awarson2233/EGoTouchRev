# 第四部分：坐标计算、ID追踪、面积估算、事件机与触控滤波 (CTD / ER / IDT / TS / TE / Touch / Filter / TouchAction / TouchMode)

在 `TSA_Processing @ 0x6ba9a443` 主管线完成 `TZ_Process`（触控区域连通域聚类与特征提取）与 `ChargerNoise_Process` 后，算法进入针对每一个 `TouchUnit` 的**几何中心补偿、跨帧身份追踪、物理面积换算、生命周期状态机、多级抗抖动/平滑滤波、动作分类与多模态仲裁**阶段。本部分涵盖 20 个核心反编译模块（共 250+ 个子函数），以下结合 `W273AS2700`（`GaokunHimaxCSOT`，60×40 传感器阵列，2560×1600 屏幕分辨率）配置参数进行逐模块、逐函数的完整逆向解析。

---

## 4.0 核心运行时结构体逆向还原 (`Touch` 与 `PrevTouch`)

本阶段所有模块围绕两个核心全局结构体指针展开：当前帧触控表 `g_touchesPtr`（`@ 0x6bbc6a40`）与上一帧历史触控表 `g_prevTouchesPtr` / `g_pPrevTouches`（`@ 0x6bbc0fa0`）。两者均包含 `0x140` 字节的表头及最多 20 个触控单元数组。

### 4.0.1 当前帧触控表 `g_touchesPtr` (`TouchTable` + `TouchUnit[20]`)

- **表头 (`0x000 .. 0x13F`，共 `0x140` 字节)**：
  - `+0x00` (`u8 bTouchCount`)：当前帧有效 `TouchUnit` 数量（最大 `0x14` = 20）。
  - `+0x01 .. +0x13` (`u8[19]`)：全局统计状态字节（如 `+0x01` 已报点数、`+0x02` 新按下数、`+0x03` 抬起数、`+0x06` 大面积数、`+0x0b` 水滴异常计数、`+0x0d` 边缘触控数、`+0x0e` 负峰值邻域计数、`+0x0f` 跳频请求标志、`+0x12` 连续可疑帧计数、`+0x13` 传感器饱和计数）。
  - `+0x14` (`u16`)：帧间隔时间 `dt`。
  - `+0x18` (`u32 extFlags`)：外部状态位掩码（`bit 0` 充电器插入、`bit 1` 屏幕旋转方向、`bit 5` 指纹识别区激活等）。
  - `+0x1c` (`u32 newExtFlags`)：本帧新接收到的外部状态位掩码。
  - `+0x20` (`u32 exceptionFlags`)：全局异常掩码（水滴、噪声等）。
  - `+0x24` (`u32 stateFlags`)：全局工作流标志（`bit 0` 存在稳定触控、`bit 2` 存在高信噪比已报点、`bit 5` 刚发生合并、`bit 6` 刚发生分裂、`bit 8` 强滤波激活）。
  - `+0x28` (`u64 lastTouchDownFrame`)、`+0x30` (`u64 lastLiftOffFrame`)、`+0x38` (`u64 lastOuterEdgeFrame`)：关键事件帧号记录。
  - `+0x40 .. +0x12F` (`ThenarInfo[10]`，每项 `0x18` 字节)：大鱼际/手掌附加区域描述表。
  - `+0x130` (`u8 bThenarCount`)：附加区域数量。

- **单指触控单元 `TouchUnit`（起始于 `+0x140`，每单元步长 `0x168` = 360 字节，索引 `i = 0..19`）**：

| 绝对偏移 (`+0x140+i*0x168`) | 单元内偏移 | 类型 | 符号命名 | 物理含义与单位 |
|---|---|---|---|---|
| `+0x148` | `+0x08` | `u64` | `timestamp` | 当前帧时间戳 (`g_curTime`) |
| `+0x150` | `+0x10` | `i16` | `dt` | 格式化帧时间差 (`Touch_GetFormatTimeInterval`) |
| `+0x152` | `+0x12` | `i16` | `prevDt` | 上一帧格式化时间差 |
| `+0x154` | `+0x14` | `u8` | `touchId` | 触控点追踪 ID (`1..20`，`0` 表示未分配) |
| `+0x156` | `+0x16` | `u16` | `frameAge` | 自按下以来的累计帧数 |
| `+0x158` | `+0x18` | `i32` | `durationMs` | 自按下消抖开始以来的持续毫秒数 |
| `+0x15c` | `+0x1c` | `u16` | `zValue` | 信号强度幅值指标 $Z$ |
| `+0x15e, +0x160` | `+0x1e, +0x20` | `u16, u16` | `rawPos.dim1, dim2` | 原始质心坐标（Q8.8 传感器网格坐标，范围 `[0, 15104] × [0, 9984]`） |
| `+0x162, +0x164` | `+0x22, +0x24` | `u16, u16` | `ecPos.dim1, dim2` | `CTD_ECProcess` 边缘补偿后坐标（Q8.8） |
| `+0x166, +0x168` | `+0x26, +0x28` | `u16, u16` | `iirPos.dim1, dim2` | `Filter_IIRFilter` 一阶自适应滤波后坐标（Q8.8） |
| `+0x16a, +0x16b` | `+0x2a, +0x2b` | `u8, u8` | `iirRem.dim1, dim2` | IIR 滤波定点除法余数（用于跨帧抗截断误差累积） |
| `+0x16c, +0x16e` | `+0x2c, +0x2e` | `u16, u16` | `jitPos.dim1, dim2` | `Filter_JITFilter` 迟滞死区抗抖动后坐标（Q8.8） |
| `+0x170, +0x172` | `+0x30, +0x32` | `u16, u16` | `rptPos.dim1, dim2` | 最终用于报点转换的内部坐标（Q8.8） |
| `+0x180..+0x186` | `+0x40..+0x46` | `u16[4]` | `stableInfo` | 稳定状态锚点坐标 (`dim1`, `dim2`, `size`, `touchCount`) |
| `+0x188, +0x18a` | `+0x48, +0x4a` | `u16, u16` | `majorAxis, minorAxis` | 触控椭圆长轴、短轴物理长度（单位：`0.1 mm`） |
| `+0x18c, +0x18e` | `+0x4c, +0x4e` | `u16, u16` | `dim1Width, dim2Width` | Dim1/Dim2 轴向投影宽度 |
| `+0x192, +0x193` | `+0x52, +0x53` | `i8, i8` | `angle, maxSigAngle` | 当前椭圆倾角、最大信号量时刻的椭圆倾角 |
| `+0x194, +0x195` | `+0x54, +0x55` | `u8, u8` | `sizeInMM, udfpSizeMM` | `TS_Process` 计算出的等效触控直径（mm）与屏下指纹专用尺寸（mm） |
| `+0x196, +0x197` | `+0x56, +0x57` | `u8, u8` | `ecRatioDim1, Dim2` | 边缘外侧与内侧信号比值（`0..255`，用于 `CTD_ECGetOffset` 查表） |
| `+0x198` | `+0x58` | `u8` | `shapeRatio` | 几何长短轴形态比 |
| `+0x1a0, +0x1a2` | `+0x60, +0x62` | `i16, i16` | `z9, smoothZ9` | 九宫格中心信号和 $Z_9$ 及其跨帧平滑值 `(prev*7 + cur) >> 3` |
| `+0x1ac, +0x1ad` | `+0x6c, +0x6d` | `u8, u8` | `dim1Span, dim2Span` | 连通域跨越的列数、行数 |
| `+0x1ba..+0x1c0` | `+0x7a..+0x80` | `i16[4]` | `minPeak, maxGlobalPeak, maxPeak, maxZ4` | 追踪周期内记录的峰值极值统计 |
| `+0x1cc, +0x1d4` | `+0x8c, +0x94` | `i32, i32` | `sigSum, maxSigSum` | 当前连通域正信号总和、历史最大正信号总和 |
| `+0x1dc..+0x1de` | `+0x9c..+0x9e` | `u8[3]` | `sigSumRatio, z4Ratio, peakRatio` | 当前帧与上一帧信号量比值（定标 $\times 32$） |
| `+0x1e0` | `+0xa0` | `i32` | `tzType` | 连通域类型（`1`=Normal，`2`=Merge，`3`=Palm/Large） |
| `+0x1f0` | `+0xb0` | `u32` | `volatileFlags1` | 单帧标志位 1（`bit 4` 消抖未过、`bit 5` ER移入修正、`bit 6` ER移出修正、`bit 8/9` EC Dim1/2生效、`bit 11` FingerZ4不足、`bit 12` 稳定触控、`bit 20` 伪滑动） |
| `+0x1f4` | `+0xb4` | `u32` | `volatileFlags2` | 单帧标志位 2（`bit 0` 抬起Z不足、`bit 5` 按下峰值不足、`bit 12` 本帧对外报点 `0x1000`） |
| `+0x1f8` | `+0xb8` | `u32` | `persistFlags1` | 跨帧继承标志位 1（`bit 2` 移动超手套阈值、`bit 3` 曾报点 `0x8`、`bit 4` 强滤波、`bit 6` 补点激活、`bit 7` 补点历史就绪、`bit 8` 超点击抑制距离、`bit 10` ER移入挂起、`bit 11` 点击转滑动偏移激活、`bit 12` 超滑动阈值、`bit 17` 曾生成补点、`bit 18` 快速点击） |
| `+0x204` | `+0xc4` | `u32` | `edgeFlags` | 边缘区域标志位（`bit 0/1` EC Dim1近/远端、`bit 2/3` EC Dim2近/远端、`bit 4..7` ER移出边缘、`bit 8..11` 内边缘、`bit 12..15` 外边缘） |
| `+0x208` | `+0xc8` | `u32` | `suppressFlags` | 模式抑制位（`bit 1` 手指/手套模式抑制、`bit 2` 手写笔模式抑制、`bit 3` 伪笔尖抑制） |
| `+0x21c` | `+0xdc` | `u32` | `teState` | `TE` 事件状态机（`1`=TD消抖、`2`=TD确认、`4`=Move、`8`=LO消抖、`0x10`=LO快点保持、`0x20`=LO确认、`0x40`=WinPad抬起倒数第二帧） |
| `+0x224` | `+0xe4` | `u32` | `touchAction` | `TouchAction` 分类状态（`0`=未定、`1`=Click、`2`=Flick/Move） |
| `+0x22c, +0x22e` | `+0xec, +0xee` | `u16, u16` | `edgeDistDim1, Dim2` | 到最近传感器逻辑边界的距离（Q8.8） |
| `+0x238` | `+0xf8` | `u16` | `reqDebounceNum` | 动态要求的按下消抖帧数 |
| `+0x23c` | `+0xfc` | `u8` | `peakIdx` | 关联的 `Peak` 索引 |
| `+0x240..+0x258` | `+0x100..+0x118`| `u16/u32` | `sqDistMetrics` | 各类位移平方与加速度平方缓存（mm² 或 pixel²） |
| `+0x27c` | `+0x13c` | `u32` | `jitSqDistThold` | 动态 JIT 抗抖动半径平方阈值 |
| `+0x287` | `+0x147` | `u8` | `matchedPrevIdx` | `IDT` 匹配到的上一帧索引（`0..19`，`0x14` 表示无匹配新点） |

### 4.0.2 历史帧触控表 `g_prevTouchesPtr` (`PrevTouchTable` + `PrevTouchUnit[20]`)

每个 `PrevTouchUnit` 大小为 `0x478`（1144）字节，前 `0x144` 字节（`+0x140 .. +0x283`）在帧末由 `PrevTouch_RecordTouches` 直接从 `TouchUnit` 镜像拷贝，后续扩展字段维护跨帧运动学状态与历史轨迹队列：

- `+0x288` (`u8 historyBufferNum`)：`HistoryBuffer` 当前存储的历史坐标点数（容量上限 `0x1e` = 30）。
- `+0x290 .. +0x46f` (`HistoryEntry[30]`，每项 16 字节)：保存 `(x: i16, y: i16, pad: u32, timestamp: u64)`，供 `ER` 边缘回溯与 `Touch_CreatePostReport` 插值补点使用。
- `+0x474, +0x475, +0x476..+0x495`：16 槽位环形网格队列 `HistoryQueue`（存储 `(x>>8, y>>8)` 整格坐标）。
- `+0x496, +0x498` (`u16 startEcPos.dim1, dim2`)：首次按下时的 EC 坐标。
- `+0x49a, +0x49c` (`u16 firstRptPos.dim1, dim2`)：首次对外报点时的坐标（Click/Flick 位移基准）。
- `+0x4b8` (`u64 tdDebounceTimestamp`)：按下消抖最近一次更新的时间戳。
- `+0x4c8, +0x4ca` (`i16 dRawDim1, dRawDim2`)：原始质心帧间位移 $\Delta x, \Delta y$。
- `+0x4cc, +0x4ce` (`i16 vRawDim1, vRawDim2`)：原始质心归一化速度 $v_x = \Delta x / \Delta t, v_y = \Delta y / \Delta t$。
- `+0x4d0, +0x4d2` (`i16 aRawDim1, aRawDim2`)：原始质心加速度 $a_x = v_x(t) - v_x(t-1), a_y = v_y(t) - v_y(t-1)$（供 `IDT` 匈牙利匹配预测使用）。
- `+0x4d4, +0x4d6` (`i16 dEcDim1, dEcDim2`) 与 `+0x4d8, +0x4da` (`i16 ddEcDim1, ddEcDim2`)：EC 坐标一阶差分与二阶差分（供 `ER_MoveOutCheck` 预测滑出屏幕使用）。
- `+0x4e4` (`u16 debounceCounter`)：当前按下或抬起消抖已累计的帧数。
- `+0x4ee, +0x4f0` (`i16 clickToFlickOffsetDim1, Dim2`)：Click 切换至 Flick 时的平滑过渡偏移量。
- `+0x4f2` (`u8 matchedCurIdx`)：`IDT` 匹配到的当前帧索引（`0..19`，`0x14` 表示未匹配/丢失）。
- `+0x4f3` (`u8 postReportIirCoef`)：补点阶段 IIR 系数倍增状态（`2 -> 4 -> 8 -> 16 -> 32`）。
- `+0x4f4` (`u8 prevIirCoef`)：上一帧自适应 IIR 滤波权重系数（用于每帧 $\pm 1$ 斜率限制）。
- `+0x5ad .. +0x5b3`：`Filter_CurveReturnSuppress` 折返抑制与线性外推预测状态机。

---

## 4.1 质心计算与边缘非线性补偿模块 (`CTD.c`)

当手指靠近触摸屏物理边缘时，部分电容耦合场超出传感器阵列边界（截断效应），导致普通加权质心向屏幕内部偏移。`CTD` 模块提供基础质心算子与基于自然对数查表（`g_ctd256Ln`）的边缘补偿算法（Edge Compensation, EC）。

### 4.1.1 函数清单与完整分析 (`CTD.c` 共 10 个函数)

1. **`CTD_BasicCalculation @ 0x6ba551e0`**
   - **调用关系**：被 `CTD_General`、`TZ` 质心计算子函数调用。
   - **输入参数**：`param_1`（一阶矩累加和 $\sum S_i \cdot idx_i$），`param_2`（信号总和 $\sum S_i$）。
   - **数学公式**：
     $$\text{pos}_{Q8.8} = \max\left(0, \left\lfloor \frac{\text{param\_1} \times 256}{\text{param\_2}} \right\rfloor + 128\right)$$
     其中 $+128$（`0x80`）表示将传感器单元中心偏置到半格位置（例如第 0 列中心为 `0x0080`，局部计算后再由上层减去半格或对齐至 `[0, (N-1)*256]`）。若 `param_2 == 0` 则返回 `0`。

2. **`CTD_General @ 0x6ba5523c`**
   - **调用关系**：通用局部窗口质心算子；内部调用 `CTD_BasicCalculation`。
   - **算法逻辑**：给定中心行列 `(param_1, param_2)` 与窗口尺寸 `(param_3, param_4)`，遍历 `[param_1 - param_3/2, param_1 + param_3/2] × [param_2 - param_4/2, param_2 + param_4/2]` 范围内的有效传感器通道，读取差分信号 `g_tsaBufDif[row * bCols + col]`。仅累加正信号（`sVar1 > 0`）的列矩 `local_14`、行矩 `local_18` 和信号和 `local_1c`，最后调用两次 `CTD_BasicCalculation` 输出 `(dim1Pos, dim2Pos)`。

3. **`CTD_ECInit @ 0x6ba553a0`**
   - **调用关系**：被 `TSA_Init` 调用。空函数（直接 `return`），因为 EC 参数直接从 `g_tsaPrmtFlash` 动态拷贝至 `g_tsaPrmtRam`。

4. **`CTD_ECProcess @ 0x6ba55ca0`**
   - **调用关系**：被 `TSA_Processing @ 0x6ba9a443` 直接调用；调用 `CTD_ECProcessUnit`、`Touch_SetEdgeDist`。
   - **算法逻辑**：遍历当前帧所有触控点 `bTouchIndex = 0 .. bTouchCount - 1`：
     1. 将原始质心 `rawPos`（`+0x15e, +0x160`）初始拷贝至 `ecPos`（`+0x162, +0x164`）；
     2. 调用 `CTD_ECProcessUnit(bTouchIndex)` 执行双轴边缘补偿；
     3. 将补偿后的 `ecPos` 同步写入 `rptPos`（`+0x170, +0x172`），并再次调用 `Touch_SetEdgeDist` 更新到逻辑边缘的距离。

5. **`CTD_ECProcessUnit @ 0x6ba55c6b`**
   - **调用关系**：被 `CTD_ECProcess` 调用；依次调用 `CTD_ECUpdatePosToEC`、`CTD_ECProcessDim1`、`CTD_ECProcessDim2`。

6. **`CTD_ECUpdatePosToEC @ 0x6ba554d8`**
   - **调用关系**：被 `CTD_ECProcessUnit` 调用；内部直接调用 `Touch_SetEdgeDist(param_1)`，根据当前坐标设置 `+0x204` 的近端/远端边缘标志位（`bit 0..3`）。

7. **`CTD_ECProcessDim1 @ 0x6ba5555b`**
   - **调用关系**：被 `CTD_ECProcessUnit` 调用；调用 `CTD_ECGetOffset`、`CTD_ECGetFinalOffset`。
   - **算法逻辑**：
     1. 清除 `volatileFlags1`（`+0x1f0`）的 `0x100` 位（Dim1 EC 生效标志）。
     2. 检查 `edgeFlags`（`+0x204`）：
        - 若 `bit 1`（`0x2`，Dim1 远端边缘，对应右边缘 `col ≈ 59`）置位：从 `g_tsaPrmtFlash + 0x0851`（即 `pTVar1[1].field_0x11`）拷贝 17 字节参数表至 `g_tsaPrmtRam`；计算原始位置距远端逻辑边界 `dim1FarEdge`（`g_tsaPrmtFlashConst+0x2c = 0x3b00 = 15104`）的距离 `iVar5 = dim1FarEdge - rawPos.dim1`；调用 `sVar3 = CTD_ECGetOffset(ecRatioDim1, sizeInMM)` 与 `iVar4 = CTD_ECGetFinalOffset(iVar5, 256 - sVar3)`，更新 `ecPos.dim1 = dim1FarEdge - iVar4`。
        - 若 `bit 0`（`0x1`，Dim1 近端边缘，对应左边缘 `col ≈ 0`）置位：从 `g_tsaPrmtFlash + 0x0840`（`pTVar1[1]`）拷贝 17 字节参数表至 `g_tsaPrmtRam`；计算距近端逻辑边界 `dim1NearEdge`（`g_tsaPrmtFlashConst+0x2a = 0`）的距离 `iVar5 = rawPos.dim1 - dim1NearEdge`；更新 `ecPos.dim1 = dim1NearEdge + CTD_ECGetFinalOffset(iVar5, 256 - sVar3)`。
     3. 若修正后距离 `iVar4 != iVar5`，置位 `volatileFlags1 |= 0x100`。

8. **`CTD_ECProcessDim2 @ 0x6ba558e4`**
   - **调用关系**：被 `CTD_ECProcessUnit` 调用；调用 `CTD_ECGetOffset`、`CTD_ECGetFinalOffset`。
   - **算法逻辑**：与 `CTD_ECProcessDim1` 完全对称，作用于 Dim2（行方向）：
     - 清除 `volatileFlags1` 的 `0x200` 位；
     - 若 `edgeFlags & 0x8`（Dim2 远端边缘，下边缘 `row ≈ 39`，边界值 `g_tsaPrmtFlashConst+0x30 = 0x2700 = 9984`）：加载 `g_tsaPrmtFlash + 0x0873`（17 字节），将坐标向外拉向 `dim2FarEdge`；
     - 若 `edgeFlags & 0x4`（Dim2 近端边缘，上边缘 `row ≈ 0`，边界值 `g_tsaPrmtFlashConst+0x2e = 0`）：加载 `g_tsaPrmtFlash + 0x0862`（17 字节），将坐标向外拉向 `dim2NearEdge`；
     - 若发生修正，置位 `volatileFlags1 |= 0x200`。

9. **`CTD_ECGetOffset @ 0x6ba553a6`**
   - **调用关系**：被 `CTD_ECProcessDim1`、`CTD_ECProcessDim2` 调用。
   - **输入参数**：`param_1`（`ecRatio`，边缘最外侧电极与次外侧电极信号比，`0..255`），`param_2`（`sizeInMM`，当前触控物理尺寸）。
   - **算法与公式**：
     1. 根据 `param_2`（`sizeInMM`）在 `g_tsaPrmtRam` 分段表中查找分段索引 `segIdx`（表头 `g_tsaPrmtRam[0]` 为分段数 `N_seg`，每段 4 字节：`[sizeLimit, reserved, lnLowIdx, lnHighIdx]`）。
     2. 利用全局自然对数查表数组 `g_ctd256Ln`（256 项 `u16`），将信号比值 `param_1` 在对数域进行线性插值：
        $$\text{offset} = \min\left(255, \left\lfloor \frac{(\ln_{\text{table}}[\text{param\_1}] - \ln_{\text{table}}[\text{lowIdx}]) \times 256}{\ln_{\text{table}}[\text{highIdx}] - \ln_{\text{table}}[\text{lowIdx}]} \right\rfloor\right)$$
   - **`W273AS2700` 参数实例**：
     - `Dim1Near` (`flash+0x0840`) 与 `Dim1Far` (`flash+0x0851`)：`N_seg = 1`，`sizeLimit = 7 mm`，`lowIdx = 16 (0x10)`，`highIdx = 224 (0xe0)`。
     - `Dim2Near` (`flash+0x0862`) 与 `Dim2Far` (`flash+0x0873`)：`N_seg = 1`，`sizeLimit = 7 mm`，`lowIdx = 16 (0x10)`，`highIdx = 96 (0x60)`（行方向边缘截断更陡峭，因此高阈值索引设为 `96`）。

10. **`CTD_ECGetFinalOffset @ 0x6ba554f7`**
    - **调用关系**：被 `CTD_ECProcessDim1`、`CTD_ECProcessDim2` 调用。
    - **输入参数**：`param_1`（原始质心距边缘距离 $d_{\text{raw}}$），`param_2`（对数模型估算的边缘距离 $d_{\text{ec}} = 256 - \text{offset}$）。
    - **数学公式**：实现边缘 1 格（`256`）到 1.25 格（`256 + 64 = 320`）之间的平滑过渡混合，避免手指进出边缘补偿区时坐标跳变：
      $$d_{\text{final}} = \begin{cases} d_{\text{ec}}, & d_{\text{raw}} \le 256 \\ \left\lfloor \frac{4(d_{\text{raw}} - 256) \cdot d_{\text{raw}} + \big(256 - 4(d_{\text{raw}} - 256)\big) \cdot d_{\text{ec}}}{256} \right\rfloor, & 256 < d_{\text{raw}} < 320 \\ d_{\text{raw}}, & d_{\text{raw}} \ge 320 \end{cases}$$

---

## 4.2 边缘防误触与移入/移出轨迹修正模块 (`ER.c`)

当手指从屏幕外滑入（Edge Move-In）或滑出屏幕边缘（Edge Move-Out）时，初期信号弱、位置偏向内侧，容易导致边缘手势（如返回手势、底部上滑 Home 手势）丢失起点或终点。`ER`（Edge Rejection / Edge Recovery）模块负责在按下初期延迟报点并回溯插值起点（Move-In Correction），以及在滑出边缘抬起时将终点吸附到物理边界（Move-Out Correction）。

### 4.2.1 函数清单与完整分析 (`ER.c` 共 12 个函数)

1. **`ER_MMPrmtToPixelPrmt @ 0x6ba56e50`**
   - **调用关系**：被 `ER_Init` 调用；调用 `TS_MMToDisplayPixel`、`TSA_SideToDimEdge`、`TSAOut_RevertDim1Pos`、`TSAOut_RevertDim2Pos`。
   - **功能**：将 Flash 中以 `0.1 mm` 为单位的边缘区间阈值转换为内部 Q8.8 传感器像素坐标距离（先通过 `TS_MMToDisplayPixel` 转为 $\times 16$ 显示像素，右移 4 位减 5 后反算回传感器坐标）。

2. **`ER_GetPrmt @ 0x6ba56ec0`**
   - **调用关系**：被 `ER_Init`、`ER_IsTouchNeedMoveInCorrection` 调用；调用 `TSA_DimEdgeToSide`、`TSA_SideToEdge`。
   - **功能**：根据逻辑边枚举 `param_1`，读取 `g_tsaPrmtFlashConst + 0x40 + edgeIdx * 6` 处的像素内界 `*param_2` 与外界 `*param_3`，并更新全局变量 `g_erLimit`。

3. **`ER_Init @ 0x6ba56f76`**
   - **调用关系**：被 `TSA_Init` 调用；调用 `ER_MMPrmtToPixelPrmt`、`TSA_SideToDimEdge`、`ER_GetPrmt`。
   - **功能与 `W273AS2700` 参数**：将四条物理边（Side `1, 2, 4, 8`，位于 `flash+0x0924..0x093b`）的 Move-In 内外距离阈值转换为传感器坐标存入 `g_tsaPrmtFlashConst+0x40..0x54`：
     - Side 1 (`flash+0x0924, +0x0926`)：`[64, 160]`（`6.4 mm ~ 16.0 mm`），底部上滑开关 `bBottomMoveInEnable` (`flash+0x0928`) = `0`；
     - Side 2 (`flash+0x092a, +0x092c`)：`[32, 96]`（`3.2 mm ~ 9.6 mm`）；
     - Side 4 (`flash+0x0930, +0x0932`)：`[64, 160]`（`6.4 mm ~ 16.0 mm`）；
     - Side 8 (`flash+0x0936, +0x0938`)：`[64, 160]`（`6.4 mm ~ 16.0 mm`）。

4. **`ER_IsTouchNeedMoveInCorrection @ 0x6ba5719b`**
   - **调用关系**：被 `ER_TouchDownProcess` 调用；调用 `TSA_GetDimEdge`、`ER_GetPrmt`、`Touch_GetDistToLogicEdge`、`TPSensor_IsNoMoveInCorrection`。
   - **判定条件**：当触控点到最近逻辑边的距离 `dist` 满足 `innerLimit < dist < outerLimit`、长轴 `majorAxis < 30`（`3.0 mm`），且未处于禁止修正区（`TPSensor_IsNoMoveInCorrection == 0`）时返回 `1`。

5. **`ER_IsInBottomZone @ 0x6ba572bf`**
   - **调用关系**：被 `ER_BottomMoveInCorrection` 调用；调用 `TSAOut_Pixel2DisplayDim1`、`TSAOut_Pixel2DisplayDim2`。
   - **判定条件**：将坐标转为屏幕显示像素 `(dispX, dispY)`，检查是否位于屏幕底部手势热区：`dispY >= yMax - 54` 且 `xMin + 100 <= dispX < xMax - 100`（在 `W273AS2700` 2560×1600 下即 $Y \in [1545, 1599], X \in [100, 2459]$）。

6. **`ER_BottomMoveInCorrection @ 0x6ba57355`**
   - **调用关系**：被 `ER_TouchDownProcess` 调用；调用 `PrevTouch_HistoryBufferGetNum`、`PrevTouch_HistoryBufferGet`、`ER_IsInBottomZone`、`PrevTouch_HistoryBufferPop`。
   - **功能**：针对底部上滑手势的快速移入检测（需 `flash+0x0928 != 0`、`majorAxis <= 29`、`maxPeak >= 1000`；在 `W273AS2700` 中因 `flash+0x0928 == 0` 默认关闭）。若消抖计数为 1~3 帧且历史起点在底部热区内、向上位移 $\Delta Y > 50$（单帧）或 $> 100$（多帧）且横向偏移 $|\Delta X| < 256$，则弹出多余历史帧并返回 `1`。

7. **`ER_MoveInCorrection @ 0x6ba57786`**
   - **调用关系**：被 `ER_MoveInCheck` 调用；调用 `PrevTouch_HistoryBufferGetNum`、`PrevTouch_HistoryBufferPop`、`PrevTouch_HistoryBufferGet`、`PrevTouch_IsHistoryBufferFull`、`PrevTouch_HistoryBufferInsert`。
   - **轨迹反向外推算法**：
     1. 将 `HistoryBuffer` 裁剪至仅保留最早的 2 个历史点 $P_0(x_0, y_0, t_0)$ 和 $P_1(x_1, y_1, t_1)$。
     2. 计算步进差分向量 $(\Delta x, \Delta y, \Delta t) = (x_1 - x_0, y_1 - y_0, t_1 - t_0)$。
     3. 循环向反方向（屏幕边缘方向）线性外推：$x_{\text{new}} = x_{\text{prev}} - \Delta x$，$y_{\text{new}} = y_{\text{prev}} - \Delta y$，$t_{\text{new}} = t_{\text{prev}} - \Delta t$，并钳位至传感器逻辑边界 `[dim1Near, dim1Far-1] × [dim2Near, dim2Far-1]`，插入到 `HistoryBuffer` 头部（索引 `0`），直到触达任一逻辑边界或缓冲区满。后续由 `Touch_CreatePostReport` 逐帧回放这些插值点。

8. **`ER_MoveInCheck @ 0x6ba57a07`**
   - **调用关系**：被 `ER_TouchDownProcess` 调用；调用 `PrevTouch_HistoryBufferGetNum`、`PrevTouch_HistoryBufferGet`、`Touch_GetSQDistPixel`、`Touch_GetSQDistPixelToEdge`、`ER_MoveInCorrection`。
   - **算法逻辑**：取历史缓冲最后两点 $P_{n-2}, P_{n-1}$ 与当前点 $P_{\text{cur}}$，要求：
     - 相邻步长平方 $\|P_{n-1} - P_{n-2}\|^2 \ge 2500$（`0x9c4`，即步长 $\ge 50$ Q8.8 单位）且 $\|P_{\text{cur}} - P_{n-1}\|^2 \ge 2500$；
     - 两帧总位移平方 $\|P_{\text{cur}} - P_{n-2}\|^2 \ge 10000$（即总位移 $\ge 100$ Q8.8 单位）；
     - 朝向屏幕内部移动（$\|P_{\text{cur}} - P_{n-2}\|^2 \ge \text{SQDistToEdge}$）。
     全部满足时调用 `ER_MoveInCorrection` 生成反向延伸轨迹并返回 `1`。

9. **`ER_MoveOutCheck @ 0x6ba57bd3`**
   - **调用关系**：被 `ER_Process` 调用。
   - **算法逻辑**：当已报点的触控点进入抬起状态时，利用上一帧的 EC 坐标、一阶差分 `dEc`（`+0x4d4/+0x4d6`）与二阶差分 `ddEc`（`+0x4d8/+0x4da`）预测下一时刻位置：
     $$\hat{x} = x_{\text{ec}} + \Delta x_{\text{ec}} + \Delta^2 x_{\text{ec}}, \quad \hat{y} = y_{\text{ec}} + \Delta y_{\text{ec}} + \Delta^2 y_{\text{ec}}$$
     若 $\hat{x}$ 或 $\hat{y}$ 超出四条逻辑边界（`<= Near` 或 `>= Far - 1`），则在 `prevTouch.edgeFlags`（`+0x204`）置位对应的移出标志位（`0x10 / 0x20 / 0x40 / 0x80`）并返回 `1`。

10. **`ER_MoveOutCorrection @ 0x6ba57e7d`**
    - **调用关系**：被 `ER_Process` 调用。
    - **功能**：根据 `prevTouch.edgeFlags` 的 `0x10 / 0x20 / 0x40 / 0x80` 标志位，将当前帧 `rptPos`（`+0x170, +0x172`）直接吸附到对应的逻辑边界极值（`0`、`15103`、`0` 或 `9983`）。

11. **`ER_TouchDownProcess @ 0x6ba57ffd`**
    - **调用关系**：被 `ER_Process` 调用；调用 `ER_BottomMoveInCorrection`、`ER_IsTouchNeedMoveInCorrection`、`ER_MoveInCheck`。
    - **状态流转**：当触控点刚进入 `TE = 2`（TouchDown Confirmed）且处于需要 Move-In 修正的边缘带时，若消抖帧数 `< 2`，则将其 `TE` 状态打回 `1`（Debounce），置位 `persistFlags1 |= 0x400`（Move-In 挂起）并强制要求消抖帧数 `reqDebounceNum >= 2`；待下一帧再次确认时执行 `ER_MoveInCheck`，若确认为滑入则置位 `volatileFlags1 |= 0x20` 与 `persistFlags1 |= 0x80`（激活 `HistoryBuffer` 补点回放）。

12. **`ER_Process @ 0x6ba584b3`**
    - **调用关系**：被 `TSA_Processing @ 0x6ba9a443` 在第 19 步（`TE_Process` 之后、`Filter_Process` 之前）调用；调用 `PrevTouch_GetIdxBasedOnCurIdx`、`ER_TouchDownProcess`、`ER_MoveOutCheck`、`ER_MoveOutCorrection`。
    - **顶层控制流**：遍历所有触控点，对 `TE == 2` 的点调用 `ER_TouchDownProcess`；对在挂起期间提前抬起（`TE == 0x20` 且 `persistFlags1 & 0x400`）的点直接释放历史缓冲（置位 `0x80`）以防漏报边缘快点；对 `TE == 8` 或 `0x20` 的已报点检测 `ER_MoveOutCheck`，若滑出边界则调用 `ER_MoveOutCorrection` 吸附终点，并将 `TE == 0x20` 改为 `0x10`（延迟一帧抬起以便把吸附后的边缘终点报出）。

---

## 4.3 热区坐标微调与诊断记录模块 (`HotZoneAdjust.c`)

`HotZoneAdjust.c` 包含 2 个导出函数，用于在特定应用热区（HotZone）内对首次报点及后续移动点进行微调与日志统计（通过 `NO_PREFIX.c` 中的内部辅助函数实现）：

1. **`HotZoneAdjust_Init @ 0x6baf3fdd`**
   - **调用关系**：被 `TSA_Init` 调用；调用 `HotZoneAdjustClearAnalyzer @ 0x6baf3da0`，并将 `g_hotZoneAnalyzer` 指向 `g_tsaStaticPtr->pFrameBufferPtr`。
2. **`HotZoneAdjust_Process @ 0x6baf729e`**
   - **调用关系**：被 `TouchMode_FingerProcess` / `Touch_CreatePostReport` 后处理路径调用；调用 `HotZoneAdjustIsFeatureOn @ 0x6baf3ff9`、`HotZoneAdjustScenario @ 0x6baf4385`、`ClearHotZone @ 0x6baf3e32`、`HotZoneAdjustProcessFirst @ 0x6baf44be`、`HotZoneAdjustProcessAfter @ 0x6baf4666`、`IsNeedRecord @ 0x6baf6a0c`、`HotZoneRecord @ 0x6baf6a49`、`GenerateAndGetDetailDescription @ 0x6baf6e55`。
   - **算法逻辑**：若特性开启且当前场景匹配，在触控首次报点时调用 `HotZoneAdjustProcessFirst`，持续移动时调用 `HotZoneAdjustProcessAfter`；当触控结束报点（上一帧 `persistFlags1 & 8` 为真而当前帧为假）时记录热区统计并通过 `StrPrintf_Process` 输出 `AppLog:` 诊断描述。

---

## 4.4 最小二乘矩阵拟合与特殊位置队列辅助模块 (`MtLsm.c` / `MtLst.c` / `MtDiff.c` / `PolynomialRegression.c` / `PosnSpecial.c`)

这 5 个辅助工具模块为触控信号重构、基线线性校正、多项式曲面拟合与连通域正负信号双端队列遍历提供底层数学支撑。

### 4.4.1 `MtLst.c` (1 个函数) 与 `MtDiff.c` (1 个函数)

1. **`MtLst_GetVectorA @ 0x6ba540e0` (`MtLst.c`)**
   - **调用关系**：被 `MtLsm_CalcMatrixB` 调用。
   - **功能**：从模型配置块 `param_3` 的设计矩阵指针 `*(param_3 + 0x20)` 中提取第 `param_2` 行的基函数行向量 $A_{i, 0..m-1}$（维度 $m = \text{*(u8*)(param\_3 + 0x11)}$）。

2. **`MtDiff_MsLinearCorrectionFilterCore @ 0x6ba55131` (`MtDiff.c`)**
   - **调用关系**：被前端差分信号线性校正滤波调用；内部调用 `MtLsm_LinearRegressionWeight`。
   - **功能**：给定观测向量 $Y$（`*param_4`，长度 $N = \text{*(u8*)(param\_4 + 0x10)}$），调用 `MtLsm_LinearRegressionWeight` 计算最小二乘拟合向量 $\hat{Y}$（写入 `param_2`），再逐元素计算残差 $E_i = Y_i - \hat{Y}_i$ 写入输出缓冲 `param_1`。

### 4.4.2 `MtLsm.c` 多触控最小二乘求解器 (共 12 个函数)

设传感器观测数为 $N$（`+0x10`），回归状态维数为 $m$（`+0x11`），设计矩阵为 $A \in \mathbb{R}^{N \times m}$（`+0x20`），预计算的格拉姆矩阵 $G = A^T A \in \mathbb{R}^{m \times m}$（`+0x28`），预计算的伪逆投影矩阵 $B = (A^T A)^{-1} A^T \in \mathbb{R}^{m \times N}$（定点化 $\times 1024$ 存于 `+0x38`）：

1. **`MtLsm_CalcMatrixB @ 0x6ba54882`**：遍历 $i = 0..N-1$ 调用 `MtLst_GetVectorA` 累加构造上三角 $G = A^T A$，调用 `Matrix_SymmetricFromUpper` 补全对称阵存入 `param_2`；接着调用 `Matrix_Inv` 求逆得 $(A^T A)^{-1}$，对每个传感器行向量计算 $(A^T A)^{-1} a_i^T$ 并乘以 `1024.0f`（`DAT_6bb51d6c`）定点化为 `i16` 存入 `param_1`。
2. **`MtLsm_LinearRegressionWeight @ 0x6ba550d4`**：先调用 `MtLsm_LinearRegressionCalcState` 求解回归状态向量 $\theta \in \mathbb{R}^m$（存于 `param_1 + 0x40`），再调用 `MtLsm_LinearReconstruct` 重构拟合值 $\hat{Y} = A \theta$。
3. **`MtLsm_LinearReconstruct @ 0x6ba5453b`**：计算矩阵乘法 $\hat{Y} = A \theta$，针对 $m = 2, 3, 4$ 进行了循环展开优化，并将结果饱和钳位至 `i16` 范围 `[-32768, 32767]`。
4. **`MtLsm_LinearRegressionCalcState @ 0x6ba55037`**：若全传感器有效标志 `*(char*)(param_1 + 0x30) != 0`，调用快速定点路径 `MtLsm_GetStateByAllSensor`；否则调用带掩码剔除的 `MtLsm_GetStateByWeightSensor`。
5. **`MtLsm_GetStateByAllSensor @ 0x6ba54c29`**：直接用预计算的 `i16` 伪逆矩阵 $B$ 与观测向量 $Y$ 做定点内积并右移 10 位：$\theta_k = \lfloor (\sum_{i=0}^{N-1} Y_i \cdot B_{k,i}) / 1024 \rfloor$。
6. **`MtLsm_GetStateByWeightSensor @ 0x6ba54cfe`**：支持部分传感器失效（权重为 0）的动态最小二乘求解。以全量 $G = A^T A$ 为初值，遍历 $i = 0..N-1$：若权重 `w[i] == 0`，则从 $G$ 中减去秩 1 外积 $a_i^T a_i$（Downdate）；若 `w[i] != 0`，则累加右端项 $b = \sum a_i^T Y_i$。若有效传感器数 $< m$ 则清零状态，否则调用 `MtLsm_Solve` 求解，且在约束标志 `+0x12 != 0` 时调用 `MtLsm_SolveWithConstrain`。
7. **`MtLsm_Solve @ 0x6ba54158`**：调用 `Matrix_Inv` 与 `Matrix_Mul` 计算无约束解 $\theta = G^{-1} b$。
8. **`MtLsm_SolveWithConstrain @ 0x6ba543cb`**：带二次型范数约束 $\theta^T C \theta = c_0$ 的拉格朗日乘子牛顿迭代求解器（最多迭代 6 次，收敛阈值 $F(\theta) < 0.01$）：
   $$\lambda_{k+1} = \lambda_k - \frac{F(\theta(\lambda_k))}{F'(\theta(\lambda_k))}, \quad \theta(\lambda) = (G + \lambda C)^{-1} b$$
9. **`MtLsm_GetInvATA @ 0x6ba541a0`**：计算正则化逆矩阵 $(G + \lambda C)^{-1}$（调用 `Matrix_Mul_C`、`Matrix_Add`、`Matrix_Inv`）。
10. **`MtLsm_GetP @ 0x6ba54204`**：给定拉格朗日乘子 $\lambda$（`param_5`），调用 `MtLsm_GetInvATA` 与 `Matrix_Mul` 更新状态向量 $\theta(\lambda) = (G + \lambda C)^{-1} b$。
11. **`MtLsm_GetF @ 0x6ba54260`**：计算约束残差 $F(\theta) = \theta^T C \theta - 1.0$。
12. **`MtLsm_GetFDeriv @ 0x6ba542cc`**：计算导数 $F'(\lambda) = -2 \cdot b^T (G + \lambda C)^{-T} C (G + \lambda C)^{-1} C \theta$（其中 `DAT_6bb51d64 = -2.0f`）。

### 4.4.3 `PolynomialRegression.c` (1 个函数) 与 `PosnSpecial.c` (18 个函数)

- **`PolynomialRegression_SetComb @ 0x6baf7460` (`PolynomialRegression.c`)**：生成多项式回归所需的非递增幂次组合表（Multichoose / 组合索引生成器），将 `param_3` 个变量在最高阶数 `param_4` 下的所有单项式组合写入 `param_1`。
- **`PosnSpecial.c` 双端符号分区队列（共 18 个函数）**：用于在区域生长/漫水填充时按正负信号分端存储网格索引（正信号从头部 `DAT_6bcac4ca` 向后压入，负信号从尾部 `DAT_6bcac4cc` 向前压入），并在交替弹出时统计最多 10 个符号连通分区（Zone `0..9`）的极值、信号和、过零次数（SignChange）与边缘传感器数：
  1. `PosnSpecial_Init @ 0x6bb422b6`：清空 `0x98` 字节控制块 `g_list`，初始化头尾指针并重置 Zone 0。
  2. `PosnSpecial_ReBase @ 0x6bb423b7`：重置过零计数 `DAT_6bcac4d3`、有效传感器数 `DAT_6bcac4d4`、边缘传感器数 `DAT_6bcac4d5` 与边缘信号和 `DAT_6bcac4d8`。
  3. `PosnSpecial_Put @ 0x6bb42569`：若 `g_tsaBufDif[cellIdx] >= 0` 压入队列前端，否则压入队列后端。
  4. `PosnSpecial_Get @ 0x6bb42624`：先调用 `PosnSpecial_UpdateSign`，再从当前活动端（正端或负端）弹出一个网格索引并调用 `PosnSpecial_AddCellInZone` 计入当前 Zone。
  5. `PosnSpecial_IsEmpty @ 0x6bb42761`：当正端与负端均为空时返回 `1`。
  6. `PosnSpecial_IsPosEmpty @ 0x6bb42050`：判断正端指针 `DAT_6bcac4ca == 0`。
  7. `PosnSpecial_IsNegEmpty @ 0x6bb42067`：判断负端指针 `DAT_6bcac4cc == cap`。
  8. `PosnSpecial_UpdateSign @ 0x6bb42265`：当当前活动端耗尽时，调用 `PosnSpecial_NextZone` 切换至另一端。
  9. `PosnSpecial_NextZone @ 0x6bb42163`：递增 Zone 索引 `DAT_6bcac4dd`（上限 9）并调用 `PosnSpecial_ResetZone`。
  10. `PosnSpecial_ResetZone @ 0x6bb420ba`：初始化指定 Zone 的最大值（`-32768`）、最小值（`32767`）、信号和（`0`）与计数（`0`）。
  11. `PosnSpecial_AddCellInZone @ 0x6bb421ae`：更新当前 Zone 的最大/最小信号值、信号累加和与通道计数。
  12. `PosnSpecial_Record @ 0x6bb42487`：记录有效传感器数及符号翻转（过零）次数。
  13. `PosnSpecial_Record2 @ 0x6bb4251d`：记录边缘传感器个数与边缘信号累加和。
  14. `PosnSpecial_GetNumSignChange @ 0x6bb42444`：返回符号翻转次数 `DAT_6bcac4d3`。
  15. `PosnSpecial_GetNumValidSensor @ 0x6bb42455`：返回有效传感器数 `DAT_6bcac4d4`。
  16. `PosnSpecial_GetNumEdgeSensor @ 0x6bb42466`：返回边缘传感器数 `DAT_6bcac4d5`。
  17. `PosnSpecial_GetSumEdgeSensor @ 0x6bb42477`：返回边缘信号总和 `DAT_6bcac4d8`。
  18. `PosnSpecial_GetPosMarker @ 0x6bb423f7`：返回正分区标记计数 `DAT_6bcac4d1`（最小为 `1`）。

---

## 4.5 跨帧触控 ID 追踪与匈牙利算法模块 (`IDT.c`)

`IDT`（ID Tracking）模块在 `TSA_Processing` 中被调用两次：第一次在 `TZ_SplitMergedTZ` 前以 `IDT_Process(1)`（严格加速度门限预追踪）试探触控分裂关系；第二次在分裂完成后以 `IDT_Process(0)` 正式完成上一帧 `PrevTouch[0..M-1]` 到当前帧 `Touch[0..N-1]` 的最优二分图匹配与运动学校验。

### 4.5.1 函数清单与完整分析 (`IDT.c` 共 11 个函数)

1. **`IDT_Init @ 0x6ba611a0`**
   - **调用关系**：被 `TSA_Init` 调用（空函数）。

2. **`IDT_Process @ 0x6ba611a6`**
   - **调用关系**：被 `TSA_Processing @ 0x6ba9a443` 调用两次（参数 `1` 与 `0`）；依次调用 `IDT_HMProcess`、`IDT_AccCheck`、`IDT_Map`。
   - **算法逻辑**：在栈上分配 20 字节映射表 `abIdtScratch[20]`（`abIdtScratch[prevIdx] = curIdx`，未匹配为 `0xff`），依次执行匈牙利全局最小代价匹配、运动学加速度剔除及 ID 赋值。

3. **`IDT_HMProcess @ 0x6ba61323`**
   - **调用关系**：被 `IDT_Process` 调用；调用 `IDT_PopulateDistanceMap`、`IDT_HM`。
   - **算法逻辑**：在栈上分配 `20 × 20 × 4 = 1600`（`0x640`）字节距离矩阵。由于 `IDT_HM` 要求行数 `dim1Len <= dim2Len`（左部顶点数 $\le$ 右部顶点数）：
     - 若当前帧点数 $N <$ 上一帧点数 $M$：调用 `IDT_PopulateDistanceMap(distMap, 1)`（以 `curIdx` 为行、`prevIdx` 为列），调用 `IDT_HM(N, M, matchOut, distMap)`，直接得到 `matchOut[prevIdx] = curIdx`；
     - 若 $N \ge M$：调用 `IDT_PopulateDistanceMap(distMap, 0)`（以 `prevIdx` 为行、`curIdx` 为列），调用 `IDT_HM(M, N, matchOut, distMap)`，再将输出反转写入 `param_1[matchOut[curIdx]] = curIdx`。

4. **`IDT_PopulateDistanceMap @ 0x6ba623e4`**
   - **调用关系**：被 `IDT_HMProcess` 调用；调用 `TS_MMToPixel`、`IDT_IsEdgeTouch`、`IDT_AlwaysMatchProcess`。
   - **核心代价矩阵与运动预测公式**：
     1. 读取近距优先阈值 $D_{\text{always}} = \text{TS\_MMToPixel}(\text{flash+0x0888})$（在 `W273AS2700` 中 `flash+0x0888 = 5 mm`，对应传感器像素约 $5 \times 38 = 190$）。
     2. 对每一对 `(curIdx = i, prevIdx = j)`：
        - 计算原始坐标欧氏距离平方：
          $$d_{i,j}^2 = (x_i - x_j)^2 + (y_i - y_j)^2$$
        - 利用上一帧速度 $(v_{x,j}, v_{y,j})$（`+0x4cc, +0x4ce`）和加速度 $(a_{x,j}, a_{y,j})$（`+0x4d0, +0x4d2`）进行二阶泰勒运动学外推：
          $$\hat{x}_j = x_j + v_{x,j} \Delta t + \left\lfloor \frac{a_{x,j}}{2} \right\rfloor \Delta t, \quad \hat{y}_j = y_j + v_{y,j} \Delta t + \left\lfloor \frac{a_{y,j}}{2} \right\rfloor \Delta t$$
        - **近距静止锁定**：若曼哈顿位移 $|x_i - x_j| + |y_i - y_j| \le D_{\text{always}}$，且追踪帧龄 `frameAge > 5`（低功耗模式下 `> 0`），且预测位移 $|v_{x,j}\Delta t| + |v_{y,j}\Delta t| \le D_{\text{always}}$，则直接将代价设为极大负奖励值 `cost = -0x3fffffff`（强制优先匹配）。
        - **高速轨迹外推锁定**：若不满足静止锁定，但外推预测误差 $|\hat{x}_j - x_i| + |\hat{y}_j - y_i| \le D_{\text{always}}$ 且当前点非边缘点（`IDT_IsEdgeTouch(i) == 0`），则将 `(i, j)` 加入 `local_a8` 候选表，交由 `IDT_AlwaysMatchProcess` 做无歧义唯一性验证。
        - **点数减少时的速度一致性惩罚**：当 $N < M$（发生手指抬起或合并）时，在欧氏距离平方上叠加两倍的速度残差平方：
          $$\text{cost}_{i,j} = d_{i,j}^2 + 2 \cdot \big((x_i - x_j - v_{x,j})^2 + (y_i - y_j - v_{y,j})^2\big)$$

5. **`IDT_AlwaysMatchProcess @ 0x6ba620ac`**
   - **调用关系**：被 `IDT_PopulateDistanceMap` 调用；调用 `TS_MMToPixel`、`IDT_GetAcc`、`IDT_IsToIncAccThold`。
   - **功能**：对 `local_a8` 中的高速外推命中对 `(i, j)`，检查双向排他性——若 `i` 对所有其他 `prevIdx != j` 的加速度均超过阈值，且 `j` 对所有其他 `curIdx != i` 的加速度也均超过阈值（即 `(i, j)` 是唯一的运动学合法配对），则将其代价矩阵元素设为 `0xc0000001`（`-0x3fffffff`），确保匈牙利算法锁定该高速轨迹。

6. **`IDT_IsEdgeTouch @ 0x6ba622e9`**
   - **调用关系**：被 `IDT_PopulateDistanceMap` 调用。
   - **功能**：判断 `rawPos >> 8` 所在的行列是否位于最外圈传感器（`col <= 0` 或 `col >= bCols - 1`，当 `dwFeatureFlags & 0x8000` 时右边界放宽至 `bCols - 2`；或 `row == 0` 或 `row >= bRows - 1`）。

7. **`IDT_HM @ 0x6ba61ba2`**
   - **调用关系**：被 `IDT_HMProcess` 调用。
   - **算法逻辑**：经典的 $O(V^3)$ **Jonker-Volgenant / Kuhn-Munkres（匈牙利算法）最短增广路实现**：
     - 维护左部顶标 `u[0..dim1Len]`（`aiStack_78`）、右部顶标 `v[0..dim2Len-1]`（`aiStack_c8`）、松弛量数组 `slack[0..dim2Len-1]`（`aiStack_118`）、增广路前驱数组 `prev[0..dim2Len-1]`（`acStack_158 + 0x20`）与访问标记 `visited[0..dim2Len-1]`（`acStack_158`）。
     - 对每个左部顶点 `local_e = 0 .. param_1 - 1`，通过 Dijkstra 式松弛寻找最小残差边 `local_18 = cost[r][c] - u[r] - v[c]`，更新顶标 `u, v` 与 `slack` 直至找到未匹配右部顶点，最后沿 `acStack_158 + 0x20` 回溯翻转增广路匹配关系写入 `param_3`。

8. **`IDT_GetAcc @ 0x6ba61704`**
   - **调用关系**：被 `IDT_AccCheck`、`IDT_AlwaysMatchProcess` 调用。
   - **数学公式**：设归一化时间步长 $\Delta t = \max(1, |\text{nBaselineStageInitialSignal}|)$（在特定模式下使用帧时间戳差），计算当前归一化速度与加速度指标：
     $$v_x = \left\lfloor \frac{x_{\text{cur}} - x_{\text{prev}}}{\Delta t} \right\rfloor, \quad v_y = \left\lfloor \frac{y_{\text{cur}} - y_{\text{prev}}}{\Delta t} \right\rfloor$$
     $$a_x = v_x - v_{x,\text{prev}}, \quad a_y = v_y - v_{y,\text{prev}}$$
     $$\text{AccMetric} = \begin{cases} \lfloor (v_x^2 + v_y^2) / 4 \rfloor, & \text{prev.frameAge} == 1 \\ \min\big(v_x^2 + v_y^2,\; a_x^2 + a_y^2\big), & \text{prev.frameAge} > 1 \end{cases}$$
     注意当 `frameAge > 1` 时取速度平方与加速度平方的较小值，使得高速匀速直线滑动（$a_x^2 + a_y^2$ 极小）和低速转向（$v_x^2 + v_y^2$ 极小）都能顺利通过校验。

9. **`IDT_IsToIncAccThold @ 0x6ba615c7`**
   - **调用关系**：被 `IDT_AccCheck`、`IDT_AlwaysMatchProcess` 调用；调用 `GripFilter_IsPeakAbsGrip`、`Touch_IsException`。
   - **功能**：判断是否需要将加速度门限放宽 4 倍（距离门限放宽 2 倍）。当存在水滴/大面积异常（`Touch_IsException != 0`）、全局抗噪状态 `bField_0x111 >= 2`、或上一帧连续高速移动计数 `prevTouch.+0x4f6 >= 25`（且未被标记为分指/合并禁放宽位 `0x1000000 | 0x2000000`）时返回 `1`。

10. **`IDT_AccCheck @ 0x6ba619a3`**
    - **调用关系**：被 `IDT_Process` 调用；调用 `TS_MMToPixel`、`IDT_GetAcc`、`IDT_IsToIncAccThold`、`GripFilter_IsToCancelIDMapping`。
    - **门限判定与 `W273AS2700` 参数**：
      - 读取基准加速度距离参数 `flash+0x088a`（即 `pTVar1[1].field_0x4a`，在 `W273AS2700` 中为 `16 mm`），调用 `uVar4 = TS_MMToPixel(16)` 并计算基准门限 `thold = (uVar4 >> 4)^2`。
      - 若 `IDT_IsToIncAccThold != 0` 或传入 `bUseStrictAccCheck != 0`（注意：函数参数名虽在注释中写作 `bUseStrictAccCheck`，但实际代码 `if (cVar2 != 0 || param_1 != 0) local_18 <<= 2;` 表示 `param_1 == 1` 时将门限左移 2 位即**放宽为 4 倍**！）。
      - 若 `IDT_GetAcc(curIdx, prevIdx) > local_18`，则判定为跨指错误匹配，将映射设为 `0xff`（断开重分配 ID），并置位 `dwSpecialControlFlags |= 4`。
      - 此外，若 `GripFilter_IsToCancelIDMapping` 返回真或上一帧帶有 `persistFlags1 & 0x200000`（强制断连标志），同样断开映射（设为 `0xff`）。

11. **`IDT_Map @ 0x6ba6122e`**
    - **调用关系**：被 `IDT_Process` 调用。
    - **功能**：先将当前帧所有 `TouchUnit` 的 `touchId`（`+0x154`）清零，再遍历 `prevIdx = 0 .. bPrevTouchCount - 1`，若 `abIdtScratch[prevIdx] != -1`，则将上一帧的 `touchId` 继承赋值给匹配到的当前帧 `TouchUnit`。

---

## 4.6 触控物理尺寸与像素换算模块 (`TS.c`)

`TS`（Touch Size）模块负责将传感器差分信号总和（`sigSum`）以及椭圆长短轴转换为以毫米（mm）为单位的触控尺寸，并提供传感器像素（Q8.8）、物理毫米（mm / 0.1mm）与屏幕显示像素之间的双向定点数换算。

### 4.6.1 函数清单与完整分析 (`TS.c` 共 13 个函数)

1. **`TS_Init @ 0x6ba7c000`**：空函数，由 `TSA_Init` 调用。
2. **`TS_MMToPixel @ 0x6ba7c416`**：将毫米数 `param_1` 转换为内部像素单位：`return g_tsaPrmtFlashConst[0x1a] * param_1`（其中 `g_tsaPrmtFlashConst[0x1a]` 在初始化时由通道间距 Pitch 计算得出，每毫米对应的 Q8.8/16 像素因子）。
3. **`TS_MMToDisplayPixel @ 0x6ba7c436`**：将毫米数 `param_1` 转换为 $\times 16$ 定标显示像素：
   $$\text{dispPixel}_{\times 16} = \left\lfloor \frac{\text{param\_1} \times \max(\text{Dim1Res}, \text{Dim2Res}) \times 16}{\text{Dim1PhysicalLength}_{0.1\text{mm}}} \right\rfloor$$
   在 `W273AS2700` 中，`max(2560, 1600) = 2560`（`flash+0x05a6`），`field_0x2e = 2640`（`264.0 mm`）。
4. **`TS_SQPixelToSQMM @ 0x6ba7c4a0`**：将内部像素距离平方转换为物理面积/距离平方（mm²）：
   $$\text{sqMM} = \left\lfloor \frac{\text{sqPixel} \times C_{1c}^2}{2^{32}} \right\rfloor$$
   其中 `C_1c = *(u16*)(g_tsaPrmtFlashConst + 0x1c)` 为预计算的 `mm / pixel * 65536` 换算常数。
5. **`TS_SQMMToSQPixel @ 0x6ba7c4fb`**：将毫米平方 `param_1` 反算为内部像素距离平方：$\lfloor (\text{param\_1} \times 2^{32} / C_{1c}) / C_{1c} \rfloor$。
6. **`TS_PixelToMM @ 0x6ba7c561`**：将内部像素距离四舍五入转换为整数毫米：$\lfloor (\text{pixel} \times C_{1c}) / 2^{16} + 0.5 \rfloor$。
7. **`TS_PixelToMMScale10 @ 0x6ba7c5a7`**：将带符号内部像素距离四舍五入转换为 `0.1 mm` 单位：$\text{sign}(x) \cdot \lfloor (|x| \times C_{1c} \times 10) / 2^{16} + 0.5 \rfloor$。
8. **`TS_GetSizeInMM @ 0x6ba7c692`**
   - **调用关系**：被 `TS_UpdateCurSizeInMM`、`TS_UpdateCurSizeInMMForUdfp` 调用。
   - **数学模型**：基于基准阈值 `param_1 = *(i16*)g_tsaPrmtDynamic`（当前峰值检测阈值）与单位面积信号系数 `uVar1 = *(u16*)(g_tsaPrmtFlashConst + 0x1e)`，从 $s = 1$ 循环递增至 $15\text{ mm}$，求解满足二次面积-信号关系的整数直径 $s$（mm）：
     $$\text{param\_1} \times C_{1e} \times (s^2 + s) \le \text{sigSum} \times 1024 \implies s \in [1, 15]$$
9. **`TS_UpdateCurSizeInMM @ 0x6ba7c006`**
   - **调用关系**：被 `TS_ProcessUnit` 调用；调用 `TS_GetSizeInMM`。
   - **逻辑**：先由信号量总和计算等效直径 `s = TS_GetSizeInMM(dynamicThold, sigSum)`；若触控椭圆长轴 `majorAxis`（`+0x188`）大于 `(bDim1Pitch >> 3) + 2`（在 `W273AS2700` 中 `bDim1Pitch = 69`，即 `69/8 + 2 = 10`），则直接取几何长轴 `majorAxis`（截断为 `u8`）作为尺寸。
10. **`TS_UpdateCurSizeInMMForUdfp @ 0x6ba7c0d6`**
    - **调用关系**：被 `TS_ProcessUnit` 调用；调用 `TS_GetSizeInMM`、`Misc_SqrtUint32`。
    - **屏下指纹（UDFP）尺寸融合公式**：计算信号量尺寸 $s_{\text{sig}} = \text{TS\_GetSizeInMM}$ 与几何等效圆直径 $s_{\text{geo}} = \lfloor \sqrt{\text{majorAxis} \times \text{minorAxis}} \rfloor$，按 $s_{\text{sig}}$ 大小分段平滑混合：
      $$s_{\text{udfp}} = \begin{cases} s_{\text{sig}}, & s_{\text{sig}} \le 6 \\ \left\lfloor \frac{s_{\text{geo}} \cdot (s_{\text{sig}} - 6) + s_{\text{sig}} \cdot (10 - s_{\text{sig}})}{4} \right\rfloor, & 7 \le s_{\text{sig}} \le 10 \\ s_{\text{geo}}, & s_{\text{sig}} \ge 11 \end{cases}$$
11. **`TS_ProcessUnit @ 0x6ba7c1d7`**
    - **调用关系**：被 `TS_Process` 调用；调用 `TS_UpdateCurSizeInMM`、`TS_UpdateCurSizeInMMForUdfp`。
    - **单调非递减约束**：将 `udfpSizeMM` 写入 `+0x195`；对于常规尺寸 `sizeInMM`（`+0x194`），取当前计算值与上一帧历史值 `prevTouch.sizeInMM` 的较大者（即单次按压周期内 `sizeInMM` 只增不减，防止抬起瞬间面积骤降导致 `CTD_ECGetOffset` 查表跳变）；若两者皆为 0 则回退至全局最近抬起尺寸 `g_tsaPrmtDynamic[0x6c]`。
12. **`TS_Process @ 0x6ba7c2cb`**
    - **调用关系**：被 `TSA_Processing @ 0x6ba9a443` 调用；遍历所有非确认抬起（`teState & 0x20 == 0`）的触控点调用 `TS_ProcessUnit`。
13. **`TS_PostProcess @ 0x6ba7c368`**
    - **调用关系**：被 `TSA_Processing` 在帧后处理阶段调用；当触控点确认抬起（`teState & 0x20 != 0`）时，将其上一帧尺寸钳位至 $\le 9\text{ mm}$ 后存入 `g_tsaPrmtDynamic[0x6c]`。

---

## 4.7 触控事件状态机与消抖模块 (`TE.c` & `DB.c`)

`TE`（Touch Event）与 `DB`（Debounce）模块共同维护每个触控点的生命周期状态机（`teState` @ `+0x21c`）：
- `0x01`：`TE_TOUCH_DOWN_DEBOUNCE`（新按下消抖等待中，不对外报点）
- `0x02`：`TE_TOUCH_DOWN`（刚通过消抖，首次确认按下）
- `0x04`：`TE_MOVE`（持续稳定按压/移动）
- `0x08`：`TE_LIFT_OFF_DEBOUNCE`（信号消失后的抬起消抖保持中，克隆上一帧坐标继续维持在表中）
- `0x10`：`TE_LIFT_OFF_HOLD`（快速点击 FastClick 或边缘滑出 MoveOut 触发的额外一帧报点保持）
- `0x20`：`TE_LIFT_OFF`（确认抬起，本帧完成后从历史表中释放）
- `0x40`：`TE_WINDOWS_PAD_PRE_LIFTOFF`（Windows Precision Touchpad 模式下抬起前的倒数第二帧保持状态）

### 4.7.1 `TE.c` 触控事件状态机 (共 10 个函数)

1. **`TE_Init @ 0x6ba77530`**：空函数，由 `TSA_Init` 调用。
2. **`TE_GetNextID @ 0x6ba77536`**
   - **调用关系**：被 `TE_TouchDownProcess` 调用；调用 `PrevTouch_IsValidId`。
   - **ID 分配策略**：基于静态变量 `prevOccupiedID_10025` 从 `1..20` 循环递增查找既不在 `PrevTouch` 活跃表中、也未被当前帧其他 `TouchUnit` 占用的最小可用 ID（范围 `1..20`，满载返回 `0`）。
3. **`TE_Process @ 0x6ba789d9`**
   - **调用关系**：被 `TSA_Processing @ 0x6ba9a443` 在第 17 步调用；调用 `Touch_GetIdxBasedOnPreIdx`、`TSA_IsWindowsPadFeatureEnabled`、`TE_LiftOffProcess`、`TE_WindowsLiftOffProcess`、`TE_TouchDownProcess`、`PrevTouch_GetIdxBasedOnCurIdx`、`TE_MoveProcess`、`TE_TouchDownDebounce`、`TE_AssertProcess`、`Touch_Clean`、`PrevTouch_Release`、`Touch_ProcessAftTouchEvent`、`TSAStatic_ProcessAftTouchEvent`。
   - **四阶段核心流水线**：
     1. **失配历史点抬起处理**：遍历上一帧所有 `prevIdx = 0 .. bPrevTouchCount - 1`，若 `Touch_GetIdxBasedOnPreIdx(prevIdx) == 0x14`（当前帧无匹配点），根据 `TSA_IsWindowsPadFeatureEnabled()` 选择调用 `TE_WindowsLiftOffProcess` 或 `TE_LiftOffProcess`（若处于抬起消抖期，会将该历史点克隆追加到当前帧 `g_touchesPtr` 末尾！因此必须在第一步提前保存原始 `bCurTouchCount`）。
     2. **当前帧触控点状态推進**：遍历原始 `bCurTouchIndex = 0 .. bCurTouchCount - 1`：
        - 若 `touchId == 0`（无匹配的新点）：调用 `TE_TouchDownProcess` 分配新 ID 并在 `g_prevTouchesPtr` 末尾创建对应历史槽位；
        - 若已有 `touchId` 且上一帧状态 `prev.teState & 1 != 0`（仍在按下消抖期）：调用 `TE_TouchDownDebounce`；
        - 若已有 `touchId` 且上一帧已确认按下（`prev.teState & 1 == 0`）：调用 `TE_MoveProcess`。
     3. **双射一致性断言自检**：调用 `TE_AssertProcess()` 检查 `Touch` 与 `PrevTouch` 数量及 `curIdx <-> preIdx` 映射是否为严格双射；若发生错位异常，立即调用 `Touch_Clean()` 与 `PrevTouch_Release()` 强制复位全部触控状态，防止死锁鬼点。
     4. **事件后同步更新**：在互容激活状态下调用 `Touch_ProcessAftTouchEvent()`（执行 `CTD_ECProcess`、更新峰值极值、运动学差分及 `HistoryQueue`）与 `TSAStatic_ProcessAftTouchEvent()`。
4. **`TE_TouchDownProcess @ 0x6ba779f3`**
   - **调用关系**：被 `TE_Process` 调用；调用 `TE_GetNextID`、`PrevTouch_UnitCopy`、`TE_TouchDownDebounce`。
   - **逻辑**：分配新 `touchId`，在 `PrevTouch` 末尾分配槽位 `bVar1 = bPrevTouchCount++`，拷贝当前点数据，初始化起点坐标 `startEcPos`（`+0x496, +0x498`）、平滑 $Z_9$ 初值（`+0x1a2`）、双向索引映射（`prev.matchedCurIdx = curIdx`, `cur.matchedPrevIdx = bVar1`），并立即调用 `TE_TouchDownDebounce(curIdx, bVar1)` 执行第 0 帧消抖判定。
5. **`TE_TouchDownDebounce @ 0x6ba7772b`**
   - **调用关系**：被 `TE_Process`、`TE_TouchDownProcess` 调用；调用 `Touch_UnitUpdate`、`DB_TouchDownNum`、`PrevTouch_UnitUpdate`。
   - **逻辑**：更新 `Touch_UnitUpdate` 后，调用 `uVar1 = DB_TouchDownNum(prevIdx, curIdx)` 获取当前所需的消抖帧数。若已累计消抖帧数 `prev.debounceCounter < uVar1`，则递增 `debounceCounter` 并保持 `teState = 1`；否则消抖通过，跃迁至 `teState = 2`（TouchDown Confirmed）并置位 `persistFlags2 |= 0x400`。
6. **`TE_MoveProcess @ 0x6ba775fa`**
   - **调用关系**：被 `TE_Process` 调用；调用 `Touch_UnitUpdate`、`PrevTouch_UnitUpdate`。
   - **逻辑**：清零 `debounceCounter`（`+0x4e4 = 0`），设置 `teState = 4`（Move），递增帧龄 `frameAge`（`+0x156`），更新持续毫秒数 `durationMs`（`+0x158`），并同步更新 `PrevTouch`。
7. **`TE_LiftOffProcess @ 0x6ba78511`**
   - **调用关系**：被 `TE_Process` 调用；调用 `DB_LiftOffNum`、`Touch_UnitCopy`、`TE_LiftOffReportProcess`、`PrevTouch_UnitUpdate`。
   - **逻辑**：常规模式抬起处理。调用 `uVar1 = DB_LiftOffNum(prevIdx)` 获取抬起消抖帧数；若 `debounceCounter < uVar1`，递增计数并将该 `PrevTouch` 克隆追加到当前帧 `g_touchesPtr` 末尾，设 `teState = 8`（LiftOff Debounce）；否则调用 `TE_LiftOffReportProcess` 进入确认抬起。
8. **`TE_WindowsLiftOffProcess @ 0x6ba78033`**
   - **调用关系**：被 `TE_Process` 调用；调用 `DB_LiftOffNum`、`Touch_UnitCopy`、`Touch_IsException`、`TE_LiftOffReportProcess`、`PrevTouch_UnitUpdate`。
   - **逻辑**：Windows 精确触控板模式下的抬起处理（`W273AS2700` 默认启用）。相比 `TE_LiftOffProcess` 增加了一拍 `teState = 0x40`（或针对未完成消抖的 FastClick 设为 `teState = 0x10` 并回退消抖计数以补报点击），在下一帧才进入 `TE_LiftOffReportProcess`。
9. **`TE_LiftOffReportProcess @ 0x6ba77d4b`**
   - **调用关系**：被 `TE_LiftOffProcess`、`TE_WindowsLiftOffProcess` 调用；调用 `Touch_UnitCopy`、`Touch_IsException`、`TSA_IsWindowsPadFeatureEnabled`。
   - **逻辑**：将失配的历史点克隆追加到当前帧 `g_touchesPtr` 末尾。若该点带有快速点击标志（`persistFlags1 & 0x40000`）、此前仍处于按下消抖（`prev.teState == 1`）、非异常且非 WindowsPad 模式，则设 `teState = 0x10`（补报一帧点击）；否则设 `teState = 0x20`（最终确认抬起）。同时统计高幅值短促点击次数 `wField_0x122`（上限 5）。
10. **`TE_AssertProcess @ 0x6ba7872d`**
    - **调用关系**：被 `TE_Process` 调用。
    - **逻辑**：严格校验 `bPrevTouchCount == bTouchCount`，且每个 `prevIdx` 通过 `Touch_GetIdxBasedOnPreIdx` 映射到唯一的 `curIdx`（覆盖 `0 .. count-1`），每个 `curIdx` 通过 `PrevTouch_GetIdxBasedOnCurIdx` 映射到唯一的 `prevIdx`。任何一项违背即返回 `1` 触发全局清理。

### 4.7.2 `DB.c` 动态按下/抬起消抖策略 (共 7 个函数)

1. **`DB_LOSize @ 0x6ba55e20`**
   - **调用关系**：被 `Touch_UpdTouchPrptBefTracking` 调用。
   - **公式**：计算抬起阶段允许的最大网格面积 `area = (flash->field_0x836)^2`（若上一帧存在抬起事件则缩减为 `3/4`）。在 `W273AS2700` 中 `flash+0x0836 = 0`。
2. **`DB_LiftOffNum @ 0x6ba55e79`**
   - **调用关系**：被 `TE_LiftOffProcess`、`TE_WindowsLiftOffProcess` 调用。
   - **逻辑**：若该触控点从未对外报点（`persistFlags1 & 8 == 0`）或已激活 `HistoryBuffer` 补点（`persistFlags1 & 0x80 != 0`），抬起消抖帧数为 `0`（立即抬起）；否则若为大尺寸手掌（`tzType == 3`）需消抖 `3` 帧，若带有 `volatileFlags1 & 0x10000000` 或全局噪声等级 `bField_0x111 > 2` 需消抖 `2` 帧，常规手指为 `0` 帧。
3. **`DB_TouchDownTholdCheck @ 0x6ba55f8d`**
   - **调用关系**：被 `DB_TouchDownNum` 调用；调用 `TSA_IsGameScenario`、`TouchThold_ToeUseSpecialThold`。
   - **逻辑**：读取动态按下峰值门限 `thold = *(i16*)(g_tsaPrmtDynamic + 0xc)`（在低功耗非游戏场景下若上一帧曾低于门限则提升至 `1.5 * thold`；特定机种边缘点使用 `+0x0e` 门限）。若关联 Peak 的信号值 `peakSig < thold`，置位 `volatileFlags2 |= 0x20` 并返回 `false`。
4. **`DB_FastClickProcess @ 0x6ba562d4`**
   - **调用关系**：被 `DB_TouchDownNum` 调用。
   - **逻辑**：当按下消抖帧数 `< 4`、帧间位移平方 `< 31`（`0x1f`）、信号不均匀度 `+0x1a6 < 11`、长轴 `< 1.6 mm`（`0x10`）且信号量呈现显著上升或下降沿（`volatileFlags1 & 1` 或 `& 2`）时，置位快速点击标志 `persistFlags1 |= 0x40000`（允许在手指极快抬起、尚未走完常规消抖时补报一次点击）；若消抖超过 5 帧则清除该标志。
5. **`DB_GetNumBySignalSum @ 0x6ba565b2`**
   - **调用关系**：被 `DB_TouchDownNum` 调用；调用 `Touch_ValidComparison`。
   - **逻辑**：若 Peak 信号和低于 `(g_tsaPrmtDynamic[6] * *(i32*)(g_tsaPrmtDynamic + 0x14)) >> 4`，则至少要求 `1` 帧消抖；且若相邻两帧 `sigSum` 暴涨超过上一帧的 `25%`（`curSigSum - prevSigSum > prevSigSum >> 2`），将消抖帧数提高到至少 `3` 帧以抑制突变干扰。
6. **`DB_GetNumBySideTouch @ 0x6ba5674c`**
   - **调用关系**：被 `DB_TouchDownNum` 调用。
   - **逻辑**：非低功耗下若长轴 `majorAxis > 14`（`1.4 mm`）要求至少 `2` 帧消抖，若未进入核心区（`volatileFlags1 & 4 == 0`）要求至少 `1` 帧消抖；低功耗下若 `majorAxis > 29` 要求至少 `2` 帧消抖。
7. **`DB_TouchDownNum @ 0x6ba5681c`**
   - **调用关系**：被 `TE_TouchDownDebounce` 调用；调用 `DB_TouchDownTholdCheck`、`DB_GetNumBySignalSum`、`DB_GetNumBySideTouch`、`DB_FastClickProcess`、`CMF_GetNoiseValue`。
   - **综合消抖帧数决策**：
     - 基础帧数：若本帧 Peak 总数比上一帧激增超过 1 个，初值设为 `1`，否则为 `0`。
     - 根据异常与环境标志逐级上调：水滴/噪声位（`+0x1ec & 0x80` 或 `bField_0x111 != 0` 或 `volatileFlags1 & 0x800`）$\to \ge 2$；合并/大面积（`tzType == 2` 或 `3`）或特定边缘标志 $\to \ge 3$。
     - **硬性否决（返回 `0xffff` 禁止通过消抖）**：若 `DB_TouchDownTholdCheck` 失败、或带有严重异常位（`+0x1ec & 0x40`、`+0x1f0` 的 `bit 10 / 24 / 25 / 30 / 31`、`+0x1f8 & 0x2000`、`+0x1f0 & 0x400000`），返回 `0xffff` 并置位 `volatileFlags1 |= 0x10`（且在下一帧恢复正常时将 `debounceCounter` 清零重新计数！）。
     - 若通过校验，再结合 `reqDebounceNum`（`+0x238`）、共模噪声 `CMF_GetNoiseValue`（若局部噪声 `> 500` 或 `< -400`，提升至 `flash+0x0818`；在 `W273AS2700` 中 `flash+0x0818 = 0`）确定最终消抖帧数。

---

## 4.8 动态触控阈值计算模块 (`TouchThold.c`)

`TouchThold` 模块根据当前触控模式（手指/手套/手写笔/皮套）、共模噪声水平与大面积干扰点数量，实时计算峰值门限、噪声门限与按下门限（存入 `g_tsaPrmtDynamic` 与 `g_tsaPrpt`）。

### 4.8.1 函数清单与完整分析 (`TouchThold.c` 共 12 个函数)

1. **`TouchThold_Init @ 0x6ba7c922`**
   - **调用关系**：被 `TSA_Init` 调用；调用 `TouchThold_GetTholdBasedOnSNR`。
   - **功能与 `W273AS2700` 参数**：用 `flash+0x0910`（`noiseBase2 = 200`）计算初始 SNR 门限存入 `g_tsaPrpt[1].field_0x16`（`200 * 20 >> 4 = 250`）；将 `wPeakMode3ThresholdMax`（`flash+0x05b6 = 600`）填入 `g_tsaPrpt[1].field_0x14..0x22`；将 `bScanRate >> 2`（`100 >> 2 = 25`）存入 `field_0x20`。
2. **`TouchThold_GetTholdBasedOnSNR @ 0x6ba7c776`**
   - **数学公式**：读取 `snrScale = *(u8*)(flash + 0x0914)`（在 `W273AS2700` 中为 `20` = `0x14`），计算：
     $$\text{thold}_{\text{SNR}} = \left\lfloor \frac{\text{snrScale} \times \text{noiseBase}}{16} \right\rfloor = \left\lfloor \frac{20 \times \text{noiseBase}}{16} \right\rfloor = 1.25 \times \text{noiseBase}$$
3. **`TouchThold_GetNoisyTouchNum @ 0x6ba7c896`**
   - **逻辑**：遍历 `PrevTouch`，统计信号总和 `sigSum > *(i16*)(flash + 0x0912)`（在 `W273AS2700` 中为 `6000` = `0x1770`）的大信号触控点个数。
4. **`TouchThold_GetTholdBasedOnTouchMode @ 0x6ba7c7a5`**
   - **逻辑**：在纯手指模式下返回 $\lfloor \text{param\_1} / 2 \rfloor$；若激活了手套（`bit 2`）、手写笔（`bit 3`）、皮套（`FEAT_SMART_COVER`）或 In-cell 信号失配标志，则返回 $\lfloor \text{param\_2} \times 4 / 5 \rfloor$。
5. **`TouchThold_GetNoiseThold @ 0x6ba7ca48`**
   - **调用关系**：被 `TouchThold_Process` 调用；调用 `TouchThold_GetNoisyTouchNum`、`TouchThold_GetTholdBasedOnSNR`、`TouchThold_GetTholdBasedOnTouchMode`、`TSAStatic_IsSafeBaselineApplied`。
   - **多级抗噪门限**：根据大信号触控点数 `bVar1 = TouchThold_GetNoisyTouchNum()` 选择噪声基数 `local_c`：
     - `0` 个：`flash+0x090c` = `100` $\implies \text{thold}_{\text{SNR}} = 125$；
     - `1` 个：`flash+0x090e` = `150` $\implies \text{thold}_{\text{SNR}} = 187$；
     - $\ge 2$ 个：`flash+0x0910` = `200` $\implies \text{thold}_{\text{SNR}} = 250$。
     若已应用安全基线（`TSAStatic_IsSafeBaselineApplied != 0`），则与 `TouchThold_GetTholdBasedOnTouchMode` 取较大值。
6. **`TouchThold_BasedOnExperience @ 0x6ba7cb32`**
   - **逻辑**：仅当 `dwPeakProcessingMode != 3` 时生效（在 `W273AS2700` 中 `dwPeakProcessingMode == 3`，故直接透传返回 `param_1`）。
7. **`TouchThold_GetPeakThold @ 0x6ba7cee9`**
   - **逻辑**：根据当前工作模式在常规门限 `param_1` 与高灵敏度/手套门限 `param_2` 之间切换；若 `dwPeakProcessingMode == 3` 且存在 `bFullIncellSignalDisparity`，则折半为 `param_1 / 2`；特定旧机种在灵敏度模式下固定为 `360`（`0x168`）或 `420`（`0x1a4`）。
8. **`TouchThold_GetTouchDownThold @ 0x6ba7cca5`**
   - **调用关系**：被 `TouchThold_Process` 调用（反编译脚本中因函数体分支或超时未生成伪代码，结合调用上下文 `uVar2 = TouchThold_GetTouchDownThold((int)*param_4); *param_5 = uVar2;` 及 `g_tsaPrmtDynamic + 0x0c` 的使用可知：该函数以当前综合噪声门限 `*param_4` 为输入，按比例缩放并钳位生成新按下确认所需的峰值阈值 `*param_5`，写入 `g_tsaPrmtDynamic + 0x0c` 供 `DB_TouchDownTholdCheck` 使用）。
9. **`TouchThold_Process @ 0x6ba7d0ee`**
   - **调用关系**：被 `TSA_Processing` / 动态参数更新模块调用；调用 `TouchThold_GetPeakThold`、`TouchThold_BasedOnExperience`、`TouchThold_GetNoiseThold`、`TouchThold_GetTouchDownThold`。
   - **输出**：计算峰值检测门限写入 `*param_3`（若单指非稳定状态则乘以 `3/4`），计算噪声门限写入 `*param_4`（取模式基准、`TouchThold_GetNoiseThold` 与 `*param_3` 三者最大值），最后调用 `TouchThold_GetTouchDownThold(*param_4)` 写入 `*param_5`。
10. **`TouchThold_IsSwitching @ 0x6ba7c90e`**：返回阈值切换过渡标志 `g_tsaPrpt[1].field_0x21`。
11. **`TouchThold_ToeUseSpecialThold @ 0x6ba7cd8f`**：检查参数名（`TSA_GetPrmtName()`）前缀是否为 `"P116"`、`"P150"` 或 `"P186"`（对于 `W273AS2700` 返回 `0`）。
12. **`TouchThold_UseHighTholdAtSensitivityMode @ 0x6ba7ce03`** 与 **`TouchThold_SOFInSmartCoverMode @ 0x6ba7ceb3`**：检查特定机种名前缀（`"P116"/"P150"/"P172"/"P173"/"P186"`，对 `W273AS2700` 返回 `0`）及皮套 CMF 方法是否为 1（在 `W273AS2700` 中 `dwCmfMethodNormal = 2`，返回 `0`）。

---

## 4.9 当前帧触控管理与属性更新模块 (`Touch.c` & `PrevTouch.c`)

`Touch.c`（62 个函数）与 `PrevTouch.c`（20 个函数）管理 `g_touchesPtr` 与 `g_prevTouchesPtr` 的生命周期、跨帧拷贝、运动学特征计算、异常形状识别、信号跌落比值统计、可信度评估以及 `HistoryBuffer` / `HistoryQueue` 历史轨迹队列。

### 4.9.1 `Touch.c` 完整函数清单与逆向解析 (共 62 个函数)

#### A. 初始化、内存管理与跨帧索引映射 (8 个函数)
1. **`Touch_Init @ 0x6ba6f503`**：被 `TSA_Init` 调用，内部直接调用 `Touch_Clean`。
2. **`Touch_Clean @ 0x6ba6f49c`**：调用 `memset_s(g_touchesPtr, 0x1d60, 0, 0x1d60)` 清空整个 `TouchTable`（`0x140 + 20 * 0x168 = 0x1d60` = 7520 字节）。
3. **`Touch_UnitCopy @ 0x6ba74462`**：将 `PrevTouch[param_2]` 的前 `0x144` 字节拷贝至 `Touch[param_1]`，同时保留目标 `Touch[param_1]` 已计算的 `maxSigSum`（`+0x1d4`）、信号比值（`+0x1dc..+0x1de`）与 `persistFlags3`（`+0x200`）。
4. **`Touch_UnitRemove @ 0x6ba763f4`**：从 `g_touchesPtr` 中移除索引为 `param_1` 的触控单元，将后续单元依次前移 `0x168` 字节并递减 `bTouchCount`。
5. **`Touch_AssignPreIdxBasedOnID @ 0x6ba73fc7`**：遍历当前帧所有 `TouchUnit`，按 `touchId`（`+0x154`）在 `PrevTouch` 中查找相同 ID 的历史槽位，将其索引写入 `matchedPrevIdx`（`+0x287`，未找到则写 `0x14`）。
6. **`Touch_GetIdxBasedOnPreIdx @ 0x6ba74433`**：返回 `PrevTouch[param_1].matchedCurIdx`（`+0x4f2`）。
7. **`Touch_KeepPrevTouchWithExceptionExcluded @ 0x6ba766b9`**：当本帧处于特殊保持状态时，将 `PrevTouch` 表头状态字段、`Thenar` 信息（调用 `Thenar_UnitCopy`）及全部 `PrevTouchUnit` 完整恢复至当前帧 `g_touchesPtr`，并调用 `HardwareAnalyzer_ReportToTSA`。
8. **`Touch_ThenarFeatureReset @ 0x6bb30bf0`**：清零 `g_touchesPtr` 表头中的大鱼际计数 `+0x130` 及特征字段 `+0x68..+0x70`。

#### B. 几何距离、时间间隔与边缘区域计算 (14 个函数)
9. **`Touch_GetFormattedTimeInterval @ 0x6ba76329`**：格式化帧间时间差 `dt`（单位：ms）：
   - 若输入 `param_1 == 0`，返回标称帧周期 `1000 / bScanRate`（`W273AS2700` 中 `bScanRate = 100 Hz`，即 `10 ms`；若为 0 则返回 `8 ms`）。
   - 若 `param_1 > 0`：非刚退出 IDLE 时钳位至 `[8, 1000] ms`。
   - 若 `param_1 < 0`：非刚进入 IDLE 时钳位至 `[-1000, -8] ms`。
10. **`Touch_GetDim1EdgeDist @ 0x6ba72a59`**：计算 `dim1Pos` 到左右逻辑边界（`g_tsaPrmtFlashConst+0x2a = 0` 与 `+0x2c = 15104`）的最小距离：$\min(\text{pos} - \text{Near}, \text{Far} - \text{pos})$。
11. **`Touch_GetDim2EdgeDist @ 0x6ba72ab9`**：计算 `dim2Pos` 到上下逻辑边界（`g_tsaPrmtFlashConst+0x2e = 0` 与 `+0x30 = 9984`）的最小距离。
12. **`Touch_SetEdgeDist @ 0x6ba72b19`**：调用 `Touch_GetDim1EdgeDist` 与 `Touch_GetDim2EdgeDist`，将 `ecPos` 到逻辑边界的距离写入 `+0x22c` 与 `+0x22e`。
13. **`Touch_GetDistToLogicEdge @ 0x6ba73009`**：根据逻辑边枚举 `param_1`（`1`=Dim1Near，`2`=Dim1Far，`4`=Dim2Near，`8`=Dim2Far）返回指定坐标到该边的有符号距离。
14. **`Touch_GetSQDistPixel @ 0x6ba73447`**：计算两点间传感器像素欧氏距离平方 $\Delta x^2 + \Delta y^2$（带无符号溢出饱和 `0xffffffff`）。
15. **`Touch_GetSQDistPixelToEdge @ 0x6ba7329a`**：沿向量 $(x_2 - x_1, y_2 - y_1)$ 的反方向步进外推直至触达逻辑边界 $(x_{\text{edge}}, y_{\text{edge}})$，再调用 `Touch_GetSQDistPixel` 返回起点 $(x_1, y_1)$ 到边界交点的像素距离平方（供 `ER_MoveInCheck` 使用）。
16. **`Touch_GetSQDistMM @ 0x6ba734be`**：先调用 `Touch_GetSQDistPixel` 再调用 `TS_SQPixelToSQMM`，返回两点间物理距离平方（mm²）。
17. **`Touch_GetSQDistMMInBetween @ 0x6ba73515`**：调用 `Touch_GetSQDistMM` 计算两个 `TouchUnit` 之间的 `ecPos` 物理距离平方（mm²）。
18. **`Touch_GetSQDistMMFromPrevPos @ 0x6ba735c9`**：计算当前帧与上一帧 `ecPos` 的物理距离平方（mm²），写入 `+0x240`。
19. **`Touch_GetSQAccMMFromPrevPos @ 0x6ba736aa`**：计算 `ecPos` 的二阶差分平方 $((x_{\text{ec}} - x_{\text{prev,ec}} - \Delta x_{\text{prev,ec}})^2 + (y_{\text{ec}} - y_{\text{prev,ec}} - \Delta y_{\text{prev,ec}})^2)$ 并通过 `TS_SQPixelToSQMM` 转为 mm²，写入 `+0x242`。
20. **`Touch_GetSQDistPixelFromPrevPos @ 0x6ba7381e`**：计算当前帧与上一帧 `ecPos` 的传感器像素距离平方，写入 `+0x244`。
21. **`Touch_GetSQDistMMFromStartPos @ 0x6ba738ff`**：计算当前帧 `ecPos` 与初始按下位置 `startEcPos`（`prev.+0x496, +0x498`）的物理距离平方（mm²），写入 `+0x24c`。
22. **`Touch_GetRptSQDistMMFromStartPos @ 0x6ba739e1`**、**`Touch_GetRptSQDistMMFromPrevPos @ 0x6ba73ac1`**、**`Touch_GetRptSQDistDisplayPixelFromPrevPos @ 0x6ba73ba1`**、**`Touch_GetFltSQDistPixelFromPrevPos @ 0x6ba73cc1`**（共 4 个报点距离函数，加上文共 17 个几何函数）：分别基于最终报点坐标 `rptPos`（`+0x170, +0x172`）计算距首次报点 `firstRptPos`（`+0x49a, +0x49c`）的 mm²（写入 `+0x250`）、距上一帧 `rptPos` 的 mm²（写入 `+0x252`）、距上一帧显示像素的距离平方（写入 `+0x254`）以及传感器像素距离平方（写入 `+0x258` / `+600`）。

#### C. 单元状态、标志位与属性更新 (20 个函数)
26. **`Touch_UnitUpdate @ 0x6ba760da`**：在 `TE_MoveProcess` 与 `TE_TouchDownDebounce` 中调用的核心单元更新入口：
    - 对中心九宫格信号 $Z_9$ 进行一阶 IIR 平滑：$\text{smoothZ9} = \lfloor (7 \cdot \text{prev.smoothZ9} + \text{cur.z9}) / 8 \rfloor$（写入 `+0x1a2`）；
    - 更新 `dt`（`+0x150`）与 `prevDt`（`+0x152`）；
    - 依次调用 `Touch_GetSQDistMMFromPrevPos`、`Touch_GetSQDistPixelFromPrevPos`、`Touch_GetSQDistMMFromStartPos`、`Touch_GetSQAccMMFromPrevPos`、`Touch_SetEdgeFlag`、`Touch_SetRegion`、`Touch_SetVolatileFlag`、`Touch_SetPersistFlag`、`Touch_SetSuppressFlag`、`Touch_UnitUpdateMaxMin`、`Touch_UnitUpdateSqDistMMFromStartPosMax`、`Touch_UpdateGlobalState`。
27. **`Touch_SetRegion @ 0x6ba72bce`**：根据显示坐标 `(dispX, dispY)` 所在屏幕象限设置 `+0x220`（左上=`0`，左下=`1`，右上=`2`，右下=`3`）。
28. **`Touch_SetEdgeFlag @ 0x6ba72d1c`**：调用 `Peak_IsOnInnerEdgeDim1Near/Far`、`Peak_IsOnInnerEdgeDim2Near/Far` 与 `Peak_IsOnOuterEdgeDim1Near/Far`、`Peak_IsOnOuterEdgeDim2Near/Far`，在 `edgeFlags`（`+0x204`）置位内边缘（`0x100..0x800`）与外边缘（`0x1000..0x8000`）标志。
29. **`Touch_SetSuppressFlag @ 0x6ba72ffb`**：预留空函数。
30. **`Touch_SetVolatileFlag @ 0x6ba70fdd`**：汇总计算单帧标志位 `+0x1f0` 与 `+0x1f4`：
    - 调用 `Touch_SetVolatileFlagSigSum`、`Touch_SetVolatileFlagTouchAxisAbnormal`；
    - 检查峰值比 `< 24/32`（置位 `0x80000`）、单帧位移 `> 100 mm²`（置位 `0x80`）、握持比 `> 7`（置位 `+0x1fc |= 8`）；
    - 检查 Peak 邻域信号和 `< peakSig / 32`（即强负值环绕，置位 `0x80000000` 及 `dwSpecialControlFlags |= 0x40`）；
    - 依次调用 `Touch_NeedNextEdgeLowThresh`、`Touch_SetVolatileFlagFingerZFailed`、`Touch_SetVolatileFlagTouchDownZFailed`、`Touch_SetVolatileFlagLiftOffZFailed`、`Touch_SetVolatileFlagStableTouch`、`Touch_UpdateStableProperty`、`BLReset_CheckAbnormalTouch`、`Touch_GetPeakSumPosSig`、`Touch_UpdateHandEnum`。
31. **`Touch_SetVolatileFlagSigSum @ 0x6ba6f7af`**：计算当前帧与上一帧的 `sigSum` 比值（`+0x1dc`）、Peak $Z_4$ 比值（`+0x1dd`）与 Peak 峰值比值（`+0x1de`），定标因子为 $\times 32$（上限 `255`）。若 `sigSumRatio < 24`（跌落超过 25%）置位 `volatileFlags1 |= 2`；若 `sigSumRatio > 40`（增长超过 25%）置位 `volatileFlags1 |= 1`。
32. **`Touch_SetVolatileFlagTouchAxisAbnormal @ 0x6ba6fc6a`**：当 $Z_4$ 比值 `< 24`（置位 `0x40000`）、长轴比上一帧暴增超过 `4/3` 倍、长轴 `> 2.5 mm`（`0x19`）且峰值 `< 1.5 * tdThold` 时，置位 `volatileFlags2 |= 0x40`（轴向异常）。
33. **`Touch_NeedNextEdgeLowThresh @ 0x6ba6fe70`** 与 **`Touch_ToeUseSpecialZ4TholdEdge @ 0x6ba6fe1a`**：当机种名为 `"P116"/"P150"/"B206"/"B20Y"` 或动态标志 `g_tsaPrmtDynamic[0x1d/0x1e]` 开启且 Peak 位于次外圈/最外圈时返回 `1`，允许将边缘 $Z_4$ 门限降低至 `3/4`。
34. **`Touch_SetVolatileFlagFingerZFailed @ 0x6ba700e1`**：若 Peak 的四邻域信号和 $Z_4$（`peak+0x20`）小于 `*(i32*)(g_tsaPrmtDynamic + 0x14)`（皮套模式下折半），置位 `volatileFlags1 |= 0x800`（Finger $Z_4$ 不足，将在 Finger 模式下被抑制）。
35. **`Touch_SetVolatileFlagTouchDownZFailed @ 0x6ba701c3`**：若 Peak $Z_4$ 小于按下门限 `*(i32*)(g_tsaPrmtDynamic + 0x10)`（边缘放宽时 $\times 3/4$），置位 `volatileFlags1 |= 0x400`；若小于最低门限 `*(i32*)(g_tsaPrmtDynamic + 0x18)`，置位 `0x40000000`（两者均会阻止通过按下消抖）。
36. **`Touch_SetVolatileFlagLiftOffZFailed @ 0x6ba70356`**：若 Peak $Z_4$ 或峰值低于抬起保持门限（常规为按下门限的 `3/4` 与历史最大值的一半中的较小者），置位 `volatileFlags2 |= 1`。
37. **`Touch_SetVolatileFlagStableTouch @ 0x6ba70775`**：当 Peak $Z_4 > 3 \times \text{thold}$ 时，置位稳定触控标志 `volatileFlags1 |= 0x1000`。
38. **`Touch_UpdateStableProperty @ 0x6ba708d8`**：在按下前 10 帧（`frameAge < 11`）或移动超过 $25\text{ mm}^2$（`5 mm`）时更新稳定锚点 `+0x180..+0x186`；若在水滴异常下长轴剧烈变化超过位移的 $\sqrt{1/2}$ 倍，置位 `volatileFlags1 |= 0x200000`。
39. **`Touch_GetPeakSumPosSig @ 0x6ba6f641`**：维护单次触控周期内的历史最大正信号和 `maxSigSum`（`+0x1d4`）及对应时刻的椭圆倾角 `maxSigAngle`（`+0x193`）。
40. **`Touch_UpdateHandEnum @ 0x6ba70df0`**：调用 `TSAOut_Angle` 转换 `maxSigAngle`（`+0x193`）：若角度在 $(15^\circ, 75^\circ)$ 之间判定为左手（`+0x1e4 = 4`），若在 $(-75^\circ, -15^\circ)$ 之间判定为右手（`+0x1e4 = 8`），否则继承上一帧状态。
41. **`Touch_IsPeakOnInnerDim1Edge @ 0x6ba70f56`**：判断 Peak 列坐标是否等于 `1` 或 `bCols - 2`（次外圈列）。
42. **`Touch_SetPersistFlag @ 0x6ba72776`**：更新并按位或继承跨帧持久标志 `+0x1f8, +0x1fc, +0x200, +0x214`：
    - 核心区标志（`+0x1f8 |= 1`）、移动超手套激活距离 `sqDistFromStart >= (g_tsaPrmtDynamic[0x88])^2`（`+0x1f8 |= 4`）、曾稳定标志（`+0x1f8 |= 0x200`）；
    - 对已报点（`prev.+0x1f8 & 8`）依次调用 `Touch_SetVolatileFlagFalseFlickDetected`、`Touch_SetPersistFlagMoveOverClickSuppressRange`、`Touch_SetPersistFlagMoveOverMaxClickSuppressRange`、`Touch_SetPersistFlagMoveOverFlickDistance`。
43. **`Touch_SetVolatileFlagFalseFlickDetected @ 0x6ba71f3a`**：当帧间像素位移平方超过 $(1\text{ mm})^2 / 2$，但位移平方小于长短轴变化量平方和的 `1/4`（说明质心移动主要由接触面积膨胀/收缩引起而非真实手指滑动）时，将滑动基准锚点（`+0x4ac, +0x4ae`）随质心平移，并置位伪滑动标志 `volatileFlags1 |= 0x100000`。
44. **`Touch_SetPersistFlagMoveOverClickSuppressRange @ 0x6ba723e9`**、**`Touch_SetPersistFlagMoveOverMaxClickSuppressRange @ 0x6ba72518`**、**`Touch_SetPersistFlagMoveOverFlickDistance @ 0x6ba72647`**：分别检查当前 `ecPos` 距锚点（`+0x4a4`、`+0x49a`、`+0x4ac`）的像素距离平方是否超过动态阈值 `g_tsaPrmtDynamic[0x78]`（来自 `flash+0x0904 = 20`）、`g_tsaPrmtDynamic[0x7a]`（来自 `flash+0x0906 = 60`）、`g_tsaPrmtDynamic[0x7c]`（来自 `flash+0x0908 = 50`），超过则分别置位 `persistFlags1` 的 `0x100`、`0x8000`、`0x1000`。
45. **`Touch_UnitUpdateMaxMin @ 0x6ba75d1a`**、**`Touch_UnitUpdateSqDistMMFromStartPosMax @ 0x6ba75c2e`**、**`Touch_IsFarAwayFromCornerCompareToStartPoint @ 0x6ba74aef`**、**`Touch_UpdateGlobalState @ 0x6ba75b75`**：维护生命周期内的各项极值（最大/最小峰值、最大 $Z_4$、最大帧间位移、最大/最小边缘距离、最大起点距离平方 `+0x24e`）及全局表头状态位 `+0x24`、`+0x38`。

#### D. 事件后处理、形状异常检测、IIR 特征平滑与补点生成 (17 个函数)
46. **`Touch_ProcessAftTouchEvent @ 0x6ba75af5`** 与 **`Touch_ProcessAftTouchEventUnit @ 0x6ba7542a`**：在 `TE_Process` 末尾对每个触控点执行：
    - 可信度评估 `Touch_CheckTrustabilityUnit` 与 `Touch_SetTrustableTag`；
    - 4 类异常几何形状检测 `Touch_IsAbnormalShape` ~ `Touch_IsAbnormalShape4`（累计计数写入 `g_touchesPtr[0x10..0x13]`）；
    - 合并抬起检测 `Touch_IsLiftOffDueToMerged`（置位 `+0x1fc |= 0x1000000` 并调用 `Touch_GetMergedInfo`）与分裂按下检测 `Touch_IsTouchDownDueToSeperated`（置位 `+0x1fc |= 0x2000000`）；
    - 侧边越界检测 `Touch_IsSideOverCore` 与 `Touch_IsCoreOffInSide`；
    - 几何特征平滑 `Touch_IIRDataProcess` 及最大通道数 `+0x228` 更新。
47. **`Touch_CheckTrustabilityUnit @ 0x6ba718d4`** 与 **`Touch_SetTrustableTag @ 0x6ba71d77`**：统计已报点帧数 `+0x23a`；若出现长轴 `> 2.5 mm`、帧间位移且加速度 `> 25 mm²`、异常标志 `+0x1ec != 0` 等情况则标记可疑位 `+0x1fc |= 0x20000`；在抬起帧（`teState == 0x20`）汇总输出可信度标签 `+0x2a0`（`1`=可信正常触控累加至 `g_tsaStaticPtr->field_0xac`，`2`=可疑触控累加至 `field_0xb0`）。
48. **`Touch_IsAbnormalShape @ 0x6ba74dfe`**、**`Touch_IsAbnormalShape2 @ 0x6ba7504e`**、**`Touch_IsAbnormalShape3 @ 0x6ba7513e`**、**`Touch_IsAbnormalShape4 @ 0x6ba7532f`**、**`Touch_IsInAbnormalShapeDetectArea @ 0x6ba74c6f`**：检测大面积畸变（如长轴 `> 3.0 mm`、传感器数 `> 39` 且跨越 `> 4×4`、长短轴比 `> 3:1` 且峰值偏低、长轴 `> 4.9 mm`、或传感器数 $\ge 27$ 且跨越 $\ge 6\times 6$）。
49. **`Touch_IIRDataProcess @ 0x6ba7474f`** 与 **`Touch_IIRProcess @ 0x6ba746ed`**：对 `majorAxis`、`minorAxis`、`dim1Width`、`dim2Width`、`angle`、`disparity`、`gripRatioDim1/2` 共 8 个几何特征执行带 $\pm 1$ 步进补偿的 $7:1$ IIR 滤波（存入 `+0x25e..+0x26c`）：
    $$y_{\text{iir}} = \left\lfloor \frac{7 y_{\text{prev}} + x_{\text{cur}}}{8} \right\rfloor + \text{sgn}\left(x_{\text{cur}} - \left\lfloor \frac{7 y_{\text{prev}} + x_{\text{cur}}}{8} \right\rfloor\right)$$
50. **`Touch_CreatePostReport @ 0x6ba76a03`** 与 **`Touch_GetPostReportLength @ 0x6ba76958`**：当手套模式或延迟确认点首次触发报点时，计算当前 `ecPos` 与初始起点 `startEcPos`（或 `firstRptPos`）的切比雪夫距离并除以 `2 mm` 对应的像素长度，确定插值点数 $L \in [2, 5]$；随后清空 `HistoryBuffer`，在起点与当前点之间均匀线性插值 $L-1$ 个过渡点压入 `HistoryBuffer`，并置位 `persistFlags1 |= 0x80 | 0x20000` 交由后续帧平滑回放。
51. **`Touch_ProcessBetaGrip @ 0x6ba76ee8`** 与 **`Touch_IsBetaGrip @ 0x6ba76d53`**：检测屏幕底角（左右边缘 8 像素内、距底边 `15 mm` 内）的高幅值持续按压（连续 $> 1$ 帧），置位 `persistFlags1 |= 0x1000000` 并累加统计计数。
52. **`Touch_ProcessExtInfo @ 0x6ba7713d`**：汇总水下检测 `TSA_IsUnderWaterDetected`（`0x20`）、出水检测 `TSA_IsOutOfWaterDetected`（`0x40`）、接近光 `Proximity_IsDetected`（`0x80`）、拿起/放下手势超时（`0x100 / 0x200`）写入 `g_touchesPtr + 0x18`，并调用 `Posture_UpdatePostureType` 与 `AntiTouch_Report`。
53. **其余状态查询辅助函数**：`Touch_IsOff @ 0x6ba74ab5` / `Touch_IsOffEvent @ 0x6ba74a8e`（检查 `teState` 是否为 `8, 0x10, 0x20`）、`Touch_IsException @ 0x6ba7416a`、`Touch_IsGrip @ 0x6ba742be`、`Touch_ValidComparison @ 0x6ba7435e`、`Touch_NeedXYFilter @ 0x6ba742dc`（`prev.persistFlags1 & 0x40` 或 `teState & 0x14`）、`Touch_GetTimeOn @ 0x6ba6f517`、`Touch_GetTimeRpt @ 0x6ba6f5ac`、`Touch_IsGhostByMinSigOff @ 0x6ba7160b`、`Touch_IsGhostByMinSigOff2 @ 0x6ba7173d`、`Touch_IsFullyLiftOff @ 0x6ba717cf`、`Touch_IsSideOverCore @ 0x6ba714db`、`Touch_IsCoreOffInSide @ 0x6ba715a8`、`Touch_IsSideCoreInSameZone @ 0x6ba71415`、`Touch_IsLiftOffDueToMerged @ 0x6ba73da0`、`Touch_GetMergedInfo @ 0x6ba74b56`、`Touch_IsTouchDownDueToSeperated @ 0x6ba73e7a`。

### 4.9.2 `PrevTouch.c` 完整函数清单与逆向解析 (共 20 个函数)

1. **`PrevTouch_Init @ 0x6ba6db42`**：调用 `PrevTouch_Release`。
2. **`PrevTouch_Release @ 0x6ba6da90`**：调用 `memset_s(g_pPrevTouches, 0x5aa0, 0, 0x5aa0)` 清空整个 `PrevTouchTable`（`0x140 + 20 * 0x478 = 0x5aa0` = 23200 字节），并根据横竖屏状态初始化 `field_0x18`。
3. **`PrevTouch_UnitCopy @ 0x6ba6d300`**：将 `Touch[param_2]` 的 `0x144` 字节镜像拷贝至 `PrevTouch[param_1]` 头部。
4. **`PrevTouch_UnitRelease @ 0x6ba6da04`**：清零单个 `PrevTouch[param_1]`（`0x478` 字节）。
5. **`PrevTouch_UnitRemove @ 0x6ba6dcf3`**：移除 `PrevTouch[param_1]`，将后续单元前移 `0x478` 字节并递减 `bPrevTouchCount`。
6. **`PrevTouch_AssignCurIdxBasedOnID @ 0x6ba6dbc9`**：按 `touchId` 查找当前帧对应单元，写入 `matchedCurIdx`（`+0x4f2`）。
7. **`PrevTouch_GetIdxBasedOnID @ 0x6ba6db56`**：按 `touchId` 查找并返回 `prevIdx`（未找到返回 `0x14`）。
8. **`PrevTouch_GetIdxBasedOnCurIdx @ 0x6ba6dcc4`**：返回 `Touch[param_1].matchedPrevIdx`（`+0x287`）。
9. **`PrevTouch_IsValidId @ 0x6ba6e909`**：检查指定 ID 是否既不在 `PrevTouch` 活跃列表中，也不在刚抬起的禁用队列 `g_prevTouchInvalidID` 中。
10. **`PrevTouch_IsAnyExceptionDetected @ 0x6ba6e898`**：检查是否有任何 `PrevTouchUnit` 存在异常（`+0x1ec != 0`）或全局异常 `nTouchCount != 0`。
11. **`PrevTouch_UnitUpdateCommon @ 0x6ba6d427`**：在新点首帧（`frameAge == 0`）记录 Peak 所在通道的 `g_tsaBufMinSigOff`、`g_tsaBufTimeOff`、`g_tsaBufTimeRpt` 初值至 `+0x5a6..+0x5aa`。
12. **`PrevTouch_UnitUpdate @ 0x6ba6d5ed`**：在非抬起帧（`teState != 8 && teState != 0x20`）计算并更新跨帧运动学状态：
    - 原始质心差分 `dRaw = cur.rawPos - prev.rawPos`（`+0x4c8, +0x4ca`）；
    - 原始质心速度 `vRaw = dRaw / dt`（`+0x4cc, +0x4ce`）与加速度 `aRaw = vRaw - prev.vRaw`（`+0x4d0, +0x4d2`）；
    - EC 坐标差分 `dEc = cur.ecPos - prev.ecPos`（`+0x4d4, +0x4d6`）、二阶差分 `ddEc = dEc - prev.dEc`（`+0x4d8, +0x4da`）与累计位移（`+0x4dc, +0x4e0`）；
    - 最大帧间位移平方 `+0x4f6`、连通域包围盒极值 `+0x518..+0x51e` 及高信噪比时间戳 `+0x520`。
13. **`PrevTouch_PostProcess @ 0x6ba6e09d`** 与 **`PrevTouch_RemoveOff @ 0x6ba6de46`**：在 `TSA_Processing` 帧末调用，将 `g_touchesPtr` 表头状态同步至 `g_pPrevTouches`，并对每个触控点：若处于抬起完成状态（`teState & 0x70 != 0`，即 `0x10 / 0x20 / 0x40`），调用 `PrevTouch_UnitRemove` 释放槽位并刷新 `Touch_AssignPreIdxBasedOnID`；否则调用 `PrevTouch_UnitCopy` 将当前帧最新状态保存至 `PrevTouch` 供下一帧使用。
14. **`HistoryBuffer` 操作函数族（共 6 个函数）**：管理 `PrevTouchUnit + 0x288` 处的 30 槽位历史轨迹线性表（每项 16 字节 `[x:i16, y:i16, pad:u32, timestamp:u64]`）：
    - `PrevTouch_HistoryBufferClean @ 0x6ba6e9b0`：清零计数 `+0x288 = 0`。
    - `PrevTouch_HistoryBufferGetNum @ 0x6ba6e9dc`：返回当前条数 `+0x288`。
    - `PrevTouch_IsHistoryBufferFull @ 0x6ba6f3ef`：判断 `+0x288 == 30`（`0x1e`）。
    - `PrevTouch_HistoryBufferPush @ 0x6ba6ea52`：向尾部追加一项（若已满 30 项则先调用 `PrevTouch_HistoryBufferRemove(preIdx, 0)` 丢弃最老项）。
    - `PrevTouch_HistoryBufferPop @ 0x6ba6ec0d`：从头部（索引 0，最老项）弹出一项，并将后续元素前移。
    - `PrevTouch_HistoryBufferGet @ 0x6ba6ee1d`：读取第 `param_2` 项内容。
    - `PrevTouch_HistoryBufferInsert @ 0x6ba6ef44`：在指定位置 `param_2` 插入一项（后移后续元素，用于 `ER_MoveInCorrection` 向头部回溯插值）。
    - `PrevTouch_HistoryBufferRemove @ 0x6ba6f11a`：删除第 `param_2` 项并前移后续元素。
15. **`HistoryQueue` 网格队列操作函数族（共 2 个函数，总计 20 个函数）**：
    - `PrevTouch_HistoryQueuePush @ 0x6ba6e650` 与 `PrevTouch_HistoryQueuePush_internal @ 0x6ba6e448`：管理 `+0x474..+0x495` 的 16 槽位整格坐标 `(x>>8, y>>8)` 环形队列；当新网格与队尾网格不相邻时，自动插入两者的中点网格 `((x_prev + x_cur)/2, (y_prev + y_cur)/2)`，并去重入队。
## 4.10 坐标平滑与防抖滤波模块 (`Filter.c` 全 22 函数 + 入口 `Filter_Process @ 0x6ba5feec` / `Filter_Init @ 0x6ba5c160`)

坐标滤波模块位于 `Touch_Process`（更新 `TouchAction`、异常标记与历史窗口）之后，负责将边缘补偿后的内部坐标 `(ecPos.x, ecPos.y)`（`TouchUnit + 0x162, +0x164`）依次通过 **Click-Hold 冻结与 Click-to-Flick 偏移渐消器**、**基于距离与压感抬起检测的自适应一阶 IIR 滤波器 (`AIIR` + `Q16.8 IIR`)**、**迟滞死区防抖滤波器 (`JITFilter`)** 以及 **折返轨迹预测混合器 (`CurveReturn Predict`)**，生成最终上报内部坐标 `(rptPos.x, rptPos.y)`（`TouchUnit + 0x170, +0x172`）。

### 4.10.1 顶层调度与点击冻结/滑动渐消 (`Filter_Process` -> `Filter_ModeTouchReportedProcessUnit`)

1. **`Filter_Init @ 0x6ba5c160`**（位于 `NO_PREFIX.c`，由 `Touch_Init` 调用）：空桩函数。
2. **`Filter_Process @ 0x6ba5feec`**（位于 `NO_PREFIX.c`，由 `TSA_Processing @ 0x6ba9a443` 调用）：
   - 遍历所有当前帧未处于抬起完成态（`teState & 0x20 == 0`）的 `TouchUnit`，依次调用 `Filter_ProcessUnit(curIdx)`。
3. **`Filter_ProcessUnit @ 0x6ba5fd96`**：
   - 调用 `TSAOut_Dim1PosInternal(ecPos.x)` 与 `TSAOut_Dim2PosInternal(ecPos.y)` 将滤波前物理坐标存入调试全局变量 `g_xPosPreFilter`、`g_yPosPreFilter`；
   - **未确认上报直通分支**：若 `(persistFlags1 & 8) == 0`（尚未通过首帧上报门限），直接令 `iirPos = ecPos`（`+0x166, +0x168`）、`jitPos = ecPos`（`+0x16c, +0x16e`）；
   - **已确认上报滤波分支**：若 `(persistFlags1 & 8) != 0`，调用 `Filter_ReportedProcessUnit(curIdx)`；
   - 最后将 `jitPos` 赋值给最终上报坐标 `rptPos`（`+0x170, +0x172`）。
4. **`Filter_ReportedProcessUnit @ 0x6ba5fb41`**：
   - 检查是否为**首帧上报**（`prev.persistFlags1 & 8 == 0`）或**后置补偿首帧**（`cur.persistFlags1 & 0x20000 != 0 && prev.persistFlags1 & 0x20000 == 0`）：
     - 若是，初始化动态死区半径平方阈值 `jitSqDistThold`（`TouchUnit + 0x27c`）为 `*(u16*)(g_tsaPrmtFlashConst + 0x12)`（若存在异常或 `tzType == 3` 则取 `*(u16*)(g_tsaPrmtFlashConst + 0x10)`），令 `iirPos = ecPos`、`jitPos = ecPos`，并将 IIR 小数余数 `iirRemX`、`iirRemY`（`+0x16a, +0x16b`）清零；
     - 否则（正常连续上报帧），调用 `Filter_ModeTouchReportedProcessUnit(curIdx, prevIdx)`。
5. **`Filter_ModeTouchReportedProcessUnit @ 0x6ba5f1c2`**：
   - **第一阶段：Click-Hold 坐标冻结判定**（当 `dwFeatureFlags & 0x100 != 0` 且非手写笔 `(dwFeatureFlags & 0x1000) == 0` 时启用）：
     在以下任一条件成立时触发**点击坐标完全冻结**（令 `iirPos = prev.iirPos`，置位 `persistFlags1 |= 0x800`，并将当前真实坐标与冻结坐标的偏差记录至 `PrevTouchUnit` 的 `clickToFlickOffset` `+0x4ee, +0x4f0`）：
     $$\Delta_{\text{hold},x} = x_{\text{ec}}(t) - x_{\text{iir}}(t-1), \quad \Delta_{\text{hold},y} = y_{\text{ec}}(t) - y_{\text{iir}}(t-1)$$
     - **条件 A（点击轻抬阶段防漂移）**：`z4Ratio < 16`（`< 100%`）且 `peakRatio < 20`（`< 125%`）且 `peakSig < dynamicThold / 2`，同时上一帧处于 `touchAction == 1`（Click 态）且未标记接触面积膨胀（`prev.persistFlags1 & 0x8000 == 0`）；
     - **条件 B（按压接触面膨胀伪滑动抑制）**：`(persistFlags1 & 0x9000) == 0` 且 `(volatileFlags1 & 0x100000) != 0`（`Touch_FalseFlickCheck` 检出由接触面积快速增大引起的质心假性偏移）；
     - **条件 C（Click 状态原地微动冻结）**：`(touchAction & 1) != 0`（仍处于 Click 动作周期内，未超时且未越过点击抑制半径 `2.0 mm`）且 `(persistFlags1 & 0x8000) == 0`，且 $|\Delta_{\text{hold}}|$ 在最大点击冻结跨度 `TSAOut_ExtToIntDist(g_tsaPrmtDynamic[0x76] * 2)` 以内（`W273AS2700` 中 `flash+0x0903 = 20` = $2.0\text{ mm}$）。
   - **第二阶段：Click-to-Flick 偏移渐消与自适应 IIR 滤波**：
     - 若上述冻结条件均不成立（例如手指已滑动越过 `2.0 mm` 或按压超过 `150 ms`，使 `touchAction` 切换为 `2` = Flick/Move）：
       - 若 `persistFlags1 & 0x800 != 0`（存在尚未释放的 Click-Hold 冻结偏差 $\Delta_{\text{hold}}$），调用 `Filter_ReportClickToFlickProcessUnit(curIdx, prevIdx)` 逐帧线性消减偏差；当 X/Y 双轴偏差均归零（返回 `1`）时，清除 `persistFlags1 &= ~0x800`；
       - 若无冻结偏差（`persistFlags1 & 0x800 == 0`）：检查 `Touch_NeedXYFilter()`（当 `g_tsaPrmtDynamic[0x80] & 1 != 0` 时返回 `true`），若开启则调用 `Filter_IIRPDims(curIdx, prevIdx)` 执行自适应一阶 IIR 滤波，否则直通 `iirPos = ecPos`。
   - **第三阶段：JIT 迟滞死区滤波**：
     - 调用 `Filter_JitFilterProcessUnit(curIdx, prevIdx)`，由 `iirPos` 生成 `jitPos`（`+0x16c, +0x16e`）。
6. **`Filter_ReportClickToFlickProcessUnit @ 0x6ba5e6c3`**、**`Filter_ReportClickToFlickDim1OffsetCheck @ 0x6ba5e149`** 与 **`Filter_ReportClickToFlickDim2OffsetCheck @ 0x6ba5e409`**：
   - 为避免从 Click 冻结态切换到 Flick 滑动态的瞬间坐标发生阶跃跳变（即常见的“起步断层/跳点”），算法先令临时输入坐标减去冻结偏差：
     $$\tilde{x}_{\text{ec}}(t) = x_{\text{ec}}(t) - \Delta_{\text{hold},x}(t-1), \quad \tilde{y}_{\text{ec}}(t) = y_{\text{ec}}(t) - \Delta_{\text{hold},y}(t-1)$$
   - 对 $(\tilde{x}_{\text{ec}}, \tilde{y}_{\text{ec}})$ 执行 `Filter_IIRPDims` 得到平滑坐标 $\tilde{x}_{\text{iir}}(t)$，随后恢复 `ecPos`；
   - 在 `Filter_ReportClickToFlickDim1OffsetCheck` / `Dim2OffsetCheck` 中，计算单帧追赶步长：
     $$\text{step}_x = \min\left( \text{TSAOut\_ExtToIntDist}\big(\text{g\_tsaPrmtDynamic}[0x7e]\big),\; \left\lfloor \frac{|\tilde{x}_{\text{iir}}(t) - x_{\text{iir}}(t-1)|}{4} \right\rfloor \right)$$
     其中 `g_tsaPrmtDynamic[0x7e]` 源自 `flash+0x0905`（`W273AS2700` 中为 `10` = $1.0\text{ mm}$）；
   - 每帧将 $|\Delta_{\text{hold},x}|$ 减去 $\text{step}_x$，同时将 $x_{\text{iir}}(t)$ 向真实坐标方向补偿 $\text{sgn}(\Delta_{\text{hold},x}) \cdot \text{step}_x$，直至 $|\Delta_{\text{hold},x}| < \text{step}_x$ 时归零并返回 `1`。

### 4.10.2 自适应一阶 IIR 滤波 (`Filter_IIRPDims` 与 `AIIR` / `Q16.8 IIR`)

7. **`Filter_IIRPDims @ 0x6ba5dcd4`**：
   - 调用 `local_9 = Filter_GetCoefBasedOnDistance(curIdx, prevIdx)` 获取当前帧自适应滤波权重 $w \in [1, 32]$；
   - **折返轨迹检测**：若 `Filter_IsNeedCurve()` 为真，调用 `Filter_IsCurveReturn(curIdx, prevIdx)` 与 `Filter_IsPredictFinish(curIdx, prevIdx)`；
   - **抬手强阻尼锁死**：调用 `Filter_IsHandLift(curIdx)` 检测信号总和骤降，或调用 `Filter_IsTouchLiftOff(curIdx, prevIdx)` 检测 Z 值骤降；若检出抬起趋势，强制将滤波权重压至最低 **`local_9 = 1`**（即 $\frac{1}{32}$ 新坐标 + $\frac{31}{32}$ 历史坐标），并重置死区阈值 `jitSqDistThold`，彻底抑制手指离开屏幕瞬间因接触面积坍缩造成的尾部甩尾（Lift-off Tail Jitter）；
   - **边界抗粘滞直通**：若 `ecPos` 已到达面板左/上边界 `0` 或右/下边界 `maxPos - 1` 且距上一帧 `iirPos` 步长 $> 49$，直接令该轴 `iirPos = ecPos`；否则分别调用 `Filter_ProcessPDim1(curIdx, prevIdx, local_9)` 与 `Filter_ProcessPDim2(curIdx, prevIdx, local_9)`；
   - 将滤波后物理坐标写入调试全局变量 `g_xPosAftIirFilter`、`g_yPosAftIirFilter`。
8. **`Filter_GetCoefBasedOnDistance @ 0x6ba5c99c`** 与 **`Filter_NeedHeavyCoef @ 0x6ba5c240`**：
   - `Filter_NeedHeavyCoef`：当触控点类型为大尺寸/手掌（`tzType == 3`）或存在全局噪声异常（`g_touchesPtr->exceptionFlags != 0`）时，置位 `persistFlags1 |= 0x10` 并返回 `1`；
   - 根据触控状态从 `g_tsaPrmtFlash`（偏移 `0x088e..0x089f`）选择三组 6 字节自适应 IIR 参数结构体 `[minDist:u16, maxDist:u16, minCoef:u8, maxCoef:u8]` 之一拷贝至 `g_tsaPrmtRam + 0x16..0x1b`：

| 档位模式 | 触发条件 | Flash 偏移 | `minDist` (内部坐标) | `maxDist` (内部坐标) | `minCoef` ($w_{\min}$) | `maxCoef` ($w_{\max}$) | `W273AS2700` 含义 |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **正常手指档 (Normal)** | 默认手指触控 | `flash+0x088e` | `0` | `384` (`0x0180`, 1.5 格) | `0` (实际下限 `1`) | `32` (`0x20`, 直通) | 低速强平滑，移动超 1.5 格全通无延迟 |
| **强滤波档 (Heavy)** | `Filter_NeedHeavyCoef() != 0` | `flash+0x0894` | `0` | `512` (`0x0200`, 2.0 格) | `0` (实际下限 `1`) | `12` (`0x0c`, $12/32$) | 噪声/大面积下最大权重限制为 $37.5\%$ |
| **手写笔/特殊档 (Stylus)** | `dwFeatureFlags & 0x1000 != 0` | `flash+0x089a` | `0` | `512` (`0x0200`, 2.0 格) | `8` (`0x08`, $8/32$) | `32` (`0x20`, 直通) | 提高静止起步权重至 $25\%$，降低笔尖滞后 |

   - 调用 `Filter_GetAdaptiveCoef(curIdx, prevIdx)` 计算权重 $w$；
   - **后置上报指数爬坡**：若 `prev.persistFlags1 & 0x40 != 0`（刚刚完成 Post-Report 历史轨迹回填），将 `prev.+0x4f3` 每帧左移 1 位（`2 -> 4 -> 8 -> 16 -> 32`）作为权重 $w$，满 `32` 后清除 `0x40` 标志。
9. **`Filter_GetAdaptiveCoef @ 0x6ba5c690`**：
   - 计算当前帧 `ecPos` 与上一帧 `iirPos` 的欧氏距离：
     $$d = \left\lfloor \sqrt{\big(x_{\text{ec}}(t) - x_{\text{iir}}(t-1)\big)^2 + \big(y_{\text{ec}}(t) - y_{\text{iir}}(t-1)\big)^2} \right\rfloor$$
   - 调用 `AIIR_GetCoef(d)`（在 `[minDist, maxDist]` 区间内对 `[minCoef, maxCoef]` 做线性插值，并夹紧于 $\ge 1$）；
   - 若处于快速移动状态（`TouchUnit + 0x279 == 1`），限制下界 $w \ge 8$；
   - **系数斜率限制 (Slew-Rate Limiting)**：若上一帧系数 `prevCoef`（`PrevTouchUnit + 0x4f4`）非零，且当前帧未越过点击抑制半径（`persistFlags1 & 0x100 == 0`），限制当前帧权重 $w(t)$ 相对于上一帧 $w(t-1)$ 的变化不超过 $\pm 1$：
     $$w(t) = \text{clamp}\big(\text{AIIR\_GetCoef}(d),\; w(t-1) - 1,\; w(t-1) + 1\big)$$
     并将结果写回 `TouchUnit + 0x27b`（镜像至下一帧 `prev.+0x4f4`）。
10. **`Filter_ProcessPDim1 @ 0x6ba5cc76`** 与 **`Filter_ProcessPDim2 @ 0x6ba5cd9c`**：
    - 调用底层 `IIR_Process(ecPos << 8, (prev.iirPos << 8) + prev.iirRem, w, 5)`，在 **Q16.8 定点域**（左移 8 位保留 1 字节小数余数）执行分母为 $2^5 = 32$ 的一阶低通 IIR 滤波：
      $$Y_{Q16.8}(t) = \left\lfloor \frac{w \cdot \big(x_{\text{ec}}(t) \ll 8\big) + (32 - w) \cdot \Big(\big(x_{\text{iir}}(t-1) \ll 8\big) + r_{\text{iir}}(t-1)\Big)}{32} \right\rfloor$$
      $$x_{\text{iir}}(t) = Y_{Q16.8}(t) \gg 8, \qquad r_{\text{iir}}(t) = Y_{Q16.8}(t) \ \&\ \text{0xFF}$$
    - 将整数部分存入 `iirPos`（`+0x166, +0x168`），8 位小数余数存入 `iirRem`（`+0x16a, +0x16b`）。保留 8 位小数余数彻底消除了小权重（如 $w=1, 2$）时传统整数截断造成的极限环死锁（Limit-Cycle Sticking）。

### 4.10.3 抬起检测、折返轨迹预测与 JIT 迟滞死区滤波

11. **`Filter_IsTouchLiftOff @ 0x6ba5c363`** 与 **`Filter_IsTouchLiftOffJitterSuppressEnable @ 0x6ba5c5fc`**：
    - `Filter_IsTouchLiftOffJitterSuppressEnable` 读取 `g_tsaPrmtFlash + 0x08a2`（`W273AS2700` 中为 `0x00`，默认关闭该分支，改由 `Filter_IsHandLift` 承担抬手检测）；
    - 若开启，当 `HistoryBuffer` 已满 30 帧、触控持续时间 $\ge 301\text{ ms}$（`0x12d`）且上一帧 Z 值相对当前帧 Z 值跌幅超过 $31\%$（$\lfloor z_{t-1} \times 100 / z_t \rfloor - 100 \ge 31$）时，置位 `persistFlags1 |= 0x400000` 并返回 `true`。
12. **`Filter_IsHandLift @ 0x6ba5c644`** 与 **`Filter_IsHandLiftFinish @ 0x6ba5dc30`**：
    - `Filter_IsHandLift`：检查 `TouchUnit + 0x1fc`（`sigSumRatio`）是否小于 `28`（`0x1c`，即当前帧总信号量跌至峰值总信号量的 $28/64 = 43.75\%$ 以下）；若成立且 `Filter_IsHandLiftFinish` 未超时，返回 `true`（触发 `Filter_IIRPDims` 强制 `w = 1`）；
    - `Filter_IsHandLiftFinish`：每次触发时递增计数器 `TouchUnit + 0x280`，满 5 帧后停止强制锁死（防止手指轻按不抬时坐标长期卡死）。
13. **折返轨迹检测与线性外推预测函数族（共 6 个函数）**：
    - **`Filter_IsNeedCurve @ 0x6ba5cec2`**：当非手掌（`tzType != 3`）且无全局异常时返回 `1`。
    - **`Filter_IsCurveReturn @ 0x6ba5d178`**：在屏幕内区（距四边 $> 384$ 内部单位，即 1.5 个传感器间距内）、`HistoryBuffer` 满 30 帧且未处于预测周期（`prev.+0x5ad == 0`）时，沿 `HistoryBuffer` 从最新帧向前积分最近 $80\text{ ms}$（`0x50`）内的轨迹弧长 $L_{\text{arc}} = \sum (\Delta x^2 + \Delta y^2)$，并记录距今约 $40\text{ ms}$（`< 0x29`）的中途锚点索引 `local_20` 与 $80\text{ ms}$ 起点索引 `local_1e`。若满足：
      1. 弧长 $L_{\text{arc}} > 128$ 且运动呈现明显的主轴方向性（$|\Delta x| > 5|\Delta y|$ 或 $|\Delta y| > 5|\Delta x|$）；
      2. 起点 `local_1e` 到最新点的弦长平方 **小于** 起点 `local_1e` 到中途点 `local_20` 的弦长平方（即轨迹发生了 180° 折返/U-Turn！）；
      则触发折返补偿：置 `prev.+0x5ad = 1`（`predictStep = 1`），调用 `Filter_GetSpeed` 计算近 6 帧平均速度 $v$，调用 `Filter_GetTurnNum` 与 `Filter_GetForwardNum` 设定折返过渡帧数。
    - **`Filter_GetSpeed @ 0x6ba5cf7a`**：取 `HistoryBuffer` 最近 6 帧（`num-1` 与 `num-7`）的主轴位移除以时间差 $\Delta t$，返回平均速度 $v$。
    - **`Filter_GetTurnNum @ 0x6ba5d0dc`**：计算折返混合总帧数 $\text{turnNum} = \min\big(12,\; \lfloor 660 / |v| \rfloor\big)$（存入 `prev.+0x5ae`）。
    - **`Filter_GetForwardNum @ 0x6ba5d12a`**：计算前向纯预测帧数 $\text{forwardNum} = \min\big(2,\; \lfloor 60 / |v| \rfloor\big)$（存入 `prev.+0x5af`）。
    - **`Filter_IsPredictFinish @ 0x6ba5d662`**：在折返补偿激活期间（`prev.+0x5ad != 0`）：
      - 若 `g_tsaPrmtFlash + 0x08a3 == 0`（**模式 0：线性外推混合模式**，`W273AS2700` 中 `flash+0x08a3 = 0x00` 即采用此模式）：只要 `predictStep <= turnNum` 且未撞击边界，利用 `HistoryBuffer` 最新两帧的速度向量对上一帧 `jitPos` 进行一阶线性外推，更新预测坐标 `predictPos`（`prev.+0x5b0, +0x5b2`）并递增 `predictStep`（`+0x5ad`）；
      - 若 `g_tsaPrmtFlash + 0x08a3 != 0`（**模式 1：死区阈值动态缩放模式**）：在 12 帧周期内前段按 $75\%$ 衰减 `jitSqDistThold`、后段按 $150\%$ 恢复 `jitSqDistThold`。
14. **`Filter_JitFilterProcessUnit @ 0x6ba5e994`**：
    - 计算分辨率步长比例因子 `iStack_24 = maxDim1Pos / wDim1Resolution + 1`（在 `W273AS2700` 中为 $\lfloor 15104 / 2560 \rfloor + 1 = 6$）；
    - 调用底层迟滞死区滤波器 `JITFilter(&jitPosX, &jitPosY, iirPosX, iirPosY, prevJitX, prevJitY, jitSqDistThold, iStack_24)`：当 `(iirPos - prevJit)^2 <= jitSqDistThold` 时保持 `jitPos = prevJit` 不动；当超出死区圆时，将坐标沿位移方向拉向死区边界，消除静止或极慢速移动时的 1~2 LSB 量化抖动；
    - **游戏场景微抖动回拉**：若 `TSA_IsGameScenario() != 0`（游戏模式下 `jitSqDistThold == 0`），对 `jitPos != prevJit` 的轴向 `prevJit` 方向回退 1 个内部单位，提供极低延迟下的亚像素抗抖动；
    - **折返轨迹线性混合（Mode 0）**：当 `g_tsaPrmtFlash + 0x08a3 == 0` 且折返预测处于混合阶段（`forwardNum < predictStep - 1`）时，将 `predictPos` 与当前 `jitPos` 按进度权重 $\alpha = \lfloor (\text{predictStep} - 1) \times 256 / \text{turnNum} \rfloor$ 进行线性插值混合：
      $$\text{jitPos}_{\text{out}} = \left\lfloor \frac{(256 - \alpha) \cdot \text{predictPos} + \alpha \cdot \text{jitPos}}{256} \right\rfloor$$
      使折返尖角处的轨迹平滑过渡，避免 IIR 相位滞后把快速折返削成圆钝短线。

---

## 4.11 触控动作分类状态机 (`TouchAction.c` 全 4 函数)

`TouchAction.c` 维护每个触控点的动作类型状态机 `touchAction`（`TouchUnit + 0x224`：`1` = Click 点击态，`2` = Flick/Move 滑动态），为 `Filter_ModeTouchReportedProcessUnit` 的 Click-Hold 坐标冻结提供核心判据。

1. **`TouchAction_Init @ 0x6ba77220`**：空初始化函数。
2. **`TouchAction_Process @ 0x6ba7733c`**：
   - 遍历所有当前帧未处于抬起完成态（`teState & 0x20 == 0`）的触控点，依次调用 `TouchAction_ProcessUnit(curIdx)`。
3. **`TouchAction_ProcessUnit @ 0x6ba77226`**：
   - 若上一帧已有状态（`prev.touchAction != 0`），继承 `cur.touchAction = prev.touchAction`；否则初始化为全局默认起始动作 `*(u32*)(g_tsaPrmtDynamic + 0x70)`（在 `TSA_Init` 中初始化为 `1` = Click）；
   - 当 `cur.touchAction == 1`（Click 态）时，检查以下两个转换条件，任一满足即切换为 **`cur.touchAction = 2`（Flick/Move 态）**：
     1. **空间位移越界**：`(persistFlags1 & 0x100) != 0`（在 `Touch_DisplacementCheck` 中，当前坐标距按下起点距离已超过点击抑制半径 `g_tsaPrmtDynamic[0x78]`，`W273AS2700` 中 `flash+0x0904 = 20` = $2.0\text{ mm}$）；
     2. **按压时间超时**：`curTimestamp - tdDebounceTimestamp > *(u16*)(g_tsaPrmtDynamic + 0x74)`（按压持续时间超过点击判定上限，`W273AS2700` 中 `flash+0x0902 = 0x0096` = **$150\text{ ms}$**）。
4. **`TouchAction_PostProcess @ 0x6ba773eb`**：
   - 在帧末遍历已确认上报并完成抬起（`teState & 0x20 != 0 && persistFlags1 & 8 != 0`）的触控点：
   - 若该点整个生命周期内未越过滑动判定距离（`persistFlags1 & 0x1000 == 0`），将下一新点的默认初始状态 `*(u32*)(g_tsaPrmtDynamic + 0x70)` 设为 `1`（Click）；否则设为 `2`（但注意在 `Touch_DisplacementCheck` 中每次无异常按压也会重置相关状态）；
   - 若抬起时上报坐标距起点平方距离 `rptSqDistFromStart < 20 mm²`（`0x14`）且未经历后置补偿（`persistFlags1 & 0x20000 == 0`），置 `g_flickTestFailed = 1`。

---

## 4.12 触控模式状态机与回调函数子树 (`TouchMode.c` 全 9 函数 + `_RDATA_CALLBACKS.c` 全 12 函数 + `_TM_CALLBACK_SUBTREE.c` 全 58 函数)

### 4.12.1 触控模式状态机架构 (`TouchMode.c` 全 9 函数)

`TouchMode.c` 实现了顶层四模态状态机（`0 = NoObj` 无物体、`1 = Finger` 正常手指、`2 = Glove` 手套/皮套、`3 = Stylus` 被动笔/小接触体），并在模式切换之后对被动笔（Stylus）与手套（Glove）触控点执行专属的空间净空滤波。状态变量存储于 `g_tsaPrpt[1]` 偏移 `0x28..0x40`：
- `g_tsaPrpt[1].field_0x28` (`u8`)：手套/特殊模式激活标志（`bGloveActive`）；
- `g_tsaPrpt[1].field_0x2c` (`u32`)：当前主模式 `curMode` $\in \{0, 1, 2, 3\}$；
- `g_tsaPrpt[1].field_0x30` (`u32`)：当前模式确认度 `curSubState`（`1` = Pending 弱保持，`2` = Confirmed 强确认）；
- `g_tsaPrpt[1].field_0x34` (`u32`)：当前帧模式检测输出 `curDetectResult`；
- `g_tsaPrpt[1].field_0x38..0x40`：上一帧模式状态备份 `[prevMode, prevSubState, prevDetectResult]`。

状态机通过位于 `.rdata` 段的函数指针表 `g_tmHandlerTable`（地址 `0x6bb52a40`，共 4 项，每项为 `[DetectFn, ProcessFn]` 两个 64 位函数指针，共 16 字节）驱动：

| 模式索引 `mode` | 模式名称 | 检测回调 `DetectFn(outResult, outSubState)` | 执行回调 `ProcessFn(result, subState)` |
| :--- | :--- | :--- | :--- |
| `0` | `NoObj` (无物体) | `TouchMode_NoObjDetect @ 0x6ba790ae` | `TouchMode_NoObjProcess @ 0x6ba79113` |
| `1` | `Finger` (正常手指) | `TouchMode_FingerDetect @ 0x6ba7911f` | `TouchMode_FingerProcess @ 0x6ba79318` |
| `2` | `Glove` (手套/皮套) | `TouchMode_GloveDetect @ 0x6ba793fe` | `TouchMode_GloveProcess @ 0x6ba797a7` |
| `3` | `Stylus` (被动笔) | `TouchMode_StylusDetect @ 0x6ba798b8` | `TouchMode_StylusProcess @ 0x6ba7991a` |

`TouchMode.c` 中的全部 9 个函数如下：

1. **`TouchMode_Init @ 0x6ba78b60`**：
   - 将 `g_tsaPrpt[1].field_0x2c..0x40` 初始化为模式 `0`（`NoObj`，`subState = 2`），清零 `field_0x28 = 0` 与连续帧计数器 `g_tsaStaticPtr[1].field_0xc = 0`。
2. **`TouchMode_Process @ 0x6ba78dde`**（由 `TSA_Processing @ 0x6ba9a443` 在 `Touch_Process` 与 `Filter_Process` 之后调用）：
   - 备份当前状态 `curMode..curDetectResult` 至 `prevMode..prevDetectResult`；
   - 调用当前模式的检测回调 `g_tmHandlerTable[curMode * 2](&curDetectResult, &curSubState)`；
   - 若当前模式不再满足保持条件（`curDetectResult == 1` 或 `curSubState == 1`），调用 `TouchMode_NewModeProcess()` 扫描其它模式并择优切换；
   - 调用切换后模式的执行回调 `g_tmHandlerTable[curMode * 2 + 1](curDetectResult, curSubState)`；
   - 最后调用 `TouchMode_StylusFilter()` 执行被动笔净空区滤波。
3. **`TouchMode_NewModeProcess @ 0x6ba78c16`**：
   - 遍历全部 4 个模式 `i = 0..3`（跳过已检测过的原模式），依次调用 `g_tmHandlerTable[i * 2](&res[i], &sub[i])`；
   - 将所有返回 `res[i] == 2`（检测到对应物体）的候选模式送入 `TouchMode_SwitchPriorityProcess(candMode, candSubState, &bestMode, &bestSubState)` 比较优先级；
   - 若找到更优模式则更新 `curMode = bestMode, curSubState = bestSubState`；若所有模式均返回 `res[i] != 2`，则回退至默认手指模式 `[curMode = 1, curSubState = 1, curDetectResult = 1]`。
4. **`TouchMode_SwitchPriorityProcess @ 0x6ba7900b`**：
   - 比较两个候选模式 `(modeA, subA)` 与 `(modeB, subB)`：优先选择 `subState == 2`（强确认）的一方；若 `subState` 相同，优先选择模式编号较小的一方（即优先级顺序为：`NoObj (0) > Finger (1) > Glove (2) > Stylus (3)`）。
5. **`TouchMode_StylusFilter @ 0x6ba7a0ed`**：
   - 当动态配置开启被动笔支持（`g_tsaPrmtDynamic[0x80] & 8 != 0` 且 `dwFeatureFlags & 0x10 != 0` 且非主动笔 `(dwFeatureFlags & 0x1000) == 0`，注：`W273AS2700` 中 `flash+0x0906 = 0x03`，bit 3 为 0，默认关闭此分支）时：
   - 若 `TouchMode_GetStylusNum() != 0`，调用 `TouchMode_StylusFilterProcess()`。
6. **`TouchMode_StylusFilterProcess @ 0x6ba79e26`**：
   - 调用 `TouchMode_StylusGetCoreIndex()` 找到信号最强（或面积最大）的核心笔尖触控点 `coreIdx`；
   - 若该核心点被 `TouchMode_StylusIsClearanceAreaSatisfied(coreIdx)` 判定净空区不满足（周围存在手掌/手指干扰），则拦截所有被动笔触控点（置 `suppressFlags |= 4`）；
   - 否则仅放行 `coreIdx` 一个笔尖点，将其余次要被动笔碎片点置 `suppressFlags |= 4` 予以抑制（若次要点已上报且距离核心点较近，还将核心点 ID 继承为已上报点的 `touchId` 以保持笔迹不断线）。
7. **`TouchMode_StylusGetCoreIndex @ 0x6ba79b0f`**：在所有满足 `Touch_IsStylus(idx)` 的触控点中，选出 `areaTotal`（`+0x174`）最大（若面積相同则 `sigSumTotal` `+0x178` 最大）的单元索引。
8. **`TouchMode_GetStylusNum @ 0x6ba79c23`**：统计当前帧满足 `Touch_IsStylus(idx)` 且未抬起的触控点总数。
9. **`TouchMode_StylusIsClearanceAreaSatisfied @ 0x6ba79930`**：检查核心笔尖点 `param_1` 与其它任何触控点之间的切比雪夫轴向距离是否小于净空阈值 `g_tsaPrmtFlash + 0x08ee`（`W273AS2700` 中 `u16@0x08ee = 500` = $50.0\text{ mm}$）；若在该净空半径内存在其它触控点，置位 `persistFlags1 |= 0x4000` 并返回 `0`（不满足净空）。

---

### 4.12.2 `.rdata` 回调函数表 (`_RDATA_CALLBACKS.c` 全 12 函数) 与模式回调子树 (`_TM_CALLBACK_SUBTREE.c` 全 58 函数)

`_RDATA_CALLBACKS.c` 汇总了 `TSACore.dll` 的 `.rdata` 只读数据段中引用的全部 12 个函数指针目标；`_TM_CALLBACK_SUBTREE.c` 则包含了 `g_tmHandlerTable` 8 个模式回调函数及其完整静态调用子树（共 58 个函数）。

#### (1) `g_tmHandlerTable` 8 个模式检测与处理回调函数（同时出现在 `_RDATA_CALLBACKS.c` 与 `_TM_CALLBACK_SUBTREE.c` 中）

1. **`TouchMode_NoObjDetect @ 0x6ba790ae`**：
   - 默认输出 `*outResult = 1, *outSubState = 2`；当活跃触控点总数等于刚抬起数（`bTouchCount == bLiftOffCount`，即当前帧无任何在屏触控点）时，输出 `*outResult = 2, *outSubState = 2`（确认处于无物体状态）。
2. **`TouchMode_NoObjProcess @ 0x6ba79113`**：空函数（无物体状态下无需额外抑制）。
3. **`TouchMode_FingerDetect @ 0x6ba7911f`**：
   - 当 `g_tsaPrmtDynamic[0x80] & 2 != 0`（`W273AS2700` 中 `flash+0x0906 = 0x03`，开启）且非主动笔时：
   - 遍历当前触控点，只要存在任意非小尺寸点（`volatileFlags1 & 0x800 == 0`）的 Peak 信号量 $\ge$ 动态手指检测门限 `*(i16*)(g_tsaPrmtDynamic + 0xc)`，即令 `*outResult = 2`（检测到手指）；
   - 若当前并非强确认手指模式（`curMode != 1 || curSubState != 2`），检查全局最大 Peak 信号量 `*(i32*)(*g_peaksPtr + 8)` 是否大于 `3 * (*(i16*)(g_tsaPrmtFlash + 0x05d8))`（`W273AS2700` 中 `3 * 90 = 270`）：连续 $> 2$ 帧（即满 3 帧，`g_tsaStaticPtr[1].field_0xc > 2`）满足时，令 `*outSubState = 2`（强确认进入手指模式）并退出手套激活标志 `g_tsaPrpt[1].field_0x28 = 0`。
4. **`TouchMode_FingerProcess @ 0x6ba79318`**：
   - 当处于强确认手指模式（`subState == 2` 或 `field_0x28 == 0`）且 `bFullIncellSignalDisparity == 0` 时，遍历所有触控点，将带有弱小信号标志（`volatileFlags1 & 0x800 != 0`，即疑似悬空/手套微弱信号）的点置位 **`suppressFlags |= 2`**（在正常手指模式下屏蔽微弱悬浮噪声）。
5. **`TouchMode_GloveDetect @ 0x6ba793fe`**：
   - 当开启手套/皮套或被动笔特性时，若存在未抬起触控点则令 `*outResult = 2`；
   - 若当前尚未进入手指模式（`curMode != 1`）且某触控点满足手套确认标志（`persistFlags1 & 4 != 0` 且 `(persistFlags1 & 0x80000) == 0`），置位 `g_tsaPrpt[1].field_0x28 = 1`、`*outSubState = 2`，并调用 `Touch_CreatePostReport(curIdx)` 生成后置补偿轨迹以平滑手套初次按下的滞后。
6. **`TouchMode_GloveProcess @ 0x6ba797a7`**：
   - 若未开启手套/皮套特性，调用 `TouchMode_StylusSuppression()`；
   - 否则若 `subState == 1`（手套待确认态），将所有触控点置位 `suppressFlags |= 2` 暂缓上报；若 `subState == 2`（手套已确认态），调用 `TouchMode_GloveFilter()`。
7. **`TouchMode_StylusDetect @ 0x6ba798b8`**：输出 `*outResult = 1, *outSubState = 1`（占位桩函数，被动笔检测已合并入 `TouchMode_StylusFilter`）。
8. **`TouchMode_StylusProcess @ 0x6ba7991a`**：空桩函数。

#### (2) `_RDATA_CALLBACKS.c` 中的其余 4 个非 TouchMode 回调函数（合计 12 函数）

9. **`tls_callback_0 @ 0x6bb47db0`**：Windows PE TLS（线程局部存储）初始化回调，在 DLL 进程附加（`param_2 == 0` 或 `3`）时调用 `__dyn_tls_dtor @ 0x6bb47d70`。
10. **`RecordTrigger_PutStr @ 0x6bafcdd0`**：DVR / 调试日志字符串输出回调，当 `param_1 != 0` 且长度 `< 20001` 时调用 `RecordTrigger_UpdateInfo(param_1, param_2)`。
11. **`TSA_GetSensorData @ 0x6ba8f9be`**：传感器帧数据指针获取回调，返回 `(void*)(g_rawData + 0x20)`。
12. **`memset @ 0x6bb4e540`**：CRT `memset` 导入跳板（被 `memset_s` 通过 `_refptr_memset` 引用）。

#### (3) `_TM_CALLBACK_SUBTREE.c` 中的其余 50 个子树被调函数（与前 8 个模式回调合计 58 函数）

- **触控与模式专属辅助函数（8 个）**：
  9. `TouchMode_GloveFilter @ 0x6ba796d1`：在手套确认态下，对非稳定手套点（`suppressFlags & 1 == 0`）且 `zValue > 64`（`0x40`，在 `Touch_GloveProcessUnit` 中评估的异常分值）的触控点置位 `suppressFlags |= 2`。
  10. `TouchMode_StylusSuppression @ 0x6ba79ba6`：遍历所有触控点置位 `suppressFlags |= 4`。
  11. `Touch_CreatePostReport @ 0x6ba76a03`：后置上报历史轨迹插值生成器（详见 §4.9.10）。
  12. `Touch_GetPostReportLength @ 0x6ba76958`：计算后置上报插值步数 $\in [2, 5]$（详见 §4.9.10）。
  13. `TS_MMToPixel @ 0x6ba7c416`：毫米转内部像素单位（详见 §4.6.4）。
  14. `PrevTouch_GetIdxBasedOnCurIdx @ 0x6ba6dcc4`：由 `curIdx` 获取 `prevIdx`（详见 §4.9.11）。
  15. `PrevTouch_HistoryBufferClean @ 0x6ba6e9b0`：清空 `HistoryBuffer`（详见 §4.9.11）。
  16. `PrevTouch_HistoryBufferPush @ 0x6ba6ea52`：向 `HistoryBuffer` 压入轨迹点（详见 §4.9.11）。
  17. `PrevTouch_HistoryBufferRemove @ 0x6ba6f11a`：从 `HistoryBuffer` 移除指定项（详见 §4.9.11）。
- **硬件异常上报与日志缓冲函数（5 个）**：
  18. `HardwareAnalyzer_SetSoftException @ 0x6baf0124`：调用 `HardwareAnalyzer_SetException(3)` 置位软件断言异常标志。
  19. `HardwareAnalyzer_SetException @ 0x6baf00f6`：执行 `g_hwAnalyzer |= (1 << (param_1 & 0x1f))`。
  20. `StrPrintf_Process @ 0x6bb3addf`：格式化调试日志字符串，当 `param_1 == 0` 时追加至 20000 字节全局日志缓冲 `g_logBuf`（满时通过函数指针 `g_strPrintfPutf` 刷出并调用 `StrPrintf_ClearLogBuffer`）。
  21. `StrPrintf_CatLogBuffer @ 0x6bb3ace3`：向 `g_logTsaBufPtr` 尾部追加字符串并累加 `g_logTsaBufCnt`。
  22. `StrPrintf_ClearLogBuffer @ 0x6bb3aca0`：调用 `memset_s(&g_logBuf, 20000, 0, 20000)` 清空日志缓冲区。
- **华为安全 C 运行库 (`libboundscheck` / `Sec*` 与 CRT 跳板，共 37 个函数）**：
  由 `StrPrintf_Process` -> `vsnprintf_s` 及 `memset_s` 静态展开的安全格式化输出引擎与标准库跳板，完整函数清单及地址如下：
  23. `vsnprintf_s @ 0x6ba46a10`（安全变参格式化入口，校验 `_DstBuf` 与 `_DstSize` 后调用 `SecVsnprintfImpl`）
  24. `SecVsnprintfImpl @ 0x6ba436a6`（构造流描述符并调用 `SecOutputS` 与 `SecPutZeroChar`）
  25. `memset_s @ 0x6ba416ef`（安全内存填充，校验边界后调用 `memset` 或 `SecMemsetError`）
  26. `SecMemsetError @ 0x6ba41680`（处理 `memset_s` 越界/空指针错误码 `0x16` / `0x22` / `0xa2`）
  27. `memset @ 0x6bb4e540`（CRT `memset` 跳板）
  28. `SecPutZeroChar @ 0x6ba43658`（向输出缓冲区写入结尾 `'\0'`）
  29. `SecOutputS @ 0x6ba42f21`（`printf` 格式串有限状态机主循环，驱动状态 `0..7`）
  30. `SecDecodeState @ 0x6ba420e1`（查表 `stateTable_4116` 解析格式串状态转移）
  31. `SecDecodeFlags @ 0x6ba42138`（解析 `' '`、`'^#'`、`'+'`、`'-'`、`'0'` 格式标志）
  32. `SecDecodeWidth @ 0x6ba4274a`（解析字段最小宽度）
  33. `SecDecodePrecision @ 0x6ba426bb`（解析 `.precision` 精度值）
  34. `SecDecodeSize @ 0x6ba422f6`（解析 `h/hh/l/ll/L/q/t/w/I` 长度修饰符）
  35. `SecDecodeSizeI @ 0x6ba421e4`（解析 `I`、`I32`、`I64` 修饰符）
  36. `SecUpdateOudiFlags @ 0x6ba4296f`（设置 `%o/%u/%d/%i` 进制与符号标志）
  37. `SecUpdateXpxFlags @ 0x6ba428e2`（设置 `%x/%X/%p` 十六进制与前导 `0x` 标志）
  38. `SecUpdatePointFlags @ 0x6ba42851`（设置 `%p` 16 位十六进制指针格式）
  39. `SecUpdateWcharFlags @ 0x6ba42808`（设置宽字符标志 `0x800`）
  40. `SecUpdateWstringFlags @ 0x6ba42835`（调用 `SecUpdateWcharFlags`）
  41. `SecUpdateNegativeChar @ 0x6ba42d3f`（处理 `%hhd` 负号补码转换）
  42. `SecUpdateSignedNumberPrefix @ 0x6ba42e91`（生成 `'+'`、`'-'` 或 `' '` 符号前缀）
  43. `SecNumberToBuffer @ 0x6ba41bb4`（将整数转换为字符串写入内部缓冲）
  44. `SecIsNumberNeedTo32Bit @ 0x6ba41b5e`（判断是否截断为 32 位无符号整数）
  45. `SecNumberToString @ 0x6ba41b37`（调用 `SecNumber64ToString`）
  46. `SecNumber64ToString @ 0x6ba41ad4`（按进制 `10 / 16 / 8` 分发整数转字符串）
  47. `SecNumber64ToDecString @ 0x6ba41977`（64 位十进制转字符串）
  48. `SecNumber64ToHexString @ 0x6ba41a71`（64 位十六进制转字符串）
  49. `SecNumber64ToOctalString @ 0x6ba41a0d`（64 位八进制转字符串）
  50. `SecNumberSatisfyPrecision @ 0x6ba42d93`（按精度要求补齐前导 `'0'`）
  51. `SecNumberForceOctal @ 0x6ba42e2d`（`%#o` 强制前导 `'0'`）
  52. `SecNumberCompatZero @ 0x6ba42f17`（零值兼容空桩）
  53. `SecDecodeTypeC @ 0x6ba42445`（解析 `%c/%C` 单字符参数）
  54. `SecDecodeTypeS @ 0x6ba425f6`（解析 `%s/%S` 字符串参数）
  55. `SecDecodeTypeSchar @ 0x6ba424be`（计算窄字符串输出长度，空指针替换为 `(null)`）
  56. `SecDecodeTypeSwchar @ 0x6ba4256d`（计算宽字符串输出长度）
  57. `SecInitFloatBuffer @ 0x6ba42bb5`（分配浮点数格式化缓冲区）
  58. `SecInitFloatFmt @ 0x6ba42ac1`（提取当前浮点数 `%...f/e/g` 子格式串）
  59. `SecSeekToFrontPercent @ 0x6ba42a87`（向前回溯查找 `'%'` 起始位置）
  60. `SecFormatDouble @ 0x6ba41813`（根据宽度与精度参数调用 `SecFormatFloat`）
  61. `SecFormatFloat @ 0x6ba417d0`（调用 `vsprintf` 格式化浮点数）
  62. `SecWriteFloatText @ 0x6ba42040`（输出格式化后的浮点文本）
  63. `SecFreeFloatBuffer @ 0x6ba42a0c`（释放动态分配的浮点缓冲）
  64. `SecOutputOneChar @ 0x6ba42652`（单字符安全写入流）
  65. `SecWriteMultiChar @ 0x6ba43734`（重复写入填充字符）
  66. `SecWriteLeftPadding @ 0x6ba41d98`（写入右对齐左侧空格填充）
  67. `SecWritePrefix @ 0x6ba41dea`（写入符号或 `0x` 前缀）
  68. `SecWriteLeadingZero @ 0x6ba41e7c`（写入 `'0'` 标志前导零填充）
  69. `SecWriteText @ 0x6ba4207d`（写入正文，宽字符走 `SecWriteTextAfterWctomb`）
  70. `SecWriteTextAfterWctomb @ 0x6ba41fac`（循环调用 `wctomb` 将宽字符转为多字节输出）
  71. `SecWriteRightPadding @ 0x6ba41edc`（写入左对齐右侧空格填充）
  72. `SecWriteStringChk @ 0x6ba41f38`（检查流容量后调用 `SecWriteStringToStream` 或 `SecWriteString`）
  73. `SecIsStreamBufEnough @ 0x6ba41d7a`（判断流剩余字节数是否充足）
  74. `SecWriteString @ 0x6ba437ba`（逐字节安全拷贝字符串至流）
  75. `SecWriteStringToStream @ 0x6ba41cf5`（长度 $\ge 12$ 时调用 `memcpy` 批量写入，否则调用 `SecWriteStringToStreamOpt`）
  76. `SecWriteStringToStreamOpt @ 0x6ba41c83`（短字符串循环展开写入）
  77. `malloc @ 0x6bb4e560`、`free @ 0x6bb4e5a0`、`vsprintf @ 0x6bb4e4d8`、`wctomb @ 0x6bb4e4d0`、`strlen @ 0x6bb4e510`、`memcpy @ 0x6bb4e548`（共 6 个 CRT 导入函数跳板）。

---

## 4.13 `W273AS2700` (`GaokunHimaxCSOT`) 第四部分核心参数汇总表

下表汇总了 `TSAPrmt.dll` 中 `W273AS2700`（`GaokunHimaxCSOT`）预设表在 `g_tsaPrmtFlash`（4216 字节主表）与 `g_tsaPrmtFlashConst`（48 字节常量表）中与第四部分（`CTD` / `ER` / `HotZoneAdjust` / `IDT` / `TS` / `TE` / `DB` / `TouchThold` / `Touch` / `Filter` / `TouchAction` / `TouchMode`）直接对应的关键配置值及其物理含义：

| 所属子模块 | 表名与偏移 | 数据类型 | 十六进制原值 | 十进制值 | 物理含义与算法作用 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **全局网格与分辨率** | `Const+0x00..0x03` | `u16[2]` | `0x1000, 0x0a00` | `4096, 2560` | 外部上报分辨率 $W_{\text{res}} = 4096, H_{\text{res}} = 2560$ |
| **全局网格与分辨率** | `Const+0x04..0x07` | `u16[2]` | `0x3b00, 0x2600` | `15104, 9728` | 内部坐标最大值（$59 \times 256 = 15104$, $38 \times 256 = 9728$） |
| **全局网格与分辨率** | `Const+0x1a..0x1b` | `u8[2]` | `0x3b, 0x26` | `59, 38` | 传感器物理通道数 $N_{\text{dim1}} = 59$ (Tx), $N_{\text{dim2}} = 38$ (Rx)，同时 `Const[0x1a]=59` 作为 $1\text{ mm}$ 对应的内部坐标缩放因子 |
| **JIT 死区滤波** | `Const+0x10..0x13` | `u16[2]` | `0x0000, 0x0000` | `0, 0` | 异常/正常态下默认 `jitSqDistThold` 初始死区平方阈值（`0` 表示默认无额外静态死区滞后） |
| **CTD 质心计算** | `Flash+0x05aa..0x05ab` | `u8[2]` | `0x01, 0x01` | `1, 1` | `CTD_RawCalculate` 连通域质心背景扣减偏置：$\max(0, \text{sig} - 1)$ |
| **CTD 边角补偿** | `Flash+0x05b0..0x05b5` | `i16[3]` | `220, 220, 128` | `220, 220, 128` | 边角补偿增益 `cornerGainDim1 = 220/256`，`cornerGainDim2 = 220/256`，中心衰减阈值 `128` |
| **CTD 边缘补偿表** | `Flash+0x05be..0x05fd` | `u16[32]` | `256..115` | `256..115` | 边缘信号比/增益查找表，用于将最外圈通道向物理边缘外推 |
| **IDT 跨帧追踪** | `Flash+0x0654..0x065b` | `u16[4]` | `768, 768, 868, 1228` | `768..1228` | 正常/大物体/高速状态下的跨帧匹配最大距离门限（$768 = 3.0\text{ 格}$） |
| **IDT 跨帧追踪** | `Flash+0x0664..0x0669` | `u16[3]` | `768, 1536, 1792` | `768, 1536, 1792` | 1 帧 / 2 帧 / 多帧速度外推预测的匹配搜索半径上限（$3.0 / 6.0 / 7.0\text{ 格}$） |
| **TS 触控尺寸** | `Flash+0x0762..0x0765` | `i16[2]` | `0x01c2, 0x0118` | `450, 280` | 触控尺寸分类阈值：`sigSum >= 450` 判为正常手指，`>= 280` 判为中等接触 |
| **TS 椭圆拟合** | `Flash+0x0784..0x0785` | `u8[2]` | `0x44, 0x44` | `68, 68` | 二阶矩特征值转主/次轴毫米数的缩放因子（`68 / 10`） |
| **TE / DB 消抖** | `Flash+0x0788..0x0790` | `u8` | `1, 0, 2, 1` | `1, 0, 2, 1` | 默认按下消抖 `1` 帧、抬起消抖 `0` 帧（大尺寸/噪声态按下 `2` 帧、抬起 `1` 帧） |
| **TouchThold 门限** | `Flash+0x05d6..0x05df` | `i16[5]` | `85, 90, 90, 85, 85` | `85, 90, 90, 85, 85` | 正常态下中心/四边基础触发信号门限（中心 `90`，边缘 `85`） |
| **TouchThold 门限** | `Flash+0x0630..0x064f` | `i16[16]` | `120..90` | `120..90` | 首帧上报确认信号门限表（按区域与尺寸档位索引） |
| **ER 边缘防误触** | `Flash+0x07ba..0x0807` | `struct` | `60, 120, 80...` | `6.0mm, 12.0mm...` | 左右/上下边缘防误触区宽度（$6.0\text{ mm} / 12.0\text{ mm}$）及滑入解锁距离门限 |
| **Filter 自适应 IIR** | `Flash+0x088e..0x0893` | `u16[2],u8[2]` | `0, 384, 0, 32` | `d:[0,384], w:[1,32]` | 正常手指 AIIR 参数：位移 $\ge 1.5\text{ 格}$ 时 $w=32$ 全通无延迟 |
| **Filter 自适应 IIR** | `Flash+0x0894..0x0899` | `u16[2],u8[2]` | `0, 512, 0, 12` | `d:[0,512], w:[1,12]` | 强滤波档 AIIR 参数（大尺寸/噪声异常）：最大权重限制为 $12/32$ |
| **Filter 自适应 IIR** | `Flash+0x089a..0x089f` | `u16[2],u8[2]` | `0, 512, 8, 32` | `d:[0,512], w:[8,32]` | 手写笔档 AIIR 参数：起步最小权重提升至 $8/32$ |
| **Filter 折返/抬起** | `Flash+0x08a2..0x08a3` | `u8[2]` | `0x00, 0x00` | `0, 0` | `0x08a2=0`（使用 `Filter_IsHandLift` 抬手强阻尼），`0x08a3=0`（使用折返线性外推混合模式 Mode 0） |
| **TouchAction / Click** | `Flash+0x0902..0x0905` | `u16, u8[3]` | `150, 20, 20, 10` | `150ms, 2mm, 2mm, 1mm` | Click 最大持续时间 $150\text{ ms}$，Click 冻结半径 $2.0\text{ mm}$，Click-to-Flick 每帧最大追赶步长 $1.0\text{ mm}$ |
| **动态功能开关** | `Flash+0x0906` (`Dyn[0x80]`) | `u8` | `0x03` | `bit0=1, bit1=1` | 开启 XY IIR/JIT 坐标滤波（`bit0=1`）与正常手指模式检测（`bit1=1`）；默认关闭手套（`bit2=0`）与被动笔（`bit3=0`） |
