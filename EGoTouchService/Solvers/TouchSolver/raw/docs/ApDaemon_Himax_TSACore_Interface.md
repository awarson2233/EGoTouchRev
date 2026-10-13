# `ApDaemon.dll` ↔ `himax_thp_drv.dll` ↔ `TSACore.dll` 完整调用接口、数据结构与执行时序分析报告（`W273AS2700` / `GaokunHimaxCSOT`）

## 模块概览与基址

| 模块名称 | 镜像基址 (`Image Base`) | 核心职责 |
| :--- | :--- | :--- |
| `ApDaemon.dll` | `0x180000000` | Windows 用户态触控守护服务（包含 `ThpBase`、`TsaFrame`、`LibAfe`、`TSACoreFuncTable` 及 VHF HID 触控/手写笔上报） |
| `himax_thp_drv.dll` | `0x180000000` | Himax HX83102J AFE 硬件抽象层（双通道 SPI Master/Slave 帧采集、行峰值死区抑制、逐像素增益校准、9×9 手写笔解包、IC 寄存器环形命令下发） |
| `TSACore.dll` | `0x6ba40000` | 华为触控算法核心（互容/自容/主动笔算法处理、安全基线跟踪、跳频控制、IDLE 低功耗与校准状态机） |
| `TSAPrmt.dll` | 由 `TSACore.dll` 加载 | 项目参数表预设库（`W273AS2700` 对应 `GaokunHimaxCSOT` 参数表） |

---

## 1. 初始化时序（`ApDaemon.dll` → `himax_thp_drv.dll` & `TSACore.dll`）

### 1.1 端到端完整调用顺序

```mermaid
sequenceDiagram
    participant APD as ApDaemon.dll (ThpBase / TsaFrame)
    participant HMX as himax_thp_drv.dll
    participant TSA as TSACore.dll

    Note over APD,HMX: 阶段 1：动态库加载与函数表绑定
    APD->>HMX: LibAfe::InitializeLibAfeHimaxAfeTable (0x180015850)<br/>LibAfe::InitializeHimaxAfeOps (0x1800164d0)
    APD->>TSA: TSACoreFuncTable_Init (0x18001da40)

    Note over APD,HMX: 阶段 2：AFE 硬件初始化与 OTP 项目识别
    APD->>HMX: thp_afe_open_project() (0x180020690)
    HMX->>HMX: SpiFuncInit (0x180046650), Ptr_SpiSetBlock(0/1, 1), Ptr_SpiSetTimeout(0/1, 200)
    HMX->>HMX: getProjectID_OTP() (0x18001de60) -> "W273AS2700"
    HMX->>HMX: 配置 W273AS2700 全局参数 (RX=60, TX=40, CSOT 手写笔表, 像素增益表)
    HMX->>HMX: configParamByCfg() (0x18001e830) -> master_suffix_begin=4807, full_size=5063
    HMX->>HMX: Init_Buffers_And_Registers() (0x18001f420) -> projectInitState = 4

    Note over APD,TSA: 阶段 3：TSA 项目初始化、AFE 规范版本与硬件能力注册
    APD->>HMX: thp_afe_get_project_id() (0x18001f610) -> "W273AS2700"
    APD->>HMX: thp_afe_hal_spec_version() (0x180037070) -> "1.3.0"
    APD->>HMX: ThpBase::GetHwCap (0x1800db410) -> thp_afe_get_hw_cap() (0x18002ded0) -> &g_AfeHwCap (0x180195320)
    APD->>TSA: ThpBase::InitTsaPrmt (0x1800e4550) -> TSA_InitAFEProject_UI("W273AS2700", "1.3.0", pHwCap) (0x6ba99dc7)
    TSA->>TSA: AFE_SetAFESpecVersion("1.3.0") (0x6bb2c94d) -> g_thpAFEStruct.specVer = {1,3,0,0}
    TSA->>TSA: AFE_SetCaps(pHwCap) (0x6bb2cb53) -> AFE_SetCapsTrans & AFE_SetCapsNormal
    TSA->>TSA: TSA_MSInitProject("W273AS2700", "0000") (0x6ba90994) -> TSA_InitCommonUI (0x6ba99ce0)

    Note over APD,TSA: 阶段 4：屏幕/触控分辨率、控制位掩码与特性字配置
    APD->>TSA: ThpBase::InitTsa (0x1800e6340) -> TSA_SetScreenResolution(width, height) (+0x3d0)
    APD->>TSA: TSA_SetTouchResolution(width, height) (+0x3d8)
    APD->>APD: shared_state->dwCtrlFlags |= 0x00040003 (置位 Bit 0, 1, 18)
    APD->>TSA: uFeat = TSA_GetFeatures() (+0x2b0); TSA_SetFeatures(uFeat & 0xffffffc0) (+0x2b8)

    Note over APD,TSA: 阶段 5：TsaFrame PreInit / Init / LastInit 与手写笔校准加载
    APD->>TSA: TsaFrame::PreInit (0x180083910) -> TsaFrame::InitAfeCap (0x18008f780)
    APD->>TSA: TsaFrame::Init (0x180089880) -> TsaFrame::PropVersion (0x180090d90) -> TSA_SetAFEInfo(&info) (0x6ba9f90e)
    APD->>TSA: TsaFrame::InitAsaCalibration (0x180087700) -> TSA_SetStylusCalibrationData (0x674 B) & TSA_SetStylusOemCalibrationData (0x64 B)
    APD->>TSA: TSA_GetIDLETouchEnterThreshold / LeaveThreshold / BaselineUpdateInterval / ScanRate
    APD->>HMX: thp_afe_set_idle_touch_threshold / set_baseline_update_interval / set_idle_scan_rate
    APD->>TSA: TsaFrame::LastInit (0x18008aa40) -> TSA_GetFeatures()

    Note over APD,HMX: 阶段 6：启动 AFE 硬件采集并开启 SPI 中断
    APD->>HMX: ThpBase::ProcessInit (0x1800d5ef0) -> thp_afe_start() (0x180029360)
    HMX->>HMX: 拉高 Reset, 写 0x13->reg[0x0d], 0x31->reg[0x13], Init_Buffers_And_Registers(), hx_sense_on(1), Ptr_SpiIntOpen(0/1)
```

### 1.2 各阶段关键实现细节

1. **函数表动态解析**：
   - `LibAfe::InitializeLibAfeHimaxAfeTable` (`0x180015850`) 与 `LibAfe::InitializeHimaxAfeOps` (`0x1800164d0`) 加载 `himax_thp_drv.dll` 并填充 `fnList` 虚表（`init_FuncList1..10` 与 `InitializeHimaxAfeCallbackWrappers`）。
   - `TSACoreFuncTable_Init` (`0x18001da40`) 加载 `TSACore.dll` 并将所有 `TSA_*` 导出函数绑定至 `TSACoreFuncTable`（由 `TsaFrame + 0xf8` 指向）。注意：`TSACoreFuncTable` 在偏移 `+0x00` 处有 `void* vtable`，因此结构体偏移 `+0xN` 对应 `TSACoreFuncTable_Init` 中的槽位索引 `(N / 8) - 1`。

