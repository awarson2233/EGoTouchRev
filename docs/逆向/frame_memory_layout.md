# hx83121a 帧内存结构完整布局

> 来源：`himax_thp_drv.dll` Ghidra 逆向 + 代码交叉验证
>
> 最后更新：2026-04-10  
> 数据校准基线：当前代码实际使用情况 vs 逆向文档

---

## 一、总帧布局 (`back_data[0..5401]`, 共 5402 字节)

```
偏移          大小         区域名称                      简称
───────────────────────────────────────────────────────────────
[0..6]        7 B         Master 协议头                  MasterHeader
[7..4806]     4800 B      Master 电容矩阵 (40TX×60RX×2B) MasterMatrix
[4807..5062]  256 B       Master 状态表 (128×u16)        MasterSuffix
[5063..5069]  7 B         Slave 协议头                   SlaveHeader
[5070..5401]  332 B       Slave 笔数据 (166×u16)         SlaveSuffix
───────────────────────────────────────────────────────────────
总计          5402 B
```

---

## 二、Master/Slave 协议头 (7 字节, 两帧共用格式)

| 偏移 | 大小 | 类型 | 字段名 | 说明 |
|------|------|------|--------|------|
| +0 | 1 | u8 | deviceId | SpiRead: `0xF3`=master, `0xF5`=slave |
| +1 | 1 | u8 | cmdByte | 帧类型/命令字节 |
| +2 | 1 | u8 | reserved | 保留 (`0x00`) |
| +3 | 2 | u16 LE | seqOrLen | 帧序列号或长度 |
| +5 | 2 | u16 LE | checksum | 半帧校验 `thp_compute_checksum16(frame+5, (size-5)>>1)` |

> **校验方式**: 计算结果 = 0 且原始帧非全零 → 通过；全 `0xFF` → 无效帧

---

## 三、Master 电容矩阵 (`MasterMatrix`, 4800 字节)

