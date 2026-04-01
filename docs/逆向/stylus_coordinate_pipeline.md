# 手写笔坐标解算管线文档

> 最后更新: 2026-03-28

## 1. 总体架构

```mermaid
flowchart TD
    A["原始帧数据\n~5402 bytes"] --> B["Slave Frame\n339 bytes"]
    A --> C["Master Frame\n5063 bytes\n(触控数据, 不使用)"]

    B --> D["ParseSlaveWords\n校验 + 解析 166 words"]
    B --> E["ExtractSlaveHeader\npress / btn / status"]

    D --> F["ExtractGridFromSlaveWords\nTX1: anchor + 9×9 grid\nTX2: anchor + 9×9 grid"]

    F --> G["FindPeak\nflood-fill 局部最大值"]
    G --> H["ProjectTo1D\n行列求和投影"]
    H --> I["CoordinateSolver\n三角插值 (TSACore算法)"]
    I --> J["Anchor 偏移校正\nfullCoord = anchor×1024\n+ gridLocal − 4×1024"]
    J --> K["BuildStylusPacket\n映射到 HID [0, 16000]"]
    E --> K

    style A fill:#334155,stroke:#94a3b8,color:#f1f5f9
    style B fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
    style D fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
    style E fill:#3b1f5e,stroke:#a78bfa,color:#ede9fe
    style F fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
    style G fill:#14532d,stroke:#4ade80,color:#dcfce7
    style H fill:#14532d,stroke:#4ade80,color:#dcfce7
    style I fill:#14532d,stroke:#4ade80,color:#dcfce7
    style J fill:#7c2d12,stroke:#fb923c,color:#ffedd5
    style K fill:#7c2d12,stroke:#fb923c,color:#ffedd5
    style C fill:#1c1917,stroke:#57534e,color:#a8a29e
```

## 2. 数据帧结构

### 2.1 完整帧布局

| Offset | Size | Description |
|--------|------|-------------|
| `0x0000` | 5063 | Master Frame (触控 mutual capacitance, 本管线不直接使用) |
| `0x13C7` | 339 | Slave Frame (手写笔核心数据) |

### 2.2 Slave Frame 结构

```mermaid
block-beta
    columns 5

    block:header:2
        columns 1
        h["Header (7 bytes)"]
        s0["[0..1] status (u16 LE)"]
        s1["[2..3] freq (u16 LE)"]
        s2["[4..5] pressure (u16 LE)"]
        s3["[6] button (u8)"]
    end

    block:payload:3
        columns 1
        p["Payload (332 bytes = 166 words)"]

        block:tx1:1
            columns 1
            t1["TX1 Block — 83 words"]
            a1["word 0: anchorRow"]
            a2["word 1: anchorCol"]
            g1["word 2..82: 9×9 grid (81 values)"]
        end

        block:tx2:1
            columns 1
            t2["TX2 Block — 83 words"]
            a3["word 83: anchorRow"]
            a4["word 84: anchorCol"]
            g2["word 85..165: 9×9 grid (81 values)"]
        end
    end

    style header fill:#3b1f5e,stroke:#a78bfa,color:#ede9fe
    style payload fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
    style tx1 fill:#14532d,stroke:#4ade80,color:#dcfce7
    style tx2 fill:#14532d,stroke:#4ade80,color:#dcfce7
```

> [!IMPORTANT]
> **Anchor 含义**: `anchorRow` / `anchorCol` 代表 9×9 grid 的 **中心位置** (grid index 4) 在完整传感器阵列上的坐标。这是固件在扫描时确定的笔的大致位置。

### 2.3 TX Block 内 Grid 排列

9×9 grid 按**行优先**存储在 word[2..82] 中:

```
grid[r][c] = (int16_t) words[2 + r × 9 + c]

     c=0  c=1  c=2  ...  c=8
r=0 [ w2   w3   w4  ...  w10 ]
r=1 [ w11  w12  w13 ...  w19 ]
 ⋮
r=8 [ w74  w75  w76 ...  w82 ]
```