2. **`thp_afe_open_project` (`0x180020690`，位于 `himax_thp_drv.dll`)**：
   - 调用 `SpiFuncInit()` (`0x180046650`) 加载 `SpiController.dll`，调用 `Ptr_SpbModuleInit()`，设置阻塞模式 `Ptr_SpiSetBlock(0/1, 1)`、超时 `Ptr_SpiSetTimeout(0/1, 200)` 并复位 IC（`Ptr_SpiSetReset`）。
   - 调用 `getProjectID_OTP()` (`0x18001de60`) 读取 10 字节项目名。当匹配 `"W273AS2700"`（`GaokunHimaxCSOT`）时，配置以下全局参数：
     - `_RX_NUM = 60` (`0x3c`)，`_TX_NUM = 40` (`0x28`)
     - `frame_head = 7`（字节），`half_frame_matrix = 0x960`（`2400` 个 `uint16_t` = `4800` 字节）
     - `frame_slave_matrix_full_size = 0x153`（`339` 字节），`matrix_header = 7`（字节），`g_SlaveStylusF1Offset = 173`（`0xad` 字节）
     - `_g_AfeFreqCommand0 = 0x004b` (`75`)，`_g_AfeFreqCommand1 = 0x0071` (`113`)
     - `hx_mode = 0x105`，`g_PanelInfoNameIndex = 1`（对应 `PTR_s_UNKOWN_180068668` 中的 `"HX83102J"`）
     - 选用 CSOT 主动笔参数表（`g_CsotStylusTable_Pen1_F0..F3`、`g_CsotStylusTable_Pen5_F0..F3`）与像素增益表（`g_PixelGainTable1Ptr = PixelGainTable1`，`g_PixelGainTable2Ptr = PixelGainTable2`）。
   - 校验 Flash CRC（`flashCheckCRCRaw @ 0x180004860`）与固件版本（`0x21500` / `0x21501`），调用 `configParamByCfg()` (`0x18001e830`) 计算：
     - `master_suffix_begin = frame_head + half_frame_matrix * 2 = 7 + 4800 = 4807` (`0x12c7` 字节)
     - `frame_master_matrix_full_size = frame_head + (half_frame_matrix + 0x80) * 2 = 7 + 5056 = 5063` (`0x13c7` 字节)
   - 调用 `Init_Buffers_And_Registers()` (`0x18001f420`)：清零 SPI 命令寄存器区 `0x10007550`（`0x50` 字节 = 5 槽位 × 16 字节）与 `0x1000753c`（4 字节），将 `g_FinalFrameOutputBuffer`、`g_PreviousMasterFrameCache`、`g_MasterSentinelFrameBuffer` 填充为 `0x7ffe`（`32766` 基准中心值），清零 `g_DefaultStylusMatrixF0/F1`，并置 `projectInitState = 4`。

3. **`ThpBase::GetHwCap` (`0x1800db410`) 与行列转置判定机制**：
   - `thp_afe_get_hw_cap()` (`0x18002ded0`) 填充并返回 `&g_AfeHwCap` (`0x180195320`，`0x60` 字节，原始值 `numCol = 60, numRow = 40`)。
   - **关键细节**：`ThpBase::GetHwCap` 将 `g_AfeHwCap` 拷贝到 `ThpBase + 0x1db0` 后，若 `*(int*)(this + 0x1ed4) - 1U < 2`（即方向配置为 `1` 或 `2`），会在本地副本中**交换 `numCol` (`+0x00`) 和 `numRow` (`+0x02`)**（变为 `numCol = 40, numRow = 60`）再传给 `TSACore.dll`。
   - 在 `TSACore.dll` 的 `AFE_SetCaps` (`0x6bb2cb53`) 中：
     - `AFE_SetCapsTrans` (`0x6bb2fbb0`) 检查 `pHwCap->numCol <= pHwCap->numRow`，若成立则置 `g_thpAFEStruct.isTransposed = 1` (`0x6bc9fd68`)。
     - `AFE_SetCapsNormal` (`0x6bb2eef0`) 在 `isTransposed == 1` 时把 `numCol` 和 `numRow` 再次交换回 `numCol = 60, numRow = 40` 存入 `g_thpAFEStruct.caps` (`0x6bc9fdc8`)；同时在后续每帧调用 `AFE_SetFrameNormal` (`0x6bb2f0ef`) 时，对 `pRawData`、`pData1`、`pData2` 矩阵执行行列转置拷贝。

4. **`TSA_InitAFEProject_UI` (`0x6ba99dc7`，位于 `TSACore.dll`)**：
   - 参数：`RCX = "W273AS2700"`，`RDX = "1.3.0"`，`R8 = pHwCap`。
   - 内部依次调用：
     1. `AFE_SetAFESpecVersion("1.3.0")` (`0x6bb2c94d`) -> 解析为 `g_thpAFEStruct.specVer = {1, 3, 0, 0}` (`0x6bc9fd60`)。
     2. `AFE_SetCaps(pHwCap)` (`0x6bb2cb53`) -> 填充 `g_thpAFEStruct.caps` (`0x6bc9fdc8`) 并置 `hasCaps = 1`。
     3. `TSA_MSInitProject("W273AS2700", "0000")` (`0x6ba90994`) -> 从 `TSAPrmt.dll` 加载 `"W273AS2700"` (`GaokunHimaxCSOT`) 参数表并调用 `TSA_InitCommonUI` (`0x6ba99ce0`)。

5. **`TsaFrame::Init` (`0x180089880`，位于 `ApDaemon.dll`)**：
   - 调用 `TsaFrame::PropVersion` (`0x180090d90`)：在栈上构造 `0x60` 字节的 `THP_AFE_INFO_STRUCT`（三个 32 字节字符串均填 `"afehal"`），调用 `TSA_SetAFEInfo(&info)` (`0x6ba9f90e` -> `AFE_SetInfo @ 0x6bb2cb3e`)。
   - 调用 `TsaFrame::InitAsaCalibration` (`0x180087700`)：读取 `"C:\\ProgramData\\Huawei\\HuaweiThp\\AsaCalibration_1.bin"`（`0x674` = `1652` 字节）传入 `TSA_SetStylusCalibrationData(buf, 0x674, 1)` (`+0x738`)，并读取 OEM 校准数据（`0x64` = `100` 字节）传入 `TSA_SetStylusOemCalibrationData(buf, 0x64, 1)` (`+0x750`)。
   - 从 `TSACore.dll` 查询 IDLE 阈值与周期（`TSA_GetIDLETouchEnterThreshold` / `LeaveThreshold` / `BaselineUpdateInterval` / `ScanRate`），并调用 `himax_thp_drv.dll` 的 `thp_afe_set_idle_touch_threshold`、`thp_afe_set_baseline_update_interval`、`thp_afe_set_idle_scan_rate` 下发给 IC。

---

### 1.3 初始化结构体精确 C 定义与 `W273AS2700` 参数值

#### `THP_AFE_HW_CAP_STRUCT`（`0x60` = 96 字节，`himax_thp_drv.dll` 中位于 `0x180195320`，`TSACore.dll` 中位于 `0x6bc9fdc8`）

