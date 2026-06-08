# Shared Memory ABI 文档

> EGoTouchRev Service → App 跨进程共享内存接口 | ABI 版本: 5 | 日期: 2026-06-05

---

## 1. 概述

EGoTouchService (Session 0) 通过 Windows Shared Memory 将实时帧数据推送给 EGoTouchApp (User Session)。使用 Triple-Buffer + Seqlock 协议保证无锁、无撕裂的读取。

### 1.1 共享内存对象清单

| 对象 | 名称 | 类型 | 大小 | 创建者 |
|------|------|------|------|--------|
| 帧数据 | `Global\EGoTouchSharedFrame` | File Mapping | ~256 KB | Service |
| Config 脏标记 | `Global\EGoTouchConfigDirty` | File Mapping | 4 bytes | Service/App |
| 帧就绪事件 | `Global\EGoTouchFrameReady` | Event | — | Service |
| 日志就绪事件 | `Global\EGoTouchLogReady` | Event | — | Service |
| 笔状态就绪事件 | `Global\EGoTouchPenStatusReady` | Event | — | Service |

### 1.2 安全模型

所有共享内存对象使用 Admin-only ACL (`D:P(A;;GA;;;SY)(A;;GA;;;BA)`):
- `SY` = SYSTEM (Service 运行账户)
- `BA` = Built-in Administrators (App 以管理员身份运行)

> :warning: App 必须以管理员权限运行才能打开共享内存。

---

## 2. SharedFrameBuffer — 三缓冲帧数据

### 2.1 内存布局

```
SharedTripleBuffer (totalSize ≈ 256 KB)
┌──────────────────────────────────────────────────────────────┐
│ SharedFrameAbiHeader  (headerSize bytes)                     │
│   abiVersion, totalSize, headerSize, slotCount, ...          │
├──────────────────────────────────────────────────────────────┤
│ Control Block (cache-line aligned, 64B each)                 │
│   readyIdx         (alignas 64)   atomic<uint32_t>           │
│   frameId          (alignas 64)   atomic<uint64_t>           │
│   slaveFrameId     (alignas 64)   atomic<uint64_t>           │
│   masterFrameId    (alignas 64)   atomic<uint64_t>           │
│   slotFrameIds[3]  (alignas 64)   atomic<uint64_t>[3]        │
│   slotSequences[3] (alignas 64)   atomic<uint64_t>[3]        │
├──────────────────────────────────────────────────────────────┤
│ Slot 0: SharedFrameData                                      │
│ Slot 1: SharedFrameData                                      │
│ Slot 2: SharedFrameData                                      │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 ABI Header

```cpp
struct SharedFrameAbiHeader {
    uint32_t abiVersion;     // = 5
    uint32_t totalSize;      // sizeof(SharedTripleBuffer)
    uint32_t headerSize;     // sizeof(SharedFrameAbiHeader)
    uint32_t capabilities;   // = 0 (保留)
    uint32_t slotCount;      // = 3
    uint32_t reserved;       // = 0
};
```

### 2.3 三缓冲读写协议

```
          Writer (Service)                        Reader (App)
          ═══════════════                         ═══════════

  1. slot = (readyIdx + 1) % 3             1. idx = readyIdx (acquire)
                                            2. fid = slotFrameIds[idx]
  2. slotSequences[slot] = odd (dirty)     3. seq1 = slotSequences[idx]
  3. write SharedFrameData into slot        4. copy SharedFrameData from slot
  4. slotFrameIds[slot] = frameId          5. seq2 = slotSequences[idx]
  5. slotSequences[slot] = even (clean)    6. fid2 = slotFrameIds[idx]
  6. readyIdx = slot
  7. frameId++                              7. if seq1 == seq2 (both even)
  8. SetEvent(frameReady)                      && fid == fid2
                                             → copy is valid, use it
                                              else
                                             → torn read, discard
