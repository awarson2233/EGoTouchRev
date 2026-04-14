# gaokunhimaxcsot 的 ASA 参数对应

> [!NOTE]
> **参数源**: `TSAPrmt.dll` (Ghidra port `8194`)  
> **参数消费者**: `TSACore.dll` (Ghidra port `8193`)  
> **设备**: `GaokunHimaxCSOT`  
> **目标**: 把 `TSACore` 中访问的 ASA 参数偏移，对应回 `g_tsaPrmtFlashAsaGaokunHimaxCSOT`

---

## 1. 装载链确认

`TSACore.dll` 中的 `TSAPrmt_FlashPrmtLoadProject` @ `0x180fb759` 会构造四张参数表描述符，其中 ASA 表是：

```c
local_28 = &g_tsaPrmtFlashAsa;
local_20 = 0xe40;
```

然后通过回调把项目名传给 `TSAPrmt.dll`。

`TSAPrmt.dll` 中真正执行拷贝的是 `TSAFlashPrmt_LoadProject` @ `0x685817c9`：

```c
uVar1 = TSAFlashPrmt_GetProj(projectName);

memcpy_s(dest_flash,     size0, g_tsaPrmtFlashArray[uVar1],      0x1890);
memcpy_s(dest_cmf_pca,   size1, g_tsaPrmtFlashCmfPcaArray[uVar1],0x132c);
memcpy_s(dest_asa,       size2, g_tsaPrmtFlashAsaArray[uVar1],   0x0e40);
memcpy_s(dest_posture,   size3, g_tsaPrmtFlashPostureArray[uVar1],0x3eb20);
```

结论：

- `TSACore` 使用 `TSAPrmt_FlashPrmtLoadProject` 装载项目参数
- `TSAPrmt.dll` 内部真正把 `g_tsaPrmtFlashAsaArray[idx]` 的 `0xE40` 字节复制给 `TSACore`
- 对于你的设备，目标源表就是 `g_tsaPrmtFlashAsaGaokunHimaxCSOT`

---

## 2. 地址校正：哪一块才是真正的 ASA 参数表

这是这次最容易混淆的地方。

### 2.1 真实表体

Ghidra 中可见：

```text
6861b180  float *  g_tsaPrmtFlashAsaGaokunHimaxCSOT = 6861a340
```

也就是说：

- `0x6861b180` 是一个**指针变量**
- 里面存放的值是 `0x6861a340`
- 真正的 ASA 参数体从 `0x6861a340` 开始

### 2.2 表长

`TSAFlashPrmt_LoadProject` 对 ASA 表拷贝的大小固定是 `0xE40`。

因此真实表范围是：

```text
base = 0x6861a340
size = 0x0e40
end  = 0x6861b17f
```

也就是：

```text
0x6861a340 .. 0x6861b17f
```

### 2.3 为什么 `0x6861b180` 会出现

因为：

```text
0x6861a340 + 0x0e40 = 0x6861b180
```

所以 `0x6861b180` 恰好是这张表的**尾后地址**，同时该地址上放了一个指向表体的指针变量。

### 2.4 `0x6861b1c0` 不是 ASA 参数起点

对 `0x6861b1c0` 读取后，Ghidra 显示它已经是下一项数据：

```text
6861c4f0  pointer  g_tsaPrmtFlashCmfPcaGloriaSynaBOE_NOVATEK = 6861b1c0
```

因此：

- `0x6861b1c0` 不在 `g_tsaPrmtFlashAsaGaokunHimaxCSOT` 的 `0xE40` 范围内
- 它已经属于后续的别的参数块

结论：

- `0x6861b180`：是指针变量地址，不是 ASA 表体起点
- `0x6861a340`：才是 `g_tsaPrmtFlashAsaGaokunHimaxCSOT` 的真实表体起点
- `0x6861b1c0`：不是这张 ASA 表的参数起点

---

## 3. `ASA_LoadProjectPrmt` 与 `ASA_LoadStylusHWPrmt` 的对应

`TSACore.dll` 中：

### 3.1 `ASA_LoadProjectPrmt` @ `0x18084c75`