```c
#pragma pack(push, 1)
typedef struct _THP_AFE_HW_CAP_STRUCT {
    uint16_t  numCol;                 // +0x00: 60 (0x003c, _RX_NUM)
    uint16_t  numRow;                 // +0x02: 40 (0x0028, _TX_NUM)
    uint8_t   numButton;              // +0x04: 0
    uint8_t   rxDirection;            // +0x05: 0
    uint8_t   rxChannel;              // +0x06: 8 (0x08)
    uint8_t   rxSlotLayout;           // +0x07: 0 (0x00)
    uint16_t  pitchSizeNum;           // +0x08: 5300 (0x14b4, 通道间距 5300 um = 5.30 mm)
    uint8_t   numScanFreq;            // +0x0a: 2
    uint8_t   pad_0b[5];              // +0x0b..+0x0f: 对齐填充
    uint16_t *pScanFreqList;          // +0x10: -> { 75 (0x004b), 113 (0x0071) } (g_AfeFreqCommand0/1)
    uint8_t   numScanRate;            // +0x18: 1
    uint8_t   pad_19[7];              // +0x19..+0x1f: 对齐填充
    uint16_t *pScanRateList;          // +0x20: -> { 120 (0x0078) } (120 Hz)
    uint32_t  featureNoiseDetect;     // +0x28: 2 (THP_AFE_NOISE_DETECT_ALL_FREQ)
    uint32_t  featureFreqHop;         // +0x2c: 2
    uint32_t  featureCalibration;     // +0x30: 2
    uint32_t  featureWakeupGesture;   // +0x34: 0
    uint32_t  sensorArch;             // +0x38: 3
    uint32_t  sensorPattern;          // +0x3c: 6
    uint32_t  stylusProtocol;         // +0x40: 2 (HPP 3.0 主动笔协议)
    uint8_t   stylusScanFreqNum;      // +0x44: 1
    uint8_t   pad_45[3];              // +0x45..+0x47: 对齐填充
    uint16_t *pStylusScanFreqList;    // +0x48: -> { 209 (0x00d1), 0 (0x0000) }
    uint32_t  featureSideTouch;       // +0x50: 0
    uint8_t   forceNum;               // +0x54: 0
    uint8_t   pad_55[3];              // +0x55..+0x57: 对齐填充
    uint32_t  stylusDataType;         // +0x58: 2 (二维 60x40 解包手写笔矩阵 pData1/pData2)
    uint8_t   numColAfterTied;        // +0x5c: 0
    uint8_t   numRowAfterTied;        // +0x5d: 0
    uint8_t   pad_5e[2];              // +0x5e..+0x5f: 对齐填充
} THP_AFE_HW_CAP_STRUCT;              // 总大小: 0x60 (96) 字节
#pragma pack(pop)
```

#### `THP_AFE_INFO_STRUCT`（`0x60` = 96 字节，`himax_thp_drv.dll` 中位于 `0x1801954a0`，`TSACore.dll` 中位于 `0x6bc9fe28`）

```c
#pragma pack(push, 1)
typedef struct _THP_AFE_INFO_STRUCT {
    char vendorName[32];              // +0x00: thp_afe_get_info 中为 "W273AS2700" (TsaFrame::PropVersion 传入 "afehal")
    char productName[32];             // +0x20: thp_afe_get_info 中为 "HX83102J"   (TsaFrame::PropVersion 传入 "afehal")
    char version[32];                 // +0x40: thp_afe_get_info 中为 "3.0.0.0501" (TsaFrame::PropVersion 传入 "afehal")
} THP_AFE_INFO_STRUCT;                // 总大小: 0x60 (96) 字节
#pragma pack(pop)
```

---

## 2. 帧数据采集与解包（`ApDaemon.dll` → `himax_thp_drv.dll`: `thp_afe_get_frame`）

在 `ThpBase::Process` (`0x1800d7670`) 主循环中，`ApDaemon.dll` 每帧调用 `ThpBase::GetFrame` (`0x1800dacd0`)，进而调用 `himax_thp_drv.dll` 的 `thp_afe_get_frame()` (`0x180030cf0`)。

### 2.1 双通道 SPI 帧读取与包长（`W273AS2700`）

`thp_afe_get_frame()` 通过 `SpiController.dll` 读取两个独立的 SPI 通道数据包：
1. **Master SPI 数据包（SPI Bus 0 —— 互容触控矩阵 + 尾部状态字）**：
   - 调用：`Ptr_SpiGetFrame(0, master_frame, frame_master_matrix_full_size)`
   - `W273AS2700` 包总长度：**`5063` 字节 (`0x13c7`)**
2. **Slave SPI 数据包（SPI Bus 1 —— 主动笔 F0/F1 9×9 局部窗口）**：
   - 调用：`Ptr_SpiGetFrame(1, slave_frame, frame_slave_matrix_full_size)`
   - `W273AS2700` 包总长度：**`339` 字节 (`0x153`)**

### 2.2 Master SPI 数据包结构（`5063` 字节）与互容信号预处理流水线

| 字节偏移范围 | 长度（字节） | 符号 / 计算公式 | 内容说明 |
| :--- | :--- | :--- | :--- |
| `0x0000..0x0004` | `5` | `master_frame[0..4]` | SPI 传输前导头 |
| `0x0005..0x0006` | `2` | `master_frame[5..6]` | 参与 16 位校验和计算的帧头字（`frame_head = 7` 总头部字节） |
| `0x0007..0x12c6` | `4800` (`0x12c0`) | `half_frame_matrix * 2` (`2400` 个 `uint16_t`) | **互容原始矩阵 `60 × 40`**（`_RX_NUM = 60`, `_TX_NUM = 40`，以 `0x7ffe`/`0x8000` 为基准中心的小端 16 位采样） |
| `0x12c7..0x13c6` | `256` (`0x0100`) | `master_suffix_begin = 4807` (`128` 个 `uint16_t`) | **Master 尾部元数据与调试状态块**，原样拷贝至 `g_MasterSuffixDebugCache` (`0x180195390`, `256` 字节) |

> **关于自容数据 (`pLineData`) 的重要说明**：在正常运行时帧采集（`thp_afe_get_frame`）中，`himax_thp_drv.dll` **不会**填充 `g_FrameOut.pLineData`（偏移 `+0x18` 恒为 `NULL`）。自容/行数据仅在产线自检函数 `thp_afe_get_inspect_line_data` (`0x18001a560`) 中使用。

#### Master 互容帧校验与预处理四步流水线：
1. **16 位校验和与全零检测**（`thp_compute_checksum16 @ 0x18002fc90`）：
   - 从 `master_frame + 5` 开始，对 `(5063 - 5) >> 1 = 2529` 个 `uint16_t` 字求和，验证低 16 位是否为 `0` 且数据不全为 `0`。若失败，则将 `g_MasterSentinelFrameBuffer`（全 `0x7ffe`）拷入 `g_RealTimeFrameBuffer`，置 `g_FrameOut.status |= 0x40` 并直接返回。
2. **重复帧阻尼滤波**（`thp_afe_damp_repeated_master_frame @ 0x180030a50`）：
   - 当 `g_MasterFrameDampingEnable != 0` 时执行。
3. **逐行峰值跟踪与 15% 死区抑制**（`thp_afe_preprocess_frame_data @ 0x1800307d0`）：
   - 遍历 `40` 个 TX 行（每行 `LocalRX = 60` 个元素）：
     - 每个元素减去 `0x7ffe` 得到有符号偏移 `val = sample - 0x7ffe`，并找出当前行的最大值 `rowMax`。
     - 计算行死区阈值 `thr = (rowMax * 15) / 100`（行峰值的 `15%`）。
     - 对该行每个元素进行死区收缩（`[-thr, +thr]` 内归零，`> thr` 减去 `thr`，`< -thr` 加上 `thr`），再加回 `0x7ffe` 写回 `master_frame + 7`。
4. **空间倒序排列、逐像素增益校准与非线性死区压缩**：
   - 读取尾部状态块 `g_MasterSuffixDebugCache[10]`（字节偏移 `+20`）作为当前频率槽位 `freq_slot`：
     - `freq_slot == 0`：选用 `PixelGainTable1`（对应频率命令 `75`）
     - `freq_slot == 1`：选用 `PixelGainTable2`（对应频率命令 `113`）
   - 遍历 `i = 0 .. 2399`：
     - **倒序读取**：`raw = ((uint16_t*)(master_frame + 7))[2399 - i] - 0x8000`。
     - **逐像素增益缩放**：`scaled = (raw * (int16_t)gainTable[i] + 50) / 100`。
     - **分段非线性死区压缩**：
       - 若 `scaled <= -20`：当 `scaled < -80` 时 `out = scaled + 60`；否则 `out = -20 + ((scaled + 20) * 10 + 30) / 60`。
       - 若 `-20 < scaled < 80`：`out = (scaled * 10) / 80`。
       - 若 `scaled >= 80`：`out = scaled - 70`。
     - 加回 `+0x7ffe` 存入 `g_FinalFrameOutputBuffer[i]`，随后整体拷入 `g_RealTimeFrameBuffer` (`0x180170520`)，并令 `g_FrameOut.pRawData = g_RealTimeFrameBuffer`。

