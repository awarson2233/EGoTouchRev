# hx83121a SPI 帧结构详细分析

> 来源：`himax_thp_drv.dll` 逆向，版本对应 port 8194

---

## 一、Master 帧结构（触摸通道，channel=0）

```
Offset    Size     说明
──────────────────────────────────────────────
+0        7 字节   协议头（固定 7 字节，具体格式见下文）
+7        4800 字节 电容原始数据（TX × RX × 2 byte）
+7+4800   256 字节  状态表（从 master_frame[DAT_180195310] 拷贝到 DAT_180195390）
```

> 4800 = TxNum × RxNum × 2，以 hx83121a 默认 40TX×60RX 为例：  
> 实际由全局 `frame_master_matrix_full_size` 决定（= 7 + TxNum×RxNum×2 + 256）

**注意**：协议头的 checksum 在 `+5` 处（`frame_master_matrix_full_size - 5 >> 1` 个 ushort 计算），全零帧 / checksum 失败时驱动会标记重试。

---

## 二、Master 状态表（256 字节 = 128 × u16）

**来源**：`thp_afe_get_frame` 在校验成功时执行：
```c
puVar17 = master_frame + DAT_180195310;  // master 帧内状态表起始偏移
memmove(DAT_180195390, puVar17, 256);    // 拷贝到全局状态块
```
被 `FUN_180010940` 按行打印，格式：`[HXTHP][HAL]STATUS [000]: v0 v1 ... v11`

**已确认字段（从 `process_stylus_status` 等逆向）：**

| 状态表偏移（字节） | DAT 地址 | 入口号 | 类型 | 含义 |
|---------|----------|--------|------|------|
| `+0x00` | `DAT_180195390` | 0 | u16 | **Firmware Retry Flag**：`1` = FW 请求主机重试本帧 |
| `+0x04` | `DAT_180195394` | 2 | u16 | **Freq Shift Done**：`!= 0` = 芯片已完成频率切换（配合 freqShiftStatus） |
| `+0x06` | `DAT_180195396` | 3 | u16 | **特殊诊断状态**：`0xBB` = 特殊调试/异常标记（触发上层 dirty flag） |
| `+0x0C` | `DAT_18019539c` | 6 | u16 | **Pending Freq Switch**：`!= 0` = 还有未完成的频率切换请求 |
| `+0x1C` | `DAT_1801953ac` | 14 | u16 | **PenF0 Noise Count**：频点0处的笔噪声计数（>5000触发频率切换） |
| `+0x20` | `DAT_1801953b0` | 16 | u16 | **PenF1 Noise Count**：频点1处的笔噪声计数（>5000触发频率切换） |


**`thp_afe_clear_status(param_1)` 命令含义：**

| param_1 | 动作 | 说明 |
|---------|------|------|
| `1` | 无效（提示用 `force_exit_idle`） | idle 状态应用 force_exit_idle |
| `4` | `freqShiftStatus = 0` | 清除"频率切换完成"标志 |
| `8` | calibration-done 清除 | 清除校准完成标志 |
| 其他 | `hx_send_command(cmd=0x06, val=param_1)` | 发 IPC 命令给芯片清除对应状态 |

---

## 三、7字节协议头（Master / Slave 帧共用格式）

```
Offset  Size  说明
+0      1     设备标识字节（SpiRead: 0xF3=master, 0xF5=slave；SpiWrite: 0xF2/0xF4）
+1      1     帧类型/命令字节（由 Hal_SpiBusRead 填入 cmd[0]）
+2      1     保留（0x00）
+3      2     帧序列号或长度（u16 LE）
+5      2     半帧 checksum（u16，对 master: frame[5..end]，对 slave: frame[5..end]）
```

> 校验方式：`thp_compute_checksum16(frame+5, (size-5)>>1, mode)`  
> - 结果为 0 且非零 → checksum 通过（通道正常）  
> - 全 0xFF → 无效帧（slave 9×9 block 中 `word[0]==0xFF && word[1]==0xFF` 也表示无笔）

---

## 四、Slave 帧结构（手写笔通道，channel=1）

```
Offset                  Size      说明
───────────────────────────────────────────────
+0                      7 字节    协议头（同上）
+matrix_header          18 字节   9×9 频点0 坐标块（center_x, center_y + 9×9 容值）
+DAT_18019530c          18 字节   9×9 频点1 坐标块（同格式）
```

`matrix_header` 和 `DAT_18019530c` 是全局变量，存储各 9×9 块在 slave 帧内的字节偏移。两个 9×9 块对应两个不同的 stylus 扫描频点（F0 和 F1）。


### 4.1 9×9 坐标块格式（`thp_unpack_9x9_block_to_matrix` 输入）

每个 9×9 块共 **20 字节**（= 2 + 9×2）：

```
Offset  Size  说明
+0      1     center_tx（TX 中心坐标，相对 TX 轴）
+1      1     center_rx（RX 中心坐标，相对 RX 轴）
+2      18    9×9 = 81 个 i16 值（以行主序排列，以 center 为中心的 9×9 子区域容值）
```

**全 `0xFF` 表示无效**：`word[0]==0xFF && word[1]==0xFF` → 无笔接触，此帧跳过解包。

**`thp_unpack_9x9_block_to_matrix` 工作原理：**
```
for row in 0..9:
  for col in 0..9:    // 其实每次写8个（见反编译，步长9个short）
    matrix_idx = (TX*RX) - ((center_tx-4 + row) * RX + (center_rx-2 + col))
    // 注意：矩阵写入是反向索引（从末尾向前），实现翻转
    output_matrix[matrix_idx] = block_data[row*9 + col]
```
输出到 `DAT_180171920`（F0矩阵）和 `DAT_180171920 + half_frame_matrix*2`（F1矩阵）。

### 4.2 噪声计数的来源

`g_PenF0NoiseCount`（`DAT_1801953ac`）和 `g_PenF1NoiseCount`（`DAT_1801953b0`）来自**master 状态表**（偏移 `+0x1C` 和 `+0x20`），由芯片固件在每帧填写，反映各频点的环境干扰强度。

---

## 五、帧数据流图

```
芯片 MISO
  │
  ├── SpiGetFrame(slave=1)
  │     SPI 帧 [0..6] = 协议头
  │     [matrix_header..]   9×9 F0 坐标块  ─→ thp_unpack → g_StylusMatrixF0Ptr
  │     [DAT_18019530c..]   9×9 F1 坐标块  ─→ thp_unpack → g_StylusMatrixF1Ptr
  │
  └── SpiGetFrame(master=0)
        SPI 帧 [0..6]         协议头
        [7 .. 7+4800-1]       电容原始矩阵 (TX×RX×2)
        [DAT_180195310..]     256字节状态表 ─→ memmove → DAT_180195390
                                                 ├── [+0]  retry_flag
                                                 ├── [+4]  freq_shift_done
                                                 ├── [+6]  diag_0xBB
                                                 ├── [+C]  pending_freq_switch
                                                 ├── [+1C] PenF0NoiseCount
                                                 └── [+20] PenF1NoiseCount
```