```c
g_asaPrmtFlash = param_2;
DAT_1820d610 = *(byte *)(param_2 + 0xa28);
DAT_1820d611 = *(byte *)(param_2 + 0xa29);
g_asaPrmt     = *(uint16_t *)(param_2 + 0xa68);
DAT_1820d602  = *(uint16_t *)(param_2 + 0xa6a);
ASA_LoadStylusHWPrmt(0);
```

所以这里的 `param_2` 就是整个 ASA 表基址，也就是：

```text
g_asaPrmtFlash = 0x6861a340
```

### 3.2 `ASA_LoadStylusHWPrmt` @ `0x18084abc`

```c
if (hwVersion == 0) hwVersion = 5;

g_asaPrmtStylus = g_asaPrmtFlash + unitIdx * 0x288 + 8;
DAT_1820d628 = *(uint16_t *)(g_asaPrmtFlash + unitIdx * 0x288 + 0xc);
DAT_1820d62a = *(uint16_t *)(g_asaPrmtFlash + unitIdx * 0x288 + 0xe);
memcpy(&DAT_1820d630,
       g_asaPrmtFlash + unitIdx * 0x288 + 0x10,
       0x238);
```

对 `0x6861a340` 开头读取后可知：

```text
0x6861a340: 01 00 00 00 00 00 00 00 05 00 00 00 FA 00 C8 00
```

可得：

- `numUnits = 1`
- 单元 0 从 `base + 0x08 = 0x6861a348` 开始
- 三个 HW version 槽位是 `[0x05, 0x00, 0x00]`
- 默认 `ASA_LoadStylusHWPrmt(0)` 会把 hwVersion 归一成 `5`
- 所以会选中 `unitIdx = 0`

因此：

```text
g_asaPrmtStylus = 0x6861a348
```

并且：

- `DAT_1820d628 = *(u16 *)(0x6861a34c) = 0x00FA = 250`
- `DAT_1820d62a = *(u16 *)(0x6861a34e) = 0x00C8 = 200`
- 被 memcpy 到 `DAT_1820d630` 的配置块源地址是：
  - `0x6861a350`
  - 长度 `0x238`

配置块起始字节：

```text
*(u8 *)0x6861a350 = 0x0E
```

---

## 4. 表头与全局区的参数对应

以下偏移均相对于：

```text
g_asaPrmtFlash = 0x6861a340
```

### 4.1 表头

| TSACore 访问 | 表偏移 | 实际地址 | 值 | 含义 |
|---|---:|---:|---:|---|
| `*g_asaPrmtFlash` | `+0x000` | `0x6861a340` | `1` | 参数单元数量 |
| `g_asaPrmtFlash[8]` | `+0x008` | `0x6861a348` | `5` | unit0 的 HW version 槽位 0 |
| `*(u16 *)(...+0x0c)` | `+0x00c` | `0x6861a34c` | `250` | `DAT_1820d628` |
| `*(u16 *)(...+0x0e)` | `+0x00e` | `0x6861a34e` | `200` | `DAT_1820d62a` |
| `memcpy src` | `+0x010` | `0x6861a350` | `0x0E` | 配置块起始字节 |

### 4.2 `ASA_LoadProjectPrmt` / HPP3 全局偏移

| TSACore 访问函数 | 表偏移 | 实际地址 | 值 | 说明 |
|---|---:|---:|---:|---|
| `ASA_LoadProjectPrmt` | `+0xa28` | `0x6861ad68` | `0x3C = 60` | `DAT_1820d610` / dim1Length |
| `ASA_LoadProjectPrmt` | `+0xa29` | `0x6861ad69` | `0x28 = 40` | `DAT_1820d611` / dim2Length |
| `GetTiltByCoorDif` 等 | `+0xa2a` | `0x6861ad6a` | `0x0A50 = 2640` | dim2Pitch |
| `GetTiltByCoorDif` 等 | `+0xa2c` | `0x6861ad6c` | `0x102C = 4140` | dim1Pitch |
| `GetPressInMapOrder` | `+0xa30` | `0x6861ad70` | `0x02` | 压力采样映射模式 = `incell` |
| `ASAStaticInit` | `+0xa40` | `0x6861ad80` | `0x00000000` | `featureDefault` |
| `ASAStaticInit` | `+0xa44` | `0x6861ad84` | `0x00000023` | `featureForce` |
| `NoPressInk Enter/Exit` | `+0xa50` | `0x6861ad90` | `0x00000001` | 无压出墨比较模式 |
| `HPP3_PostPressureProcess` | `+0xa74` | `0x6861adb4` | `0x0C` | fake pressure decrease 使能条件位，代码只检查 `!=0` |
| `HPP3_GetPressureMapping` | `+0xa80` | `0x6861adc0` | `0x00` | 默认 HW 的 pressure map index |
| `HPP3_GetPressureMapping` | `+0xa81` | `0x6861adc1` | `0x00` | HW v1/v2 的 pressure map index |
| `HPP3_CoordinateProcess` | `+0xa84` | `0x6861adc4` | `0x00` | signalRefreshMode |