### 2.3 Master 尾部状态块关键字段（`g_MasterSuffixDebugCache` @ `0x180195390`）

| 字索引 (`uint16_t`) | 尾部块内字节偏移 | `master_frame` 偏移 | 含义及对 `g_FrameOut` / `g_StylusOut` 的影响 |
| :--- | :--- | :--- | :--- |
| `word[0]` | `+0x00` (`0`) | `4807` (`0x12c7`) | **IDLE / 重试标志**：若 `== 1`，置 `g_FrameOut.status |= 0x03`（Bit 0 & 1）且 `g_FrameRetryEnableFlag = 1`。 |
| `word[2]` | `+0x04` (`4`) | `4811` (`0x12cb`) | **跳频完成伴随字**：在 `thp_afe_process_stylus_status` 中，若 `freqShiftStatus != 0` 且 `curScanFreq == g_AfeCurrentFreqCommand`，置 `g_FrameOut.status |= 0x04` 并清零 `freqShiftStatus`。 |
| `word[3]` | `+0x06` (`6`) | `4813` (`0x12cd`) | **硬件校准完成魔数**：若 `== 0x00bb`，置 `g_FrameOut.status |= 0x08`。 |
| `word[6]` | `+0x0c` (`12`) | `4819` (`0x12d3`) | **固件复位 / SOS 异常标志**：若 `!= 0`，置 `g_FrameOut.status |= 0x40`。 |
| `word[10]` | `+0x14` (`20`) | `4827` (`0x12db`) | **当前扫描频率槽位 (`0` 或 `1`)**：选择 `PixelGainTable1/2`，并设置 `g_FrameOut.curScanFreq = g_AfeFreqTablePtr[word[10]]`（槽位 0 为 `75`，槽位 1 为 `113`）。 |
| `word[14]`, `word[16]` | `+0x1c` (`28`), `+0x20` (`32`) | `4835`, `4839` | **充电器 / 交流噪声指标**：若 `word[14] > 1499` 或 `word[16] > 1499` (`0x5db`)，置 `g_FrameOut.status |= 0x20`。 |
| `word[24]` | `+0x30` (`48`) | `4855` (`0x12f7`) | **主动笔 F0 噪声指标**：若 `> 2999` (`0xbb7`)，丢弃当前 Slave 笔数据（改用全零默认矩阵）并在 `thp_afe_process_stylus_status` 中触发主动笔跳频请求。 |
| `word[25]` | `+0x32` (`50`) | `4857` (`0x12f9`) | **主动笔 F1 噪声指标**：与 `word[24]` 配合触发主动笔跳频请求（`g_StylusOut.status |= 0x40` 或 `0x20`）。 |
| `word[31]` | `+0x3e` (`62`) | `4869` (`0x1305`) | **主动笔当前频率索引**：`0` 或 `1`，用于与 `g_StylusFreqSwitchTargetIndex` 比对。 |

### 2.4 Slave SPI 数据包结构（`339` 字节）与 9×9 手写笔窗口解包

| 字节偏移范围 | 长度（字节） | 符号 / 计算公式 | 内容说明 |
| :--- | :--- | :--- | :--- |
| `0x0000..0x0004` | `5` | `slave_frame[0..4]` | SPI 传输前导头 |
| `0x0005..0x0006` | `2` | `slave_frame[5..6]` | 参与 16 位校验和计算的帧头字（`matrix_header = 7` 总头部字节） |
| `0x0007..0x00ac` | `166` (`0x00a6`) | `83` 个 `uint16_t` | **主动笔 F0 数据块**：`word[0]` = TX 中心行 + 4（无笔时为 `0xff`），`word[1]` = RX 中心列（无笔时为 `0xff`），`word[2..82]` = `81` 个 `uint16_t`（`9 × 9` 局部窗口采样） |
| `0x00ad..0x0152` | `166` (`0x00a6`) | `g_SlaveStylusF1Offset = 173` (`83` 个 `uint16_t`) | **主动笔 F1 数据块**：`word[0]` = TX 中心行 + 4，`word[1]` = RX 中心列，`word[2..82]` = `81` 个 `uint16_t`（`9 × 9` 局部窗口采样） |

#### 9×9 窗口解包流程（`thp_unpack_9x9_block_to_matrix @ 0x18002fa80`）：
1. 对 `slave_frame + 5` 起的 `167` 个 `uint16_t` 字执行 16 位校验和验证。
2. 将 `g_StylusUnpackedMatrixBuffer` (`0x180172aa0`，共 `9600` 字节 = 两个连续的 `60 × 40` `int16_t` 矩阵) 清零。
3. 调用 `thp_unpack_9x9_block_to_matrix((int16_t*)(slave_frame + 7), &g_StylusUnpackedMatrixBuffer[0])` 解包 **F0**：
   - 若 `block[0] == 0xff && block[1] == 0xff`，直接返回 `0`。
   - 否则计算起始行 `tx_start = (int8_t)block[0] - 4`、中心列 `rx_center = (int8_t)block[1]`，将 `9 × 9` 窗口内的 `81` 个采样点按**倒序线性下标** `2400 - 1 - (r * 60 + c)` 写入 `60 × 40` 矩阵。
4. 调用 `thp_unpack_9x9_block_to_matrix((int16_t*)(slave_frame + 173), &g_StylusUnpackedMatrixBuffer[2400])` 解包 **F1**（位于 `+4800` 字节偏移处）。
5. 令 `g_StylusOut.pData1 = &g_StylusUnpackedMatrixBuffer[0]` (`0x180172aa0`)，`g_StylusOut.pData2 = &g_StylusUnpackedMatrixBuffer[2400]` (`0x180173d60`)。
6. 调用 `thp_afe_process_stylus_status(&g_FrameOut, &g_StylusOut, ...)` (`0x180030020`) 更新 `g_StylusOut` 状态（`curDetectFreq = 209`，`curScanRate = 161`，未连接/噪声拒绝时 `status = 0x06`，连接有效时 `status = 0x12`，跳频请求时附加 `0x20`/`0x40`）以及 `g_FrameOut.curScanFreq` / `curScanRate = 120`。

---

## 3. 帧处理调用接口（`ApDaemon.dll` → `TSACore.dll`）

### 3.1 单帧处理时序（`TsaFrame::Process @ 0x1800832a0`）