```

**关键不变式**:
- `slotSequences` 奇数 = writer 正在写入 (reader 不可读)
- `slotSequences` 偶数 = slot 已完成写入 (reader 可读)
- `readyIdx` 总是指向最新完成的 slot
- reader 在拷贝前后各读一次 sequence; 两次均偶数且一致 → 无撕裂

### 2.4 控制块字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `readyIdx` | `atomic<uint32_t>` | 最新可读 slot 索引 (0/1/2) |
| `frameId` | `atomic<uint64_t>` | 单调递增帧计数器 (所有帧) |
| `slaveFrameId` | `atomic<uint64_t>` | Slave 帧计数器 |
| `masterFrameId` | `atomic<uint64_t>` | Master 帧计数器 |
| `slotFrameIds[3]` | `atomic<uint64_t>[3]` | 各 slot 对应的 frameId |
| `slotSequences[3]` | `atomic<uint64_t>[3]` | 各 slot 的 seqlock 序号 |

---

## 3. SharedFrameData — 单帧数据

### 3.1 运行时状态字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `workerState` | `int8_t` | 工作线程状态 |
| `streaming` | `bool` | 是否推送帧 |
| `lastFrameProcessUs` | `int64_t` | 上帧处理时间 (μs) |
| `avgFrameProcessUs` | `int64_t` | 平均帧处理时间 (μs) |
| `acquisitionFps` | `int32_t` | Master 采集 FPS (App 端计算) |
| `slaveAcquisitionFps` | `int32_t` | Slave 采集 FPS (App 端计算) |
| `vhfEnabled` | `bool` | VHF 设备启用 |
| `vhfDeviceOpen` | `bool` | VHF 设备已打开 |
| `vhfTranspose` | `bool` | VHF 坐标转置启用 |

### 3.2 触控数据

#### Heatmap

| 字段 | 类型 | 说明 |
|------|------|------|
| `heatmapMatrix[40][60]` | `int16_t[40][60]` | 热图矩阵 (rows × cols) |
| `rawDataLength` | `uint16_t` | 原始帧数据长度 |
| `rawData[Frame::kTotalFrameSize]` | `uint8_t[]` | 原始帧数据 |
| `timestamp` | `uint64_t` | 帧时间戳 |

#### 诊断区域

| 字段 | 类型 | 说明 |
|------|------|------|
| `touchZones[40][60]` | `uint8_t[40][60]` | 触控区域图 (仅 EGOTOUCH_DIAG) |
| `peakZones[40][60]` | `uint8_t[40][60]` | 峰值区域图 (仅 EGOTOUCH_DIAG) |

#### SharedPeak

```cpp
struct SharedPeak {
    int r, c;          // 行列坐标
    int16_t z;         // 信号值
    uint8_t id;        // 索引
};
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `peaks[30]` | `SharedPeak[30]` | 峰值点列表 |
| `peakCount` | `uint8_t` | 实际峰值点数 (≤ 30) |

#### SharedContact — 触控点