### 4.3 `featureForce = 0x23` 的意义

`0x23 = 0b0010_0011`，默认开启的是：

- bit0 = `0x01`：`ASA_IsHpp3CoorReiviseFeatureEnabled`
- bit1 = `0x02`：`ASA_IsHpp3TouchEnableFeatureEnabled`
- bit5 = `0x20`：`ASA_IsHpp3LinearFilterFeatureEnabled`

默认**没有**开启的是：

- bit6 = `0x40`：`ASA_IsHpp3NoPressInkFeatureEnabled`
- bit7 = `0x80`：`ASA_IsHpp3NoPressTLearnedFeatureEnabled`

这只说明一件事：

- 对 `gaokunhimaxcsot` 而言，**静态表默认值**没有直接把 `NoPressInk` / `NoPressTLearned` 置位

### 4.4 运行时 feature 覆盖链

但 `g_FlagHpp3Feature` 并不是只在 `ASAStaticInit` 里写一次，后面还能被运行时函数覆盖。

#### `ASAStaticInit`

```c
g_FlagHpp3Feature =
    *(uint *)(g_asaPrmtFlash + 0xa40) |
    *(uint *)(g_asaPrmtFlash + 0xa44);
```

#### `AsaSetFeatures`

```c
g_FlagHpp3Feature = param_1 | *(uint *)(g_asaPrmtFlash + 0xa44);
```

也就是说，调用方完全可以把 `0x40 / 0x80` 从 `param_1` 里带进来。

#### 直接置位函数

```c
void ASA_EnableHpp3NoPressInkFeature(void)
{
    g_FlagHpp3Feature |= 0x40;
}

void ASA_EnableHpp3NoPressTLearnedFeature(void)
{
    g_FlagHpp3Feature |= 0x80;
}
```

因此：

- `+0xa44 = 0x23` 只能证明**静态默认**没有开 `NoPressInk`
- 不能证明运行时最终 `g_FlagHpp3Feature` 也没开
- 你的实际体验“它一定开启”与当前表值并不冲突

更准确的说法应该是：

- `gaokunhimaxcsot` 的 ASA 表 **完整定义了 NoPressInk 所需的参数**
- `NoPressInk` 是否实际生效，还要看运行时有没有把 `g_FlagHpp3Feature` 的 `0x40 / 0x80` 置上

---

## 5. `g_asaPrmtStylus` 的参数对应

以下偏移均相对于：

```text
g_asaPrmtStylus = 0x6861a348
```

为了方便核对，这里给出从 `0x6861a578` 开始读到的原始字节：

```text
0x6861a578:
02 00 DC 05 DC 05 B8 0B B8 0B 00 00 00 00 00 00
10 27 10 27 64 1E E8 03 10 27 1D 01 98 08 80 0C
04 00 28 00 5A 00 9B 00 C8 00 00 00 00 00 00 00
52 03 B6 03 E8 03 00 00 00 00 90 05 05 00 00 00
01 00 00 00 20 03 00 00 B0 04 00 00 DC 05 00 00
D0 07 00 00 00 00 00 00 04 00 00 00 C8 00 96 00
```

### 5.1 与落笔/抬笔直接相关的偏移