1. **帧状态位检查与 `dwCtrlFlags` 预更新**：
   - 通过 `afe_driver->vtable[+0x120]()` 获取 `pFrame` (`THP_AFE_FRAME_STRUCT*`)，检查 `pFrame->status` (`+0x34`)：
     - **Bit 6 (`0x40`, FW 复位 / SOS)**：若置位，调用 `TsaFrame::ProcessSos` (`0x180097260`) 触发硬件复位恢复，**本帧不进入 `TSACore.dll`**。
     - **Bit 3 (`0x08`, 校准完成)**：调用 `TsaFrame::UpdateStatusCalibration` (`0x1800979e0`) -> 置 `dwCtrlFlags |= 0x00000008`。
     - **Bit 4 (`0x10`, 手势事件)**：调用 `FUN_180098170` -> 通知 `algo_ctrl[0x38](0x10)`。
     - **Bit 2 (`0x04`, 跳频完成)**：调用 `FUN_1800981a0` -> 置 `dwCtrlFlags |= 0x00000010`。
     - **Bit 5 (`0x20`, 充电器/交流噪声)**：调用 `FUN_1800981d0` -> 同步置位或清除 `dwCtrlFlags` 的 `0x00020000`（Bit 17）。
     - **Bit 0 (`0x01`, IDLE 状态)**：调用 `TsaFrame::UpdateStatusIdle` (`0x180098230`) -> 当 `status & 1` 时置 `dwCtrlFlags |= 0x00000020`（Bit 5）；当退出 IDLE（从 `1` 变 `0`）时，清除 `0x00000020` 并置一次性标志 `dwCtrlFlags |= 0x00000004`（Bit 2）。
2. **调用 `TSA_ProcessingAFEFrame`（`TsaFrame_CallProcessingAFEFrame @ 0x180081900`）**：
   - 调用 `TSA_GetScenario()` (`tsa_func_table + 0x398`) 存入 `shared_state->scenario` (`+0x08`)。
   - 调用 `afe_driver->vtable[+0x78]()` (`ThpBase::GetTimeStamp @ 0x1800dad40`) 计算微秒时间戳 `timeStampUs = pFrame->timeStampSec * 1000000ULL + pFrame->timeStampUsec`，存入 `frame_ctx + 0x590`。
   - 读取 `dwCtrlFlags = *(uint32_t*)(shared_state + 0x00)`，调用 **`TSA_ProcessingAFEFrame(pFrame, dwCtrlFlags, timeStampUs)`** (`0x6ba9a7a7`)。
   - 调用返回后：
     - 调用 `TSA_IsNewSafeBaselineAvailable()` (`tsa_func_table + 0x608`, `0x6ba987c2`) 并通过 `afe_driver->vtable[+0x1e0]` 同步基线冻结状态。
     - **清除一次性控制位**：`*shared_state &= 0xf0ffffe0`（自动清零 Bit `0..4` 与 Bit `24..27`）。
     - 调用 `TSA_SetFeatures(*(uint32_t*)(shared_state + 0x04))` (`tsa_func_table + 0x2b8`, `0x6ba9f8cf`) 刷新算法特性掩码。

### 3.2 `TSA_ProcessingAFEFrame` 函数签名与 3 个参数详解（`0x6ba9a7a7`）

```c
int __fastcall TSA_ProcessingAFEFrame(
    const THP_AFE_FRAME_STRUCT *pFrame,      // RCX (参数 1: AFE 帧结构体指针)
    uint32_t                    dwCtrlFlags, // EDX (参数 2: 32位算法控制位掩码)
    uint64_t                    timeStampUs  // R8  (参数 3: 64位微秒级时间戳)
);
```

在 `TSACore.dll` 内部，`TSA_ProcessingAFEFrame` 先调用 `AFE_SetFrame(pFrame)` (`0x6bb2cb9d` -> `AFE_SetFrameTrans` + `AFE_SetFrameNormal`) 将外部帧深拷贝（并根据 `isTransposed` 转置）到全局变量 `g_thpAFEStruct` (`0x6bc9fd60`) 中，随后调用 `TSA_Processing((short *)&g_rawDataBuf, dwCtrlFlags, (void *)timeStampUs)` (`0x6ba9a725`)。

### 3.3 `dwCtrlFlags` 控制位掩码完整定义表（`shared_state + 0x00`）

| 位索引 | 十六进制掩码 | 生命周期 | `ApDaemon.dll` 设置位置 | `TSACore.dll` 内部含义 |
| :--- | :--- | :--- | :--- | :--- |
| **Bit 0** | `0x00000001` | 一次性（帧后 `& 0xf0ffffe0` 清零） | `ThpBase::InitTsa` (`0x1800e6340`), `ThpBase::ScreenOn` (`0x1800f56b0`) | **首帧 / 算法重初始化标志**：开机首帧或亮屏首帧重置滤波器与跟踪状态。 |
| **Bit 1** | `0x00000002` | 一次性（帧后 `& 0xf0ffffe0` 清零） | `ThpBase::InitTsa` (`0x1800e6340`), `ThpBase::ScreenOn` (`0x1800f56b0`) | **强制重置基线标志**：使用当前帧立即重建互容基线。 |
| **Bit 2** | `0x00000004` | 一次性（帧后 `& 0xf0ffffe0` 清零） | `TsaFrame::UpdateStatusIdle` (`0x180098230`) 退出 IDLE 时 | **退出 IDLE 转换标志**：通知算法刚刚从 IDLE 唤醒至 ACTIVE 扫描。 |
| **Bit 3** | `0x00000008` | 一次性（帧后 `& 0xf0ffffe0` 清零） | `TsaFrame::UpdateStatusCalibration` (`0x1800979e0`) 当 `frame->status & 0x08` | **AFE 硬件校准完成标志**：通知算法硬件自校准（`0x00bb`）已完成。 |
| **Bit 4** | `0x00000010` | 一次性（帧后 `& 0xf0ffffe0` 清零） | `FUN_1800981a0` (`0x1800981a0`) 当 `frame->status & 0x04` | **跳频完成标志**：通知算法请求的扫描频率切换已在硬件生效。 |
| **Bit 5** | `0x00000020` | 持续保持（IDLE 期间） | `TsaFrame::UpdateStatusIdle` (`0x180098230`) 当 `frame->status & 0x01` | **IDLE 模式激活标志**：指示当前帧采集于 IDLE 低功耗模式。 |
| **Bit 7** | `0x00000080` | 持续保持 | `FUN_180094e20` (`0x180094e20`) 当 `(posture & 0xfd) != 0` | **折叠/非平铺姿态标志**：设备处于折叠/帐篷等非展平状态时置 `1`，展平时清零。 |
| **Bit 8** | `0x00000100` | 持续保持（灭屏期间） | `ThpBase::ScreenOff` (`0x1800f61b0`) 置位，`ScreenOn` 清零 | **灭屏状态标志 (`ScreenOff`)**。 |
| **Bit 17** | `0x00020000` | 持续保持（有噪声期间） | `FUN_1800981d0` (`0x1800981d0`) 当 `frame->status & 0x20` | **充电器 / 交流噪声标志**：当 Master 尾部 `word[14]` 或 `word[16] > 1499` 时置 `1`。 |
| **Bit 18** | `0x00040000` | 持续保持 | `ThpBase::InitTsa` (`0x1800e6340`) | **TSA 算法使能常驻位**：正常运行时恒为 `1`。 |
| **Bit 23** | `0x00800000` | 持续保持（锁屏期间） | `FUN_1800968e0` (`0x1800968e0`) 解析 `"state:locked"` / `"state:unlocked"` | **系统锁屏状态标志**：锁屏时为 `1`，解锁时为 `0`。 |
| **Bits 24..27** | `0x0f000000` | 一次性（帧后 `& 0xf0ffffe0` 清零） | 外部命令处理函数 | **一次性命令触发位**。 |
| **Bits 28..29** | `0x30000000` | 持续保持 | `FUN_180094d80` (`0x180094d80`) (`0..3` 映射到高 2 位) | **屏幕旋转方向编码 (`0..3`)**：`0`=0°, `1`=90°, `2`=180°, `3`=270°。 |

---

### 3.4 帧结构体与 `g_thpAFEStruct` 逐字节 C 定义