```cpp
struct SharedContact {
    int    id;             // 触点 ID
    float  x, y;           // 坐标 (mm)
    int    state;          // 状态 (0=up, 1=down, 2=move)
    int    area;           // 区域 (节点数)
    int    signalSum;      // 信号总量
    float  sizeMm;         // 物理尺寸 (mm)
    bool   isEdge;         // 边缘接触
    bool   isReported;     // 是否已上报
    int    prevIndex;      // 前一帧匹配索引
    int    debugFlags;     // 调试标记
    uint32_t lifeFlags;    // 生命周期标记
    uint32_t reportFlags;  // 上报标记
    int    reportEvent;    // 上报事件类型
};
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `contactCount` | `uint8_t` | 触控点数 (≤ 10) |
| `contacts[10]` | `SharedContact[10]` | 触控点列表 |
| `touchPackets[2]` | `SharedTouchPacket[2]` | HID 触控报文 (reportId + 32B payload) |

### 3.3 Stylus (笔) 数据

#### SharedStylusSolvePoint — 笔求解坐标

```cpp
struct SharedStylusSolvePoint {
    bool     valid;           // 数据有效标志
    float    x, y;            // 解算坐标 (mm)
    uint16_t reportX, reportY;// 上报坐标
    uint16_t pressure;        // 压力值 (处理后)
    uint16_t rawPressure;     // 原始压力
    uint16_t mappedPressure;  // 映射后压力
    uint16_t peakTx1, peakTx2;// TX1/TX2 峰值
    bool     tiltValid;       // 倾角有效
    int16_t  preTiltX, preTiltY;  // 前一帧倾角
    int16_t  tiltX, tiltY;        // 当前倾角
    float    tiltMagnitude;       // 倾角幅度
    float    tiltAzimuthDeg;      // 倾角方位角 (°)
    float    tx1X/Y, tx2X/Y;     // TX1/TX2 坐标
    float    confidence;          // 置信度
};
```

#### Stylus 状态字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `stylusPoint` | `SharedStylusSolvePoint` | 笔解算坐标 |
| `stylusPacket` | `SharedStylusPacket` | 笔 HID 报文 (reportId + 13B payload) |
| `stylusSlaveValid` | `bool` | Slave 数据有效 |
| `stylusChecksumOk` | `bool` | Checksum 通过 |
| `stylusSlaveOffset` | `uint8_t` | Slave 字偏移 |
| `stylusChecksum16` | `uint16_t` | CRC-16 校验值 |
| `stylusTx1Valid` | `bool` | TX1 块有效 |
| `stylusTx2Valid` | `bool` | TX2 块有效 |
| `stylusStatus` | `uint32_t` | 笔状态寄存器 |
| `stylusPressure` | `uint16_t` | 笔压力值 |
| `stylusBtRawPressure[4]` | `uint16_t[4]` | BT 原始压力 ×4 样本 |

#### ASA/HPP3 字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `stylusAsaMode` | `uint8_t` | ASA 模式 |
| `stylusDataType` | `uint8_t` | 数据类型 |
| `stylusProcessResult` | `uint8_t` | 处理结果码 (5=正常) |
| `stylusValidJudgment` | `bool` | 有效判断 |
| `stylusRecheckEnabled` | `bool` | 复核启用 |
| `stylusRecheckPassed` | `bool` | 复核通过 |
| `stylusRecheckOverlap` | `bool` | 复核重叠 |
| `stylusRecheckThreshold` | `uint16_t` | 复核阈值 |
| `stylusHpp3NoiseInvalid` | `bool` | HPP3 噪声无效 |
| `stylusHpp3NoiseDebounce` | `bool` | HPP3 噪声去抖 |
| `stylusHpp3Dim1/2Valid` | `bool` | HPP3 维度有效 |
| `stylusHpp3WarnX/Y` | `uint8_t` | HPP3 警告指示 |
| `stylusHpp3AvgX/Y` | `uint16_t` | HPP3 平均值 |
| `stylusHpp3Samples` | `uint8_t` | HPP3 样本数 |
| `stylusTouchNullLike` | `bool` | 触控空值检测 |
| `stylusTouchSuppressActive` | `bool` | 触控抑制激活 |
| `stylusTouchSuppressFrames` | `uint8_t` | 触控抑制帧数 |
| `stylusSignalX/Y` | `uint16_t` | 笔信号 X/Y |
| `stylusMaxRawPeak` | `uint16_t` | 原始最大峰值 |
| `stylusNoPressInk` | `bool` | AFT 无压墨水 |
| `stylusPipelineStage` | `uint8_t` | Pipeline 阶段 (0=ok, 1=slaveParse, 2=tx1, 3=peak, 4=coord, 5=noise) |

#### SharedStylusRawGrid — 笔原始网格

```cpp
struct SharedStylusRawGridBlock {
    uint16_t anchorRow, anchorCol;                    // 锚点坐标
    int16_t  grid[9][9];                              // 9×9 网格数据
    bool     valid;
};