| TSACore 访问函数 | `g_asaPrmtStylus` 偏移 | 实际地址 | 值 | 说明 |
|---|---:|---:|---:|---|
| `HPP3_PostPressureProcess` | `+0x232` | `0x6861a57a` | `0x05DC = 1500` | 边缘低信号抑制进入阈值 |
| `HPP3_PostPressureProcess` | `+0x236` | `0x6861a57e` | `0x0BB8 = 3000` | 边缘低信号抑制退出阈值 |
| `GetNopressInkTholdFromLearnedTable` fallback | `+0x240` | `0x6861a588` | `0x2710 = 10000` | 无压出墨表为空时的默认阈值 |
| `UpdateNoPressInkThold` | `+0x244` | `0x6861a58c` | `0x64 = 100` | 无压出墨阈值比例 A |
| `UpdateNoPressInkThold` | `+0x245` | `0x6861a58d` | `0x1E = 30` | 无压出墨阈值比例 B |
| `GetNoPressInkTiltCompensation` | `+0x246` | `0x6861a58e` | `0x03E8 = 1000` | tilt compensation 基线 |
| `GetNoPressInkTiltCompensation` | `+0x248` | `0x6861a590` | `0x2710 = 10000` | tilt compensation 上限钳位 |
| `ClearTiltPrmt` | `+0x24a` | `0x6861a592` | `0x1D = 29` | tilt scale 默认值 |
| `IsTiltLearnedOK` | `+0x24b` | `0x6861a593` | `0x01` | 开启 learned tilt 严格检查 |
| `HPP3_SuppressBtPressBySignal` | `+0x24c` | `0x6861a594` | `0x0898 = 2200` | 蓝牙压力抑制进入阈值 |
| `HPP3_SuppressBtPressBySignal` | `+0x24e` | `0x6861a596` | `0x0C80 = 3200` | 蓝牙压力抑制退出阈值 |
| `HPP3_BtPenFreqShiftingDebounceTimeOut` | `+0x26e` | `0x6861a5b6` | `0x00` | freq shift 特殊 debounce flag |

### 5.2 同一区域里其他能确认的值

| 偏移 | 实际地址 | 值 | 说明 |
|---:|---:|---:|---|
| `+0x250` | `0x6861a598` | `0x04` | TX1/TX2 长度限制分段数 |
| `+0x252` | `0x6861a59a` | `40` | 分段阈值 0 |
| `+0x254` | `0x6861a59c` | `90` | 分段阈值 1 |
| `+0x256` | `0x6861a59e` | `155` | 分段阈值 2 |
| `+0x258` | `0x6861a5a0` | `200` | 分段阈值 3 |
| `+0x26a` | `0x6861a5b2` | `0x90 = 144` | 天线物理距离相关参数 |

---

## 6. 快速落笔/抬笔参数全集

把 `ASA_HPP3Process`、`NoPressInkProcess`、`HPP3_PostPressureProcess`、`EdgeCoorProcess`、`ReleaseASAReportInFreqShifting` 这几条链合起来看，这张 ASA 表已经覆盖了“快速落笔/抬笔”的全部直接参数入口。

### 6.1 Feature 与路径开关

- `+0xa40 / +0xa44`
  - 决定静态默认 feature
- `AsaSetFeatures / ASA_EnableHpp3NoPressInkFeature / ASA_EnableHpp3NoPressTLearnedFeature`
  - 决定运行时 feature 是否把 `0x40 / 0x80` 置上

### 6.2 真实压力进入路径

- `+0xa30`
  - `GetPressInMapOrder` 的采样次序
- `+0xa80 / +0xa81`
  - `HPP3_GetPressureMapping` 的曲线索引

### 6.3 无压出墨快速落笔路径

- `+0xa50`
  - `EnterToNoPressInk / ExitToNoPressInk` 的比较模式
- `g_asaPrmtStylus + 0x240`
  - 无压出墨表为空时的默认阈值
- `g_asaPrmtStylus + 0x244 / 0x245`
  - 无压出墨阈值比例
- `g_asaPrmtStylus + 0x246 / 0x248`
  - tilt compensation 的死区与上限
- `g_asaPrmtStylus + 0x24b`
  - learned tilt 严格检查开关

### 6.4 快速抬笔 / 抑制路径

- `g_asaPrmtStylus + 0x24c / 0x24e`
  - `HPP3_SuppressBtPressBySignal` 的进入/退出阈值