#### `THP_AFE_STYLUS_FRAME_STRUCT`（`0x30` = 48 字节，`himax_thp_drv.dll` 中 `g_StylusOut` 位于 `0x180195668`，`TSACore.dll` 中位于 `0x6bca1ea8`）

```c
#pragma pack(push, 1)
typedef struct _THP_AFE_STYLUS_FRAME_STRUCT {
    int16_t  *pData1;                 // +0x00: F0 60x40 解包手写笔矩阵 (g_StylusUnpackedMatrixBuffer[0..2399], 4800 B)
    int16_t  *pData2;                 // +0x08: F1 60x40 解包手写笔矩阵 (g_StylusUnpackedMatrixBuffer[2400..4799], 4800 B)
    uint16_t  curDetectFreq;          // +0x10: 209 (0x00d1)
    uint16_t  curScanRate;            // +0x12: 161 (0x00a1)
    uint32_t  penId;                  // +0x14: 蓝牙/主动笔 ID (通过 thp_afe_set_stylus_id 设置)
    uint16_t  pressure;               // +0x18: 蓝牙/主动笔压力值 (通过 thp_afe_set_stylus_press 设置)
    uint16_t  reserved_1a;            // +0x1a: 填充 (0)
    uint32_t  status;                 // +0x1c: 状态位掩码: 0x06=未连接/噪声丢弃, 0x12=已连接有效
                                      //        | 0x20=强制立即切换频率, | 0x40=下一上行帧切换频率请求
    int16_t  *pTipRingLineData;       // +0x20: W273AS2700 中恒为 NULL (仅当 stylusDataType == 1 时使用)
    uint16_t  reqDetectFreq;          // +0x28: 请求切换的手写笔检测频率 (209 或槽位 1 命令字)
    uint16_t  reqScanRate;            // +0x2a: 请求切换的手写笔扫描率 (161 = 0x00a1 或 24 = 0x0018)
    uint32_t  reserved_2c;            // +0x2c: 填充 (0)
} THP_AFE_STYLUS_FRAME_STRUCT;        // 总大小: 0x30 (48) 字节
#pragma pack(pop)
```

#### `THP_AFE_FRAME_STRUCT`（`0x48` = 72 字节，`himax_thp_drv.dll` 中 `g_FrameOut` 位于 `0x180195620`，`TSACore.dll` 中位于 `0x6bc9fd70`）

```c
#pragma pack(push, 1)
typedef struct _THP_AFE_FRAME_STRUCT {
    uint32_t                     timeStampSec;      // +0x00: 时间戳（秒）
    uint32_t                     timeStampUsec;     // +0x04: 时间戳（微秒，0..999999）
    uint16_t                     frameIndex;        // +0x08: 递增帧序号 (g_FrameSequenceCounter++)
    uint8_t                      pad_0a[6];         // +0x0a..+0x0f: 对齐填充
    int16_t                     *pRawData;          // +0x10: -> g_RealTimeFrameBuffer (60x40 uint16_t/int16_t = 4800 B)
    int16_t                     *pLineData;         // +0x18: Himax 正常帧中为 NULL (最多 100 个 int16_t 自容数据)
    int16_t                     *pNoiseScope;       // +0x20: Himax 正常帧中为 NULL (最多 8 个 int16_t)
    uint16_t                    *pAllFreqNoiseData; // +0x28: Himax 正常帧中为 NULL (最多 2*numScanFreq 个 uint16_t)
    uint16_t                     curScanFreq;       // +0x30: 当前互容扫描频率 (75 或 113)
    uint16_t                     curScanRate;       // +0x32: 当前互容扫描率 (120 Hz)
    uint32_t                     status;            // +0x34: 帧状态位掩码:
                                                    //        0x01 = IDLE 模式
                                                    //        0x02 = 重试伴随位
                                                    //        0x04 = 跳频完成
                                                    //        0x08 = 硬件校准完成 (0x00bb)
                                                    //        0x10 = 手势事件
                                                    //        0x20 = 充电器 / 交流噪声 (>1499)
                                                    //        0x40 = FW 复位 / SOS / 校验和错误
    int16_t                     *pSideTouchData;    // +0x38: W273AS2700 中为 NULL
    THP_AFE_STYLUS_FRAME_STRUCT *pStylusFrame;      // +0x40: -> &g_StylusOut (0x180195668)
} THP_AFE_FRAME_STRUCT;                             // 总大小: 0x48 (72) 字节
#pragma pack(pop)
```

#### `TSACore.dll` 全局结构体 `g_thpAFEStruct`（起始地址 `0x6bc9fd60`，总长 `0x6088` 字节）

```c
#pragma pack(push, 1)
typedef struct _TSA_THP_AFE_GLOBAL_STRUCT {
    uint8_t                     specVer[4];                  // +0x0000 (0x6bc9fd60): { 1, 3, 0, 0 } ("1.3.0")
    uint8_t                     hasFrame;                    // +0x0004 (0x6bc9fd64): AFE_SetFrame 后置 1
    uint8_t                     hasCaps;                     // +0x0005 (0x6bc9fd65): AFE_SetCaps 后置 1
    uint8_t                     hasInfo;                     // +0x0006 (0x6bc9fd66): AFE_SetInfo 后置 1
    uint8_t                     pad_07;                      // +0x0007 (0x6bc9fd67): 填充
    uint32_t                    isTransposed;                // +0x0008 (0x6bc9fd68): 若输入 numCol <= numRow 则为 1（触发矩阵转置）
    uint32_t                    pad_0c;                      // +0x000c (0x6bc9fd6c): 填充
    THP_AFE_FRAME_STRUCT        frame;                       // +0x0010 (0x6bc9fd70): 0x48 字节（内部指针重定向至下方静态缓冲区）
    uint8_t                     reserved_58[16];             // +0x0058 (0x6bc9fdb8): 0x10 字节保留区
    THP_AFE_HW_CAP_STRUCT       caps;                        // +0x0068 (0x6bc9fdc8): 0x60 字节 (numCol=60, numRow=40)
    THP_AFE_INFO_STRUCT         info;                        // +0x00c8 (0x6bc9fe28): 0x60 字节
    int16_t                     afeGridData[3984];           // +0x0128 (0x6bc9fe88): 0x1f20 字节 (7968 B; 存放 60x40 = 2400 个 int16_t)
    int16_t                     afeLineData[100];            // +0x2048 (0x6bca1da8): 0x00c8 字节 (200 B; 自容行数据缓冲区)
    int16_t                     afeNoiseScope[8];            // +0x2110 (0x6bca1e70): 0x0010 字节 (16 B)
    uint16_t                    afeAllFreqNoise[16];         // +0x2120 (0x6bca1e80): 0x0020 字节 (32 B)
    uint16_t                    afeScanFreqList[4];          // +0x2140 (0x6bca1ea0): 0x0008 字节 ({ 75, 113, 0, 0 })
    THP_AFE_STYLUS_FRAME_STRUCT afeStylusFrame;              // +0x2148 (0x6bca1ea8): 0x0030 字节
    int16_t                     afeStylusData1[3584];        // +0x2178 (0x6bca1ed8): 0x1c00 字节 (7168 B; 存放 F0 60x40 = 2400 个 int16_t)
    int16_t                     afeStylusData2[3584];        // +0x3d78 (0x6bca3ad8): 0x1c00 字节 (7168 B; 存放 F1 60x40 = 2400 个 int16_t)
    int16_t                     afeStylusTipRingLine[128];   // +0x5978 (0x6bca56d8): 0x0100 字节 (256 B)
    int16_t                     afeSideTouchData[768];       // +0x5a78 (0x6bca57d8): 0x0600 字节 (1536 B)
    uint16_t                    afeStylusScanFreqList[4];    // +0x6078 (0x6bca5dd8): 0x0008 字节 ({ 209, 0, 0, 0 })
    uint16_t                    afeScanRateList[4];          // +0x6080 (0x6bca5de0): 0x0008 字节 ({ 120, 0, 0, 0 })
} TSA_THP_AFE_GLOBAL_STRUCT;                                 // 总大小: 0x6088 字节
#pragma pack(pop)
```