## 3. 坐标解算流程详解

### 3.1 Step 1 — Slave Words 解析

> 📄 [StylusPipeline.cpp](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/StylusPipeline.cpp#L28-L52) :: `ParseSlaveWords`

- 跳过 slave header 的 7 字节
- 读取 166 个 little-endian uint16 words
- (可选) 校验和验证: `sum(all_words) & 0xFFFF == 0`

### 3.2 Step 2 — Grid 提取

> 📄 [AsaTypes.h](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/AsaTypes.h#L71-L99) :: `ExtractGridFromSlaveWords`

```cpp
TX1.anchorRow = words[0];
TX1.anchorCol = words[1];
TX1.grid[r][c] = (int16_t) words[2 + r*9 + c];
TX1.valid = (anchorRow != 0x00FF) || (anchorCol != 0x00FF);
```

### 3.3 Step 3 — Peak 检测

> 📄 [GridPeakDetector.cpp](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/GridPeakDetector.cpp#L76-L103) :: `FindPeak`

```mermaid
flowchart LR
    A["遍历 9×9\n每个 cell"] --> B{"IsPeak?\n4-邻域局部最大值\n且 > noiseThreshold"}
    B -- Yes --> C["FloodFill\n计算连通区域"]
    B -- No --> A
    C --> D{"connectedPixels\n< maxConnected?"}
    D -- Yes --> E["计算 3×3\n邻域和"]
    D -- No --> A
    E --> F{"邻域和 >\nbest?"}
    F -- Yes --> G["更新 bestPeak"]
    F -- No --> A

    style B fill:#14532d,stroke:#4ade80,color:#dcfce7
    style D fill:#7c2d12,stroke:#fb923c,color:#ffedd5
    style G fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
```

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `noiseThreshold` | 50 | 低于此值的 cell 视为噪声 |
| `maxConnected` | 20 | 连通区域超过此值视为噪声块 |

### 3.4 Step 4 — 1D 投影

> 📄 [GridPeakDetector.cpp](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/GridPeakDetector.cpp#L106-L139) :: `ProjectTo1D`

以 peak 为中心，取 ±`projRadius` (默认 2) 行/列做求和投影:

```
dim1[c] = Σ grid[rMin..rMax][c]   (c = 0..8)  → 列方向信号
dim2[r] = Σ grid[r][cMin..cMax]   (r = 0..8)  → 行方向信号

peakIdxDim1 = argmax(dim1)
peakIdxDim2 = argmax(dim2)
```

### 3.5 Step 5 — 三角插值 (核心算法)

> 📄 [CoordinateSolver.cpp](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/CoordinateSolver.cpp) :: 逆向自 TSACore.dll `TriangleAlgUsing3Piont`

```mermaid
flowchart TD
    A["1D 投影 signal\n+ peakIdx"] --> B{"peakIdx == 0?"}
    B -- "左边缘" --> C["TriangleAlgEdge\n用虚拟邻居补偿"]
    B -- No --> D{"peakIdx == 8?"}
    D -- "右边缘" --> E["TriangleAlgEdge\nresult = 9×1024 − edge"]
    D -- "中间" --> F["TriangleAlgUsing3Point\nleft, peak, right"]

    F --> G["比较 left vs right"]
    G -- "right < left\n(偏左)" --> H["minVal = min(right, peak−1)\nratio = −(left−min)×1024 / (peak−min) / 2"]
    G -- "right ≥ left\n(偏右)" --> I["minVal = min(left, peak−1)\nratio = +(right−min)×1024 / (peak−min) / 2"]
    H --> J["result = ratio + 0x200\n(+512 半格偏移)"]
    I --> J

    J --> K["gridLocal = peakIdx × 1024 + result"]

    C --> K
    E --> K

    style F fill:#14532d,stroke:#4ade80,color:#dcfce7
    style J fill:#7c2d12,stroke:#fb923c,color:#ffedd5
    style K fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
```

> [!NOTE]
> `0x200` (512) 偏移将结果定位在 cell 中心。当三个信号值相等时，`ratio = 0`，结果为 `peakIdx × 1024 + 512` — 即 cell 正中央。

### 3.6 Step 6 — Anchor 偏移 (关键步骤)

> 📄 [StylusPipeline.cpp](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/StylusPipeline.cpp#L247-L264) :: 坐标重建

```mermaid
flowchart LR
    subgraph "9×9 Grid 局部坐标"
        GL["gridLocalX ≈ 4×1024 + 512\n= 4608 (peak在中心时)"]
    end

    subgraph "Anchor 信息"
        AN["anchorRow = 笔在传感器上的\n大致行号 (grid 中心)"]
    end

    GL --> CALC
    AN --> CALC

    CALC["fullSensorX =\nanchor × 1024\n+ gridLocal\n− 4 × 1024"] --> RESULT["全传感器坐标"]

    style GL fill:#14532d,stroke:#4ade80,color:#dcfce7
    style AN fill:#3b1f5e,stroke:#a78bfa,color:#ede9fe
    style CALC fill:#7c2d12,stroke:#fb923c,color:#ffedd5
    style RESULT fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
```

**公式**:
```
fullSensorX = (anchorRow − gridCenter) × kCoorUnit + gridLocalX
fullSensorY = (anchorCol − gridCenter) × kCoorUnit + gridLocalY

其中 gridCenter = 4, kCoorUnit = 1024
```

> [!WARNING]
> 如果不减去 `gridCenter × kCoorUnit`，坐标会固定偏移 ~4096，因为 anchor 指的是 grid **中心** 而非左上角。

### 3.7 Step 7 — HID 报告映射

> 📄 [StylusPipeline.cpp](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/StylusPipeline.cpp#L572-L608) :: `BuildStylusPacket`

```
reportX = (1.0 − fullSensorX / (sensorDimX × 1024)) × 16000
reportY = fullSensorY / (sensorDimY × 1024) × 16000

输出范围: [0, 16000] → Windows HID 映射到屏幕像素 (2560×1600)
```

X 轴反转 (`1.0 − ...`) 是横屏方向约定。

## 4. 原厂 vs 我们的实现

```mermaid
flowchart TD
    subgraph "原厂 ThpService + TSACore"
        direction TB
        O1["Himax 固件输出\nslave frame\n9×9 grid + anchor"] --> O2["ThpService\n(Himax驱动)"]
        O2 --> O3["创建 fullDim × fullDim\n零矩阵 (如 37×23)"]
        O3 --> O4["将 9×9 贴到\nanchor 位置"]
        O4 --> O5["TSACore 收到完整矩阵\n长度=37 的 1D 投影\nPeak 在全局位置"]
        O5 --> O6["坐标直接正确\n无需 anchor 偏移"]
    end

    subgraph "EGoTouchService (我们)"
        direction TB
        E1["Himax 固件输出\nslave frame\n9×9 grid + anchor"] --> E2["直接在 9×9 上\nPeak detection\n1D 投影 (长度=9)"]
        E2 --> E3["三角插值\ngridLocal 坐标\n(范围 0~9216)"]
        E3 --> E4["加 anchor 偏移\n减 gridCenter\n→ 全传感器坐标"]
        E4 --> E5["映射到 HID"]
    end

    style O1 fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
    style O4 fill:#7c2d12,stroke:#fb923c,color:#ffedd5
    style O6 fill:#14532d,stroke:#4ade80,color:#dcfce7
    style E1 fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
    style E4 fill:#7c2d12,stroke:#fb923c,color:#ffedd5
    style E5 fill:#14532d,stroke:#4ade80,color:#dcfce7
```

> [!TIP]
> 两种方案**数学等价**: 原厂在完整矩阵中做投影时，只有 anchor 附近的 9×9 区域有非零值，peak 总是在 `[anchor, anchor+8]` 范围内。我们直接用 anchor 偏移得到相同结果。

## 5. 关键参数表

| 参数 | 值 | 说明 |
|------|------|------|
| `kGridDim` | 9 | 网格维度 |
| `kCoorUnit` | 1024 (`0x400`) | 每传感器间距的子单位数 |
| `kGridCenter` | 4 | 网格中心索引 (`kGridDim / 2`) |
| `sensorDimX` | **待确定** (默认 37) | 完整传感器行数 |
| `sensorDimY` | **待确定** (默认 23) | 完整传感器列数 |
| `noiseThreshold` | 50 | Peak 检测信号门限 |
| `projRadius` | 2 | 1D 投影的行/列范围 |
| HID Report Max | 16000 | HID 逻辑坐标最大值 |
| 屏幕分辨率 | 2560 × 1600 | Gaokun CSOT 面板 |

### 确定 sensorDimX/Y

将笔移到屏幕四角，观察日志 `Anchor` 行的值:

```
sensorDimX = max(anchorRow) + kGridDim   (最大 anchorRow + 9)
sensorDimY = max(anchorCol) + kGridDim   (最大 anchorCol + 9)
```

## 6. Pressure / Button 处理

### 6.1 数据来源

从 **slave frame header** (前 7 字节) 提取:

| Offset | Size | Field |
|--------|------|-------|
| 0-1 | uint16 LE | status |
| 2-3 | uint16 LE | frequency |
| 4-5 | uint16 LE | pressure `[0, 0x0FFF]` |
| 6 | uint8 | button (bit 0 = pressed) |

> [!NOTE]
> 此布局基于 Himax HPP3 协议假设，需通过日志中 `SlaveHdr` 的实际字节验证。

### 6.2 Pressure 处理链

```mermaid
flowchart LR
    A["raw pressure\n(uint16)"] --> B["分段多项式\nmapping"]
    B --> C["增益缩放\n×gainPercent/100"]
    C --> D["IIR 低通\n滤波"]
    D --> E["尾部衰减\n(释放时)"]
    E --> F["输出\n[0, 4095]"]

    style A fill:#3b1f5e,stroke:#a78bfa,color:#ede9fe
    style B fill:#14532d,stroke:#4ade80,color:#dcfce7
    style D fill:#1e3a5f,stroke:#60a5fa,color:#e0f2fe
    style F fill:#7c2d12,stroke:#fb923c,color:#ffedd5
```

## 7. 文件索引

| 文件 | 职责 |
|------|------|
| [AsaTypes.h](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/AsaTypes.h) | 常量、数据结构、Grid 提取 |
| [GridPeakDetector.cpp](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/GridPeakDetector.cpp) | Peak 检测 + 1D 投影 |
| [CoordinateSolver.cpp](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/CoordinateSolver.cpp) | 三角插值坐标解算 |
| [CoorPostProcessor.cpp](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/CoorPostProcessor.cpp) | 坐标后处理 (IIR, jitter) |
| [StylusPipeline.cpp](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/StylusPipeline.cpp) | 主管线、帧解析、anchor偏移、HID |
| [StylusPipeline.h](file:///d:/source/repos/EGoTouchRev-rebuild-自建算法/EGoTouchService/Engine/StylusSolver/StylusPipeline.h) | 所有可配置参数定义 |

## 8. 已知问题

- [ ] `sensorDimX` / `sensorDimY` 待通过实际 anchor 范围确定
- [ ] Slave header 字节布局需日志验证 (pressure/button 偏移)
- [ ] Tilt 输出暂时禁用，需正确 TX2 参数
- [ ] `CoorMultiOrderFitCompensate` (多项式校正) 未实现
- [ ] `SensorPitchSizeMap` (非线性间距映射) 未实现，当前为线性近似
- [ ] Edge compensation 参数使用默认值，应从 TSAPrmt 提取