struct SharedStylusRawGrid {
    SharedStylusRawGridBlock tx1;                     // TX1 9×9 网格
    SharedStylusRawGridBlock tx2;                     // TX2 9×9 网格
};
```

#### SharedStylusDiagnostics — 笔诊断 (100+ 字段)

完整列表参见目标 API 文档概述。关键字段组:

| 组 | 字段数 | 说明 |
|----|--------|------|
| 坐标诊断 | ~15 | anchorRow/Col, rawDim1/2, finalDim1/2, centerOff, pointX/Y |
| 速度/滤波 | ~5 | speedInstant, speedShortAvg, speedFullAvg, iirCoef |
| 倾角诊断 | ~10 | tiltDiffX/Y, tiltLenLimit, tiltRawDiff, preTilt, reportTilt |
| 压力诊断 | ~6 | peakSignal, rawPressure, mappedPressure, btRawPressure, preIirPressure |
| 边缘检测 | ~6 | dim1Edge, dim2Edge, edgeSignalTooLowLatched, fakePressureActive |
| 线性滤波 | ~10 | lfStateMachine, lfLineFitSlopeA/B, lfCos1000, lfStraightBufCount |
| 状态机 | ~8 | vhfPenState, linearFilterState, coorReviserActive, sigSuppressActive, penLifecycle |

#### Master/Slave Suffix — 硬件后缀视图

| 字段 | 类型 | 说明 |
|------|------|------|
| `masterSuffix` | `Frame::MasterSuffixView` | Master 硬件后缀 (结构化 POD 视图) |
| `slaveSuffix` | `Frame::SlaveSuffixView` | Slave 硬件后缀 |
| `masterSuffixValid` | `bool` | Master 后缀有效 |
| `slaveSuffixValid` | `bool` | Slave 后缀有效 |
| `masterWasRead` | `bool` | Master 帧被读取 |

---

## 4. ConfigDirtyFlag

### 4.1 内存布局

> Historical/legacy: `ConfigDirtyFlag` 描述保留用于解释旧 App 写 `config.ini` 后通知 Service reload 的兼容背景。当前 connected config 主路径是 Config v3 IPC (`GetConfigCatalogV3` / `GetConfigSnapshotV3` / `ApplyConfigPatchV3` / `PersistConfigV3`)，详见 [ipc_protocol.md](ipc_protocol.md) 与 [config_framework_api.md](config_framework_api.md)。

```
ConfigDirtyFlag (4 bytes)
┌──────────────────────────┐
│ atomic<uint32_t>         │
│ 0 = clean, 1 = dirty     │
└──────────────────────────┘
Shared Memory Name: Global\EGoTouchConfigDirty
```

### 4.2 协议

```
App (Writer):                      Service (Reader):
                                    每帧:
  legacy config.ini 更新完成后:       if CheckAndClear():
  m_flag->store(1, release)            ReloadConfig()