---

## 4. 算法结果提取与闭环硬件控制（`TSACore.dll` → `ApDaemon.dll` → `himax_thp_drv.dll`）

### 4.1 帧后结果提取（`FUN_18008e350` / `TsaFrame::ObInput` / `FUN_18008bf60`）

`TsaFrame_CallProcessingAFEFrame` 执行完毕后，`ApDaemon.dll` 调用 `FUN_18008e350` (`0x18008e350`) 将算法结果提取至 `TsaOutput` (`frame_ctx = TsaFrame + 0xe8`)：
- **多指触控缓冲区**：`frame_ctx + 0x000`（通过 `FUN_1800a5aa0` 访问）
- **主动笔缓冲区**：`frame_ctx + 0x51c`（通过 `FUN_1800a5b00` 访问，每帧由 `TsaOutput::ClearStylusdata @ 0x1800a63f0` 清零）
- **控制与闭环状态缓冲区**：`frame_ctx + 0x55c`（通过 `FUN_1800a5b60` 访问）

#### A. 多指触控坐标提取（`TsaFrame::ObInput @ 0x18008d920` -> `TsaFrame::ObInputPeak @ 0x18008c460` -> `FUN_18008c120 @ 0x18008c120`）

| `TSACoreFuncTable` 偏移 | `TSACore.dll` 导出函数 (`地址`) | 返回类型 | `TsaOutput` (`frame_ctx`) 存储位置 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| `+0x188` | `TSA_RptTouchNum` (`0x6baa2730`) | `uint8_t` | `frame_ctx + 0x194`（上限截断为 `10`） | 当前有效触控点数（`0..10`） |
| `+0x198` | `TSA_RptTouchDown` (`0x6baa2462`) | `uint64_t` | 若 `(ret & 0x1f) != 0` 则 `point[i] + 0x00 = 3` | 触控点 `i` 按下/保持状态 |
| `+0x1a0` | `TSA_RptTouchUp` (`0x6baa28b2`) | `uint64_t` | 若 `(ret & 0x40) != 0` 则 `point[i] + 0x00 = 2` | 触控点 `i` 抬起状态 |
| `+0x270` | `TSA_RptTouchY` (`0x6baa2990`) | `uint16_t` | `point[i] + 0x04` (`rawY`) | 触控点 `i` 的 Y 坐标（VHF 上报时乘以 10 映射至 `0..16000`） |
| `+0x278` | `TSA_RptTouchX` (`0x6baa292a`) | `uint16_t` | `point[i] + 0x08` (`rawX`) | 触控点 `i` 的 X 坐标（VHF 上报时映射为 `16000 - rawX*10`） |
| `+0x248` | `TSA_RptTouchPressure` (`0x6baa27ca`) | `uint16_t` | `point[i] + 0x0c` | 触控点 `i` 压力/信号幅度 |
| `+0x1a8` | `TSA_RptTouchID` (`0x6baa25b8`) | `uint8_t` | `point[i] + 0x10` | 触控点 `i` 跟踪 ID（`0..9`） |
| `+0x1b0` | `TSA_RptTouchType` (`0x6baa284c`) | `uint8_t` | `point[i] + 0x14` | 触控点 `i` 类型（手指、手掌、手套等） |
| `+0x260` | `TSA_RptTouchMajor` (`0x6baa2664`) | `uint16_t` | `point[i] + 0x1c` | 主轴宽度 |
| `+0x240` | `TSA_RptTouchOrientation` (`0x6baa276a`) | `int8_t` | `point[i] + 0x20` | 方向角 |
| `+0x1b8..+0x288` | `TSA_RptTouchMajorCellNum` 等统计量 | `uint8_t`/`uint16_t` | `frame_ctx + 0x19c .. + 0x318` | 触控团块网格数、均值、峰值、形状等特征 |

在 `DriverThp_ReportPoints` (`0x18005b0e0`) 中，最多 10 个触控点被打包为两个 32 字节 VHF HID 报文（`pkt0 @ this + 0x62c`、`pkt1 @ this + 0x64c`，每包 5 个点，每点 6 字节：`byte[0] = (status == 3) | (contactId << 2)`，`uint16_t X = 16000 - rawX * 10`，`uint16_t Y = rawY * 10`，`pkt0[31] = 总点数`），通过 `service_callback->vtable[0]` 发送给内核 VHF 驱动。

#### B. 主动笔结果提取（`FUN_18008e350 @ 0x18008e3d5..0x18008e4e3`）

存入 `stylus_buf` (`frame_ctx + 0x51c`)：

| `TSACoreFuncTable` 偏移 | `TSACore.dll` 导出函数 (`地址`) | 返回类型 | `stylus_buf` 偏移 (`frame_ctx + 0x51c`) | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| `+0x498` | `TSA_GetStylusScanRateIndex` (`0x6baa1f8a`) | `uint8_t` | `+0x04` (`uint32_t`) | 请求的主动笔扫描率/模式索引（`0`=未激活，`!=0`=激活） |
| `+0x4a0` | `TSA_GetStylusFreqIndex` (`0x6baa1f54`) | `uint8_t` | `+0x08` (`uint32_t`) | 请求的主动笔检测频率索引 |
| `+0x4a8` | `TSA_GetStylusY` (`0x6baa2044`) | `uint16_t` | `+0x1c` (`uint32_t`) | 笔尖 Y 坐标 |
| `+0x4b0` | `TSA_GetStylusX` (`0x6baa2010`) | `uint16_t` | `+0x20` (`uint32_t`) | 笔尖 X 坐标 |
| `+0x4c8` | `TSA_GetStylusPressure` (`0x6baa1f1e`) | `uint16_t` | `+0x24` (`uint32_t`) | 笔尖压力（`0..4095`） |
| `+0x4b8` | `TSA_GetStylusTiltY` (`0x6baa1fdc`) | `int8_t` | `+0x28` (`int8_t`) | Y 轴倾角 |
| `+0x4c0` | `TSA_GetStylusTiltX` (`0x6baa1fa8`) | `int8_t` | `+0x29` (`int8_t`) | X 轴倾角 |
| `+0x4d0` | `TSA_GetStylusPointNum` (`0x6baa1f02`) | `uint8_t` | 循环 `i = 0..min(ret, 2)-1` | 手写笔点数（最多 2 点） |
| `+0x4d8` | `TSA_GetStylusPointStatus` (`0x6baa1ec4`) | `uint16_t` | `+0x34 + i*8` (`uint32_t`) | 笔尖状态标志（悬停、落笔、按键、橡皮擦） |
| `+0x4e0` | `TSA_GetStylusPointType` (`0x6baa1e86`) | `uint8_t` | `+0x30 + i*8` (`0x40` 或 `0x20`) | 手写笔工具类型 |

在 `DriverThp_ReportStylusPoints` (`0x180059f70`) 中，`stylus_buf` 被打包为 `this + 0x66c` 处的 13 字节 VHF HID 手写笔报文（`byte[0] = 0x08`，`byte[1]` = 状态位，`byte[2] = 0x80`，随后为 `X`、`Y`、`pressure`、`tiltX`、`tiltY`），通过 `service_callback->vtable[1]` 上报。