- `g_asaPrmtStylus + 0x232 / 0x236`
  - `HPP3_PostPressureProcess` 的边缘低信号抑制阈值
- `+0xa74`
  - fake pressure decrease 拖尾开关

### 6.5 边缘与几何约束

- `+0xa28 / +0xa29`
  - dim1 / dim2 长度
- `+0xa2a / +0xa2c`
  - dim2 / dim1 pitch

结论：

- 对快速落笔/抬笔来说，这张 `g_tsaPrmtFlashAsaGaokunHimaxCSOT` 表已经定义了**所有直接进入判定与抑制判定的参数**
- 不在表里的主要是运行时状态和历史值，而不是新的静态参数

---

## 7. 对你前面那条落笔/抬笔链的直接结论

把这些实值带回 `ASA_HPP3Process` 相关代码后，可以直接得到：

### 7.1 压力采样模式

`GetPressInMapOrder` 读取 `g_asaPrmtFlash + 0xa30`，本设备值是：

```text
+0xa30 = 0x02
```

结论：

- 这台设备走的是 `incell` 压力采样顺序表逻辑

### 7.2 压力曲线索引

`HPP3_GetPressureMapping` 读取：

- `+0xa80`
- `+0xa81`

本设备两者都是 `0`，所以：

- 默认 HW 和 HW v1/v2 都使用曲线索引 `0`

### 7.3 fake pressure decrease

`HPP3_PostPressureProcess` 检查 `g_asaPrmtFlash + 0xa74 != 0`。

本设备：

```text
+0xa74 = 0x0C
```

结论：

- fake pressure decrease 路径对 `gaokunhimaxcsot` 是开启的

### 7.4 蓝牙压力抑制阈值

`HPP3_SuppressBtPressBySignal` 使用：

- `g_asaPrmtStylus + 0x24c = 2200`
- `g_asaPrmtStylus + 0x24e = 3200`

结论：

- 信号低于 `2200` 时会触发蓝牙压力清零抑制
- 信号恢复到高于 `3200` 时才退出该抑制状态

### 7.5 边缘低信号强制抬笔阈值

`HPP3_PostPressureProcess` 使用：

- `g_asaPrmtStylus + 0x232 = 1500`
- `g_asaPrmtStylus + 0x236 = 3000`

结论：

- 边缘低信号抑制的进入/退出阈值分别是 `1500 / 3000`

### 7.6 NoPressInk 的静态默认值与运行时启用

由 `featureForce = 0x23` 可知，静态默认下：

- `bit6` (`0x40`) 没开
- `bit7` (`0x80`) 没开

结论：

- 本设备表里的默认值**不是直接开启** `NoPressInk` / `NoPressTLearned`
- 但运行时如果调用了 `AsaSetFeatures` 或 `ASA_EnableHpp3NoPressInkFeature` / `ASA_EnableHpp3NoPressTLearnedFeature`，这两个 feature 就会被打开
- 一旦被打开，本表中 `+0xa50`, `+0x240`, `+0x244`, `+0x245`, `+0x246`, `+0x248`, `+0x24b` 这些参数就会立刻进入 NoPressInk 路径

---

## 8. 最终定位结论

如果你后续要继续从 `TSACore` 代码里的偏移反查到 `gaokunhimaxcsot` 的参数值，可以直接使用下面这三组基址：

### 7.1 整个 ASA 表

```text
g_tsaPrmtFlashAsaGaokunHimaxCSOT (真实表体) = 0x6861a340
size = 0x0e40
range = 0x6861a340 .. 0x6861b17f
```

### 7.2 指针变量

```text
g_tsaPrmtFlashAsaGaokunHimaxCSOT (指针变量地址) = 0x6861b180
*(uint64_t *)0x6861b180 = 0x6861a340
```

### 7.3 选中的 stylus unit

```text
g_asaPrmtStylus = 0x6861a348
config memcpy 源 = 0x6861a350
```

所以你后面看到：

- `g_asaPrmtFlash + X`
- `g_asaPrmtStylus + Y`

都可以分别替换成：

- `0x6861a340 + X`
- `0x6861a348 + Y`

去直接读 `gaokunhimaxcsot` 这张表里的实际设备参数。