```

### 4.3 API

```cpp
class ConfigDirtyFlag {
public:
    bool Open();                          // 打开/创建共享内存
    void SetDirty();                      // App 写入后标记脏
    bool CheckAndClear();                 // Service 每帧检查并清除
    void Close();
    bool IsOpen() const;
};
```

> :warning: 该路径是 historical/legacy 说明；当前由 Config v3 IPC patch/persist 管理 connected 配置变更。

---

## 5. SharedFrameWriter / SharedFrameReader

### 5.1 Writer (Service 侧)

```cpp
class SharedFrameWriter {
public:
    bool Create(const wchar_t* name);     // Service 创建 Global\ 映射
    bool Open(const wchar_t* name);       // 或打开已有映射
    void Write(const SharedFrameData& frame); // 写入一帧 (三缓冲协议)
    void Close();
    bool IsOpen() const;
};
```

**Write() 行为**:
1. 选取下一个 slot = `(readyIdx + 1) % 3`
2. `slotSequences[slot] = ++seq` (使之为奇数, 标记 dirty)
3. `memcpy` 帧数据到 slot
4. `slotFrameIds[slot] = ++frameId`
5. `slotSequences[slot] = ++seq` (使之为偶数, 标记 clean)
6. `readyIdx = slot`
7. `SetEvent(frameEvent)`

### 5.2 Reader (App 侧)

```cpp
class SharedFrameReader {
public:
    bool Create(const wchar_t* name);
    bool Open(const wchar_t* name);       // App 打开已有映射
    bool Read(SharedFrameData& out);      // 安全读取 (seqlock 校验)
    bool Read(HeatmapFrame& out);         // 读取并转换为 HeatmapFrame
    uint64_t LastFrameId() const;
    uint64_t LastSlaveFrameId() const;
    uint64_t LastMasterFrameId() const;
    const SharedTripleBuffer* RawBuffer() const;
    const SharedFrameData* Raw() const;   // ⚠️ 不安全 (无 seqlock)
    HANDLE FrameReadyEvent() const;
    void Close();
    bool IsOpen() const;
};
```

**Read() 行为**:
1. 读取 `readyIdx` (acquire)
2. 记录 `slotFrameIds[readyIdx]`
3. 读取 `slotSequences[readyIdx]` → S1
4. `memcpy` 帧数据
5. 读取 `slotSequences[readyIdx]` → S2
6. 读取 `slotFrameIds[readyIdx]`
7. 若 S1 == S2 (偶数) 且 frameId 一致 → 数据有效
   否则 → 丢弃, 返回 false

**Raw() 行为** (:warning: 不安全):
- 直接返回 ready slot 指针, 无 seqlock 保护
- Writer 可能同时写入导致撕裂
- 仅用于高频轮询场景 (如 ImGui 渲染循环中采样)

---

## 6. 性能特征

| 指标 | 值 |
|------|-----|
| 单帧数据大小 (`sizeof(SharedFrameData)`) | ~80 KB |
| Triple-Buffer 总量 (`sizeof(SharedTripleBuffer)`) | ~256 KB |
| Writer 延迟 (`Write()`) | ~5 μs (memcpy 80KB + atomic writes) |
| Reader 延迟 (`Read()` seqlock 成功) | ~5 μs (memcpy 80KB + 4 atomic reads) |
| Reader 失败重试 | 等待下一次 `SetEvent(frameReady)` (~16ms at 60fps) |
| Cache-line 对齐 | 控制块每字段独立 cache line (64B) → 无 false sharing |

---

## 7. 兼容性

| 版本 | 变更 |
|------|------|
| ABI v1-v4 | 历史版本 (不再支持) |
| **ABI v5** | 当前版本: 增加 `masterSuffix`/`slaveSuffix`/`masterSuffixValid`/`slaveSuffixValid`、`SharedStylusDiagnostics` 扩展字段 (lfStateMachine 组、polySegment、btFreqShift 等)、Seqlock 替代 FrameId-only 协议 |

### 7.1 版本检查

```cpp
// Reader 打开共享内存后:
const auto* abi = &m_buf->abi;
if (abi->abiVersion != kSharedFrameAbiVersion) {
    LOG_ERROR("ABI mismatch: expected {}, got {}", kSharedFrameAbiVersion, abi->abiVersion);
    return false;
}
```

---

## 8. 生命周期

```
Service 启动:
  SharedFrameWriter::Create("Global\\EGoTouchSharedFrame")
  CreateEvent(frameReadyEvent)
  CreateEvent(logReadyEvent)
  CreateEvent(penStatusReadyEvent)

App 启动 → EnterDebugMode:
  SharedFrameReader::Open("Global\\EGoTouchSharedFrame")
  开始 WaitForSingleObject(frameReadyEvent) → Read()

App 退出:
  SharedFrameReader::Close()

Service 停止:
  SharedFrameWriter::Close()    (不删除共享内存 — 由 OS 在最后一个句柄关闭时回收)
```