#### C. 闭环控制请求提取（`FUN_18008bf60 @ 0x18008bf60`）

存入 `ctrl_buf` (`frame_ctx + 0x55c`)：

| `TSACoreFuncTable` 偏移 | `TSACore.dll` 导出函数 (`地址`) | 返回类型 | `ctrl_buf` 偏移 (`frame_ctx + 0x55c`) | `TsaFrame::Process` 闭环动作 |
| :--- | :--- | :--- | :--- | :--- |
| `+0x200` | `TSA_RequestEnterIDLEMode` (`0x6baa22aa`) | `uint8_t` | `+0x18` (`uint8_t`) | 连续 5 帧为 `1` 时调用 `TsaFrame::ChangeToIdlemode` (`0x180093960`) -> `thp_afe_enter_idle` |
| `+0x1f8` | `TSA_RequestEarlySuspend` (`0x6baa228e`) | `uint8_t` | `+0x1a` (`uint8_t`) | 早期挂起请求标志 |
| `+0x1f0` | `TSA_RequestCalibration` (`0x6baa2272`) | `uint8_t` | `+0x19` (`uint8_t`) | 若 `!= 0`，调用 `afe_driver->vtable[+0x150]()` -> `thp_afe_start_calibration` |
| `+0x208` | `TSA_RequestEnterHighCurrentIdle` (`0x6baa22c6`) | `uint8_t` | `+0x1b` (`uint8_t`) | 高电流 IDLE 请求 |
| `+0x5e0` | `TSA_GetHighCurrentIdleRequest` (`0x6ba98756`) | `uint8_t` | `+0x1c` (`uint8_t`) | 由 `FUN_1800838e0` 检查并触发 `algo_ctrl[0x5e/0x5f]` |
| `+0x210` | `TSA_DetectedHwException` (`0x6baa222e`) | `uint32_t` | `+0x24` (`uint32_t`) | 硬件异常检测结果 |
| `+0x5f0` | `TSA_IsSafeBaselineEstablished` (`0x6ba9878e`) | `uint8_t` | `+0x1e` (`bool`: `ret == 0`) | 安全基线是否已建立 |
| `+0x5f8` | `TSA_GetProxScreenOffState` (`0x6ba987a6`) | `uint8_t` | `+0x1f` (`bool`: `ret != 0`) | 接近感应灭屏状态 |
| `+0x5e8` | `TSA_GetBaselineDebugInfo` (`0x6ba98770`) | `uint32_t` | `+0x20` (`uint32_t`), Bit 7 -> `+0x1d` | 基线调试字 |
| `+0x320` | `TSA_RequestSwitchScanRate` (`0x6ba9f86c`) | `int (uint8_t*)` | `+0x08` (`uint32_t`, 无请求时为 `-1`) | 请求切换互容扫描率索引（`TsaFrame::SetScanmode @ 0x1800945b0`） |
| `+0x328` | `TSA_RequestSwitchScanFreq` (`0x6ba9f83b`) | `int (uint8_t*)` | `+0x0c` (`uint32_t`, 无请求时为 `-1`) | 请求切换互容扫描频率索引（`TsaFrame::SetScanmode @ 0x1800945b0`） |

此外：
- `TsaFrame::UpdateScanState` (`0x180082b30`) 调用 `TSA_GetCurrentScanState()` (`+0x628`, `0x6ba987ee`) 同步扫描状态。
- `FUN_1800876c0` (`0x1800876c0`) 调用 `TSA_IsStylusCalibrationDataReady()` (`+0x728`, `0x6ba99bb7`)；若就绪则调用 `TsaFrame::UpdateAsaCalibrationBin` (`0x1800845c0` -> `TSA_GetStylusCalibrationData` `+0x730`) 与 `TsaFrame::UpdateAsaCalibrationOem` (`0x180086720` -> `TSA_GetStylusOemCalibrationData` `+0x748`) 将更新后的校准数据写回 `"C:\\ProgramData\\Huawei\\HuaweiThp\\AsaCalibration_<idx>.bin"`。

---

### 4.2 `himax_thp_drv.dll` 闭环控制与 `hx_send_command` 寄存器通信协议（`0x1800119a0`）

`himax_thp_drv.dll` 中所有闭环控制函数均通过 `hx_send_command(uint8_t cmd_id, uint64_t unused, uint8_t cmd_val)` (`0x1800119a0`) 向 HX83102J 发送 16 字节指令帧：
- **16 字节指令格式**：
   - `byte[0] = 0xA8`, `byte[1] = 0x8A`（小端魔数头 `0x8AA8`）
   - `byte[2] = cmd_id`, `byte[3] = 0x00`
   - `byte[4] = cmd_val`, `byte[5..13] = 0x00`
   - `uint16_t word[7]` (`byte[14..15]`)：16 位补码校验和（使 8 个 16 位小端字之和模 `0x10000` 为 `0`）。
- **5 槽位环形寄存器邮箱**：
   - 写入 AHB 地址 `0x10007550 + g_AfeCurrentFreqSlot * 0x10`（共 5 个槽位，每次递增 `(g_AfeCurrentFreqSlot + 1) % 5`），重写前 4 字节触发通知，并回读 16 字节校验是否写入成功。

| `himax_thp_drv.dll` 导出函数 (`地址`) | `cmd_id` | `cmd_val` | 功能说明 |
| :--- | :--- | :--- | :--- |
| `thp_afe_disable_freq_shift` (`0x180039420`) | `0x02` | `0x00` | 禁用固件自动跳频 |
| `thp_afe_force_to_scan_rate` (`0x18003f5f0`) | `0x04` | `rate_idx` | 强制切换互容扫描率索引 |
| `thp_afe_clear_stylus_status` (`0x180042ff0`) | `0x06` | `0x06` | 清除主动笔跳频/状态标志 |
| `thp_afe_reset_idle_baseline` (`0x18003cc80`) | `0x07` | `0x07` | 重置 IC 内部 IDLE 基线 |
| `thp_afe_enter_idle` (`0x18003d590`) | `0x08` | `0x08` | 控制 IC 进入 IDLE 低功耗扫描模式 |
| `thp_afe_set_idle_touch_threshold` (`0x18003b0d0`) | `0x09` | `thr` | 设置 IDLE 触控唤醒/退出阈值 |
| `thp_afe_start_calibration` (`0x1800364d0`) | `0x0A` | `0x0A` | 启动硬件基线自校准 |
| `thp_afe_set_idle_scan_rate` (`0x18003c370`) | `0x0B` | `rate` | 设置 IDLE 模式扫描率 |
| `thp_afe_force_to_freq_point` (`0x18003ec50`) | `0x0C` | `freq_idx` | 强制切换互容扫描频率点（`0` -> `75`, `1` -> `113`） |
| `thp_afe_enable_freq_shift` (`0x180038cf0`) | `0x0D` | `0x0D` | 使能固件自动跳频 |
| `thp_afe_force_exit_idle` (`0x18003e2b0`) | `0x10` | `0x10` | 强制退出 IDLE 模式回到正常扫描 |
| `thp_afe_set_hpp3_0_detect_freq_immediately` (`0x180041d80`) | `0x15` | `freq` | 立即切换 HPP 3.0 主动笔检测频率 |
| `thp_afe_set_stylus_connect` (`0x180044a10`) | `0x16` | `state` | 通知 IC 蓝牙/主动笔连接状态 |
| `thp_afe_enable_hpp3_0` (`0x180040f60`) | `0x17` | `0x11` | 开启 HPP 3.0 主动笔检测模式 |
| `thp_afe_disable_active_stylus` (`0x180041670`) | `0x17` | `0x00` | 关闭主动笔检测模式 |