**偏移**: `back_data[7..4806]`  
**大小**: 4800 字节 = 40 (TX) × 60 (RX) × 2 (int16_t LE)  
**排列**: 行主序, `matrix[tx][rx]`, 每个元素 `int16_t` little-endian  
**解析代码**: [MasterFrameParser.cpp:15-24](file:///d:/source/repos/EGoTouchRev-algo-A/EGoTouchService/Solvers/Preprocessing/MasterFrameParser.cpp#L15-L24)

```cpp
const uint8_t* raw_ptr = frame.rawData.data() + 7;
for (int i = 0; i < 2400; ++i) {
    uint16_t val = raw_ptr[i*2] | (raw_ptr[i*2+1] << 8);
    heat_ptr[i] = static_cast<int16_t>(val);
}
```

---

## 四、Master 状态表 (`MasterSuffix`, 256 字节 = 128 × u16)

**偏移**: `back_data[4807..5062]`  
**解析基础**: 所有字段均为 `uint16_t` little-endian, 按 word index 索引

### 4.1 已确认字段 (代码中实际被读取/使用)

| Word Index | 字节偏移 | 类型 | 字段名 | 消费者 | 状态 |
|:---:|:---:|:---:|--------|--------|:---:|
| 0 | +0x00 | u16 | `retryFlag` | 逆向文档 (FW 重试标记) | ✅ 已确认 |
| 2 | +0x04 | u16 | `freqShiftDone` | [HimaxAfe.cpp:231](file:///d:/source/repos/EGoTouchRev-algo-A/EGoTouchService/Device/himax/HimaxAfe.cpp#L231) | ✅ 实际使用 |
| 3 | +0x06 | u16 | `diagStatus` | 逆向文档 (`0xBB`=调试标记) | ✅ 已确认 |
| 6 | +0x0C | u16 | `pendingFreqSwitch` | 逆向文档 (未完成切换请求) | ✅ 已确认 |
| 8 | +0x10 | u16 | `tpFreq1` | [HimaxAfe.cpp:232](file:///d:/source/repos/EGoTouchRev-algo-A/EGoTouchService/Device/himax/HimaxAfe.cpp#L232) | ✅ 实际使用 |
| 9 | +0x12 | u16 | `tpFreq2` | [HimaxAfe.cpp:233](file:///d:/source/repos/EGoTouchRev-algo-A/EGoTouchService/Device/himax/HimaxAfe.cpp#L233) | ✅ 实际使用 |
| 14 | +0x1C | u16 | `penF0NoiseCount` | [HimaxAfe.cpp:229](file:///d:/source/repos/EGoTouchRev-algo-A/EGoTouchService/Device/himax/HimaxAfe.cpp#L229) | ✅ 实际使用 |
| 16 | +0x20 | u16 | `penF1NoiseCount` | [HimaxAfe.cpp:230](file:///d:/source/repos/EGoTouchRev-algo-A/EGoTouchService/Device/himax/HimaxAfe.cpp#L230) | ✅ 实际使用 |
| 54 | +0x6C | u16 | `touchX` | [HimaxChip.cpp:777](file:///d:/source/repos/EGoTouchRev-algo-A/EGoTouchService/Device/himax/HimaxChip.cpp#L777) | ✅ 实际使用 |
| 55 | +0x6E | u16 | `touchY` | [HimaxChip.cpp:778](file:///d:/source/repos/EGoTouchRev-algo-A/EGoTouchService/Device/himax/HimaxChip.cpp#L778) | ✅ 实际使用 |

### 4.2 文档中提到但代码中未使用的字段

> [!WARNING]
> 以下字段出现在 `stylus_system_reference.md` 或 `EngineTypes.h` 中，但在当前代码中**从未被赋值或读取**。可能是过时的逆向残留，需要你确认是否仍然有效。

| Word Index | 字节偏移 | 类型 | 字段名 | 来源 | 状态 |
|:---:|:---:|:---:|--------|--------|:---:|
| `base+8` | 动态 | u16 | `masterMetaTx1Freq` | stylus_system_reference.md (auto-detect base) | ⚠️ 从未赋值 |
| `base+9` | 动态 | u16 | `masterMetaTx2Freq` | stylus_system_reference.md | ⚠️ 从未赋值 |
| `base+10` | 动态 | u16 | `masterMetaPressure` | stylus_system_reference.md (标记"不可靠") | ⚠️ 从未赋值 |
| `base+12..13` | 动态 | u32 | `masterMetaButton` | stylus_system_reference.md | ⚠️ 从未赋值 |
| `base+14..15` | 动态 | u32 | `masterMetaStatus` | stylus_system_reference.md | ⚠️ 从未赋值 |
| 102 | +0xCC | u16 | `penSignalPressure` | stylus_system_reference.md | ⚠️ 代码中未引用 |
| 103 | +0xCE | u16 | `penSignalTilt` | stylus_system_reference.md | ⚠️ 代码中未引用 |

> **说明**: `masterMetaBaseWord` 及其相关的 `masterMeta*` 字段在 `EngineTypes.h` 的 `StylusFrameData` 中被声明(L123-129)，在 `SharedFrameBuffer.cpp` 中被拷贝到共享内存(L173-179)，在 `DiagnosticsWorkbench.cpp` 中被显示(L1315-1322)——但**在 Service Engine（StylusPipeline/StylusSolver）中从未被实际写入**。GUI 中显示的值始终为默认值 (Valid=N, Base=0xFF)。

### 4.3 Word Map 全貌

```
Word 位置            状态表字段
──────────────────────────────────────────────────────────
  0                  retryFlag (FW重试)               ✅
  1                  (未知)
  2                  freqShiftDone                    ✅
  3                  diagStatus (0xBB=调试)            ✅
  4                  (未知)
  5                  (未知)
  6                  pendingFreqSwitch                ✅
  7                  (未知)
  8                  tpFreq1 (当前频率1)              ✅
  9                  tpFreq2 (当前频率2)              ✅
  10..13             (未知)
  14                 penF0NoiseCount                  ✅
  15                 (未知)
  16                 penF1NoiseCount                  ✅
  17..53             (未知 — 74 words)
  54                 touchX (触摸X, 0xFF=无)           ✅
  55                 touchY (触摸Y, 0xFF=无)           ✅
  56..101            (未知 — 46 words)
  102                penSignalPressure (?)             ⚠️ 未使用
  103                penSignalTilt (?)                 ⚠️ 未使用
  104..127           (未知 — 24 words)
──────────────────────────────────────────────────────────
```

---

## 五、Slave 笔数据 (`SlaveSuffix`, 332 字节 = 166 × u16)

**偏移**: `back_data[5070..5401]` (= slave 帧起始 5063 + 7字节协议头)  
**解析代码**: [AsaTypes.h:71-99](file:///d:/source/repos/EGoTouchRev-algo-A/EGoTouchService/Solvers/StylusSolver/AsaTypes.h#L71-L99)

### 5.1 总体结构

| Word 范围 | 字节偏移 (相对 5070) | 大小 | 区域 |
|:---------:|:-------------------:|:----:|------|
| 0..82 | +0..+165 | 166 B | **TX1 Block** (频点0, 83 words) |
| 83..165 | +166..+331 | 166 B | **TX2 Block** (频点1, 83 words) |

### 5.2 TX Block 内部结构 (83 words, TX1 和 TX2 格式相同)

| Word 偏移 (块内) | 类型 | 字段名 | 说明 |
|:---:|:---:|--------|------|
| 0 | u16 | `anchorRow` | TX 中心坐标 (低字节有效), `0xFF`=无笔 |
| 1 | u16 | `anchorCol` | RX 中心坐标 (低字节有效), `0xFF`=无笔 |
| 2..82 | i16[81] | `grid[9][9]` | 9×9 电容值矩阵, 行主序 |

> **无效判定**: `word[0] & 0xFF == 0xFF && word[1] & 0xFF == 0xFF` → 无笔接触, 整块跳过

### 5.3 Word Map 全貌

```
Word 位置            TX1 Block
──────────────────────────────────────────────────
  0                  anchorRow (TX中心)              ✅
  1                  anchorCol (RX中心)              ✅
  2                  grid[0][0]                      ✅
  3                  grid[0][1]                      ✅
  ...                ...
  10                 grid[0][8]                      ✅
  11                 grid[1][0]                      ✅
  ...                ...
  82                 grid[8][8]                      ✅
──────────────────────────────────────────────────

Word 位置            TX2 Block
──────────────────────────────────────────────────
  83                 anchorRow (TX中心)              ✅
  84                 anchorCol (RX中心)              ✅
  85                 grid[0][0]                      ✅
  ...                ...
  165                grid[8][8]                      ✅
──────────────────────────────────────────────────
```

### 5.4 9×9 Grid 的物理含义

```
全传感器阵列 (40TX × 60RX):
┌──────────────────────────────────────────────┐
│                                              │
│             ┌─────────────┐                  │
│             │  9×9 窗口    │                  │
│             │             │  anchor = 窗口中心│
│             │   ★peak     │  (index=4,4)    │
│             │             │                  │
│             └─────────────┘                  │
│                                              │
└──────────────────────────────────────────────┘

全局坐标 = (anchor - 4) × 1024 + local_coord
         = (anchor - 4) × 0x400 + interp_result
```

---

## 六、Slave 协议头中的压力字段 (参考)

**偏移**: `back_data[5063..5069]` (7 字节 Slave 协议头)

| 偏移 (协议头内) | 类型 | 字段名 | 说明 | 状态 |
|:---:|:---:|--------|------|:---:|
| +3..+4 | u16 LE | `slaveSeq` | 帧序列号/长度 | ✅ 使用 |
| +5..+6 | u16 LE | `slaveChecksum` | 校验和 | ✅ 使用 |

> [!NOTE]
> `stylus_system_reference.md` 中提到的 `slaveHdrPressure` (Slave Header offset 4-5) 在当前代码中**未找到引用**。这可能是旧版逆向中的误解，实际该位置是帧序列号。

---

## 七、代码中已确认的过时/死亡字段

> [!CAUTION]
> 以下字段存在于代码中但**从未被实际赋值**，显示在 GUI 中始终为默认值。应在重构时清理。

### 7.1 `StylusFrameData` 中未使用字段 ([EngineTypes.h:104-222](file:///d:/source/repos/EGoTouchRev-algo-A/EGoTouchService/Solvers/EngineTypes.h#L104-L222))

| 字段名 | 类型 | 默认值 | 问题 |
|--------|------|--------|------|
| `masterMetaValid` | bool | `false` | Engine 中从未设为 `true` |
| `masterMetaBaseWord` | u8 | `0xFF` | Engine 中从未赋值 |
| `masterMetaTx1Freq` | u16 | `0` | Engine 中从未赋值 |
| `masterMetaTx2Freq` | u16 | `0` | Engine 中从未赋值 |
| `masterMetaPressure` | u16 | `0` | Engine 中从未赋值 |
| `masterMetaButton` | u32 | `0` | Engine 中从未赋值 |
| `masterMetaStatus` | u32 | `0` | Engine 中从未赋值 |
| `slaveWords[166]` | u16 array | `{}` | Engine 中从未填充 |
| `tx1Matrix[40][60]` | i16[][] | `{}` | Engine 中从未填充 (9×9 grid 直接在 AsaGridData 中处理) |
| `tx2Matrix[40][60]` | i16[][] | `{}` | 同上 |

> **结论**: `masterMeta*` 字段对应的是逆向文档中 "auto-detect base block" 的概念——从 master suffix 中动态定位一个 16-word stylus meta block。但该 auto-detect 逻辑**从未被实现**。`tx1/tx2Matrix` 和 `slaveWords` 同样是预留但未使用的字段，增加了 `sizeof(StylusFrameData)` 约 19.5KB 的无效内存。

### 7.2 `SharedFrameData` 中的对应过时字段

| 字段名 | 写入位置 | 问题 |
|--------|----------|------|
| `stylusMasterMetaValid` | [SharedFrameBuffer.cpp:173](file:///d:/source/repos/EGoTouchRev-algo-A/Common/source/SharedFrameBuffer.cpp#L173) | 永远为 `false` (因为来源字段从未被赋值) |
| `stylusMasterMetaBase` | [SharedFrameBuffer.cpp:174](file:///d:/source/repos/EGoTouchRev-algo-A/Common/source/SharedFrameBuffer.cpp#L174) | 永远为 `0xFF` |
| `stylusMasterMetaTx1/Tx2` | [SharedFrameBuffer.cpp:175-176](file:///d:/source/repos/EGoTouchRev-algo-A/Common/source/SharedFrameBuffer.cpp#L175-L176) | 永远为 `0` |
| `stylusMasterMetaPress` | [SharedFrameBuffer.cpp:177](file:///d:/source/repos/EGoTouchRev-algo-A/Common/source/SharedFrameBuffer.cpp#L177) | 永远为 `0` |
| `stylusMasterMetaBtn/Stat` | [SharedFrameBuffer.cpp:178-179](file:///d:/source/repos/EGoTouchRev-algo-A/Common/source/SharedFrameBuffer.cpp#L178-L179) | 永远为 `0` |

---

## 八、数据流图 (完整帧 → 各消费者)

```
芯片 MISO
  │
  ├── HimaxChip::GetFrame()
  │     back_data[0..5062]   ← m_master->GetFrame(5063B)
  │     back_data[5063..5401] ← m_slave->GetFrame(339B)
  │
  ├── [消费者 1: MasterFrameParser]
  │     读 back_data[7..4806]  → heatmapMatrix[40][60]
  │
  ├── [消费者 2: HimaxAfe::ProcessStylusStatus()]
  │     读 back_data[4807+0x04] → freqShiftDone
  │     读 back_data[4807+0x10] → tpFreq1
  │     读 back_data[4807+0x12] → tpFreq2
  │     读 back_data[4807+0x1C] → penF0NoiseCount
  │     读 back_data[4807+0x20] → penF1NoiseCount
  │
  ├── [消费者 3: HimaxChip::isFingerDetected()]
  │     读 back_data[4807+0x6C] → touchX
  │     读 back_data[4807+0x6E] → touchY
  │
  ├── [消费者 4: HimaxChip::isStylusDetected()]
  │     读 back_data[5070+0]    → slave word[0] (anchorRow)
  │     读 back_data[5070+2]    → slave word[1] (anchorCol)
  │
  ├── [消费者 5: StylusPipeline::Process()]
  │     读 back_data[5070..5401] → ExtractGridFromSlaveWords() → AsaGridData
  │
  ├── [消费者 6: SharedFrameWriter::Write()]
  │     memcpy back_data[4807..5062] → SharedMem.masterSuffix[256]
  │     memcpy back_data[5070..5401] → SharedMem.slaveSuffix[332]
  │
  └── [消费者 7: DiagnosticsWorkbench (via SharedMem)]
        读 SharedMem.masterSuffix → DrawMasterSuffixTable() (128 words hex dump)
        读 SharedMem.slaveSuffix  → DrawSlaveSuffixTable()  (166 words hex dump)
        读 SharedMem.slaveSuffix  → DrawSlaveHeatmap()      (9×9 heatmap × 2)
```

---

## 九、附录：偏移量速查表

| 常量名 | 值 | 计算方式 | 定义位置 |
|--------|-----|---------|---------|
| `kHeaderBytes` | 7 | — | 通用 |
| `kMatrixBytes` | 4800 | 40 × 60 × 2 | — |
| `kMasterSuffixBytes` | 256 | 128 × 2 | `SharedFrameBuffer.h:27` |
| `kSlaveSuffixBytes` | 332 | 166 × 2 | `SharedFrameBuffer.h:28` |
| `kMasterFrameSize` | 5063 | 7 + 4800 + 256 | — |
| `kSlaveFrameSize` | 339 | 7 + 332 | — |
| `kTotalFrameSize` | 5402 | 5063 + 339 | — |
| `kMatrixOffset` | 7 | `kHeaderBytes` | — |
| `kMasterSuffixOffset` | 4807 | 7 + 4800 | `HimaxAfe.cpp:228` |
| `kSlaveHeaderOffset` | 5063 | `kMasterFrameSize` | `HimaxChip.cpp:803` |
| `kSlaveSuffixOffset` | 5070 | 5063 + 7 | 多处硬编码 |
| `kBlockWords` | 83 | 2 + 81 | `AsaTypes.h:10` |
| `kGridDim` | 9 | — | `AsaTypes.h:11` |

---

> [!NOTE]
> 本文档反映截至 2026-04-10 的代码实际状态。
> 标记为 ⚠️ 的字段需要你确认：是需要实现 auto-detect parser 来填充，还是直接删除。
