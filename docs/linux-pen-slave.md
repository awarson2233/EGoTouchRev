# Linux 侧参考实现：HX83121A 级联从片主动笔（MateBook E Go / gaokun3）

> 平台：Ubuntu 26.04 / 内核 7.2.9 / aarch64（SC8280XP）
> 作者：mashen11 — 同一台机器（gaokun3 / GK-W7X，面板 CSOT `ppc357db1-4`）
> 内核侧代码**不在本仓**（GPL-2.0），只外链：`mashen11/MateBookEgo-linux-kernel`
> `patches/common/linux-gaokun/0083…0110`、`scripts/stylus/`

---

## 0 为什么值得单独写一篇

本项目的笔集成在 Windows 侧暂时断开（README 原话：*Active Stylus / BT-MCU
integration is temporarily disconnected in this branch*）。Linux 侧我们从零走出了
**另一条路**：不碰主片，只读级联从片（`spi20`）的 raw 帧，用户态合成 uinput 笔。

两条路用的是**同一对 IC、同一套寄存器约定**（`raw_out_sel` / `rawdata_addr`），
所以下面这些数字可以直接交叉验证贵项目的抓包结论；反过来，贵仓
`docs/penservice_events.md` 与 `ipc_interface_protocol.md` 里的 MCU 报文，
也是我们电量/侧键通道的依据。

一句话结论：**笔在 Linux 上能完整跑通（书写/压感/倾角/悬停/长按右键/顶栏电量/
出入坞/单击/拖选）**，且在 GNOME 50 上也修完了。下面是可用事实，不是计划。

---

## 1 硬件拓扑

| 角色 | 控制器 | 设备节点 | 驱动 |
|---|---|---|---|
| 主片（手指 + TCON） | `spi6` = `9c0000.geniqup/spi@998000` | `spi1.0`（Ubuntu 实测） | `himax_hx83121a_spi` |
| **从片（笔 AFE）** | **`spi20` = `8c0000.geniqup/spi@890000`** | `spi0.0`（Ubuntu 实测） | `himax_hx83121a_pen` |

- 从片中断：**GPIO 39**，`IRQ_TYPE_LEVEL_LOW`；SPI `mode 3`、12 MHz、CS0。
- 从片引脚：`gpio87–90`（`qup20`）。**注意 `gpio92` 属于同一个功能组，但在本机是后摄稳压器
  的使能脚**，只能复用这四个。
- ★ **从片没有自己的复位线**（DTS 原注：*It has no reset line of its own: it is reset
  together with the master*）。这一条是后面 §3.4 那个 bug 的根。
- ★ **编号坑**：`890000` 地址低于 `998000`，谁先 `okay` 谁先注册拿 `spi0`
  ⇒ **永远动态发现**（`ls -d /sys/bus/spi/devices/*/algo`），绝不硬编码总线号。

ACPI 对应：主片 `HIMX...`，从片 **`THPB` / `HIMX0002`（`\_SB.SP21`）**。

---

## 2 协议：从片相对主片只有两处不同

| 项 | 主片 | 从片 |
|---|---|---|
| 读操作码 | `0xF3` | **`0xF5`** |
| 写操作码 | `0xF2` | **`0xF4`** |
| 数据源切换 | —— | **必须写 `raw_out_sel = 0xF6`**（`0x100072ec`）|
| 原始数据地址 | —— | **必须写 `rawdata_addr = 0x5AA5`**（`0x10000000`）|

> 这两个写就是贵仓 `EGoTouchRev` 里
> `pfw_op.addr_raw_out_sel` 与 `himax_switch_data_type(Slave, EGO_RAWDATA)` 的等价物。
> **不写这两个寄存器，从片会稳定回全零帧** —— 这是我们踩了两天的坑（当时误判成
> "从片不存在"，实际是少了这两笔）。

### 帧格式（336 B）

```
offset 0..1   5a a5            帧头（不对 = 撕裂/空读，丢弃）
offset 4..    166 × u16 LE
  words 0..82     TX1 块：word0=row, word1=col, word2..82 = 81 点网格
  words 83..      TX2 块：同上（83=row, 84=col, 85..=81 点网格）
```

- 出帧率：笔在范围内 ~**240 Hz**，无笔 ~130 Hz（仍在刷）。
- `ICID`（`0x900000d0`）读回 **`0x83121a00`**，与主片一致。
- ★ **读会消费帧**：读太快会拿到全零或撕裂缓冲（无 `5a a5`），要按中断节奏读。

### TX1 锚点有效性（关键）

```c
valid1 = !((row & 0xff) == 0xff && (col & 0xff) == 0xff);
```

`00ff/00ff` = 这一帧没有有效锚点。**位置与悬停取自 TX1**（与上游说法一致）。
这个判据我们用它做了两件事：一是坐标解算，二是**判定从片是否真的在工作**
（见 §3.4）。

---

## 3 五条已定案结论

### 3.1 主片网格上做不出笔 —— 别再试

我们曾在主片 raw 网格上做过完整尝试（候选门/空间门/`nbr8`/latch/四阶段差分，
共 16 个内核补丁）。终局：**被动电容耦合分不开"极近悬停"与"轻触"**。

| | 真接触 | 极近悬停 |
|---|---|---|
| 网格峰值 `best` | 400–580 | **140–360** |
| `nbr8` | 720–1280 | 65–150 |

两段**重叠**，任何单一阈值都必然错杀一类。25 s 纯悬停能刷出 580 帧 / 293 个幽灵点。
⇒ 这 16 条补丁已全部 `.patch.disabled` 退役，只在文档里留作反面教材。

### 3.2 从片才是正路

从片给的是**笔自己的 AFE 读数**，信噪比比主片网格高一个数量级。上面那张表在
从片上变成：真接触 peak 1500–5000、悬停 300–700，**可分**。

### 3.3 倾斜轴向与上游代码相反

5 段标定（分离度 4×~460×，无歧义）证明：**左右倾只改列轴、前后倾只改行轴**。

```c
tilt_x = out_c;   // 上游写的是 out_r
tilt_y = out_r;   // 上游写的是 out_c
```

照抄上游会把两轴写反。符号照 libinput `doc/tablet-support.dox`：两轴都 +1。
（顺带一提：**上游的坐标框和我们不同，"照上游"的代码要当嫌疑犯审**。）

### 3.4 ★ 核心发现：从片的 init 必须晚于主片的 FW reload

**这是最值得贵项目参考的一条**，因为它同样是"主片 + 级联从片"的时序问题，
与语言/平台无关。

**现象**：开机后笔能用，但**悬停光标满屏乱跳、点击不灵敏**；重载一次驱动即恢复。

**根因**：从片 probe（~3.5 s）**早于**主片的 pin reset 与 FW reload（~4.2 s 才完成）。
在那之前写 `raw_out_sel = 0xF6`，级联的 TX1 锚点通路还没就绪：

```
开机后实测（同一 boot 内 A/B）
  重载前：tx1_valid 5543/19444 = 12~28%，锚点列在 2→52 之间乱跳，坐标满屏飞
  重载后：tx1_valid 13687/21197 = 64.6%，锚点列收敛到 12~25，光标平滑
```

看门狗随后重写同一个寄存器**救不回来** —— 问题不在寄存器**值**，在写入**时机**；
只有"整模块重载"（等主片稳定后从头来一次）才恢复。

**修法**（`0110`）：从片 probe 里，`spi_setup()` 之后、首次 `pen_hw_init()` 之前等待。

```c
static int master_settle_ms = 2500;   /* 0 = 关闭，便于 A/B */

/* 实测冷启动主片最晚 4.34 s 完成 FW reload，2.5 s 留约 1.6 s 余量 */
if (master_settle_ms > 0)
        msleep(master_settle_ms);
```

**验证**（热重启，同一台机器）：

```
19:31:21  himax-spi spi1.0: FW reload done          ← 主片先完成
19:31:23  himax-hx83121a-pen spi0.0: slave init 0   ← 从片晚 2 s（等待生效）
（开机那条 "watchdog: no frame for 1 s, re-init #1" 彻底消失，wd_reinits 0）
悬停帧间位移 p50 2 px / p90 6~21 px，压力 p50 2015~2695
```

> 对 Windows 侧的启示：如果贵项目的笔路径也在主片 reset/FW 加载期间初始化从片，
> 同样的 12%~28% 锚点有效率问题会以"笔抖/丢点"的形式出现，且重跑服务能好 —— 
> 可以作为一个排查方向。

### 3.5 压力/倾角**不走**从片，走 BT-MCU

从片只有坐标场。压力与倾角来自键盘套 MCU（USB `12d1:10b8`）：

- **接口 0**（HID）：ReportID **`0x55`**，压感/倾斜
- **接口 1**（Vendor）：电量 `0x08` / 充电 `0x09` / 连接 `0x71`（沿用贵仓
  `penservice_events.md` 的编号，我们实测一致）

★ 一个必须知道的坑：**MCU 在系统 suspend/resume 后不再转发压感**，只有 USB 端口复位能恢复
（`usbcore.quirks=12d1:10b8:b` 可在启动参数里兜住）。

---

## 4 用户态：为什么必须自己合成一支笔

从片只给原始帧，Linux 桌面不认识它。我们按下面的顺序踩完了全部坑，最后是
`gk3pend`（Python，uinput 合成器）：

| 步骤 | 结论 |
|---|---|
| uinput 声明为 tablet | libinput 认成 `tablet` ✅ |
| 但报 `BUS_VIRTUAL` | **libwacom 的 `DeviceMatch` 只认 `bus:vid:pid`** ⇒ 匹配不上自定义 `.tablet` ⇒ GNOME 建不出平板工具 ⇒ `Gtk.GestureStylus.get_axis()` 全读不到 |
| 改报 `BUS_USB 12d1:10b8` | 压感/倾角终于可读 ✅（这是**必须**的一步）|
| 但 Mutter 会把轴转 90° | ⇒ 必须 `--frame portrait-swap` 预转置，否则笔上划光标右移 |
| `libwacom ≥2.14` | `DeviceMatch` 语法从 `usb:12d1:10b8` 改成 **`usb|12d1|10b8`**（老的会被判 `invalid .tablet`）|
| GNOME 50 / Mutter 18 | ★ **tablet 的 tip 不再转成 pointer click**（实测一次书写：tablet Motion 1436 / 核心指针 Motion 2）⇒ GTK 应用"画笔能画、点按钮没反应"。解法：合成一个 cursor 镜像设备，在 `BTN_TOUCH` 跳变处配发 **`BTN_LEFT`** |

后两条是 26.04 迁移时才暴露的，写在这里是因为**任何把 tablet 事件桥到桌面的实现都会撞上**。

---

## 5 与 Windows 侧的对照

| 维度 | EGoTouchRev（Windows） | 本实现（Linux） |
|---|---|---|
| 手指 | 主片 raw + TSACore 复刻 | 内核 `himax_hx83121a_spi`（上游驱动 + 49 条质量补丁）|
| 笔坐标 | 主片 HID / MCU | **级联从片 `spi20` 原始帧** |
| 笔压力 | BT-MCU（本项目当前断开）| BT-MCU `12d1:10b8` if0 / `0x55` |
| 输出 | Windows HID 注入 | uinput stylus + cursor 镜像 |
| 电量/出入坞 | `THP_Service` | 内核 `power_supply("m-pencil")` + udev + 顶栏扩展 |

两条路可以互证的点：`raw_out_sel=0xF6`、`rawdata_addr=0x5AA5`、`ICID=0x83121a00`、
MCU 事件号 `0x08/0x09/0x71`、压感 ReportID `0x55`。

---

## 6 复现（Linux，同机型）

```bash
# 1) 内核：本仓不提供，见 mashen11/MateBookEgo-linux-kernel
#    patches/common/linux-gaokun/0083  从片笔驱动
#                            0084  启用 spi20 + pen@0
#                            0085  power_supply "m-pencil"（仅入坞时创建）
#                            0086  raw_out_sel 运行时可调
#                            0110  等主片 FW reload 再 init（§3.4）
# 2) 用户态（本目录 Tools/LinuxPenSlave）
sudo bash install-gaokun3-fixes.sh --apply && sudo reboot
```

`Tools/LinuxPenSlave/` 里是两个纯 Python 文件（无第三方依赖），可直接读：

- `gk3pend.py` —— 从 `/dev/gk3_pen` 读帧 + 从 MCU hidraw 读压力 → 合成 uinput 笔
- `gk3pen_solver.py` —— 帧 → 坐标/倾斜的解算与标定

它们不参与 CMake 构建（`Tools/` 各自独立），只作参考实现；许可按本仓 MIT。

---

## 7 仍未解决（如实列出）

1. **悬停幽灵**：从片在"极近悬停"仍偶发上报（peak 300–700 区间的边界效应）。
   彻底解需要主动笔 HPP，Linux 侧没有。
2. **倾斜幅度未校准**：物理 45° 报 ~69°（沿用上游常数）。
3. **手掌误触**：uinput 笔没有 `ID_PATH`，libinput 的 touch arbitration 用不上。
4. **帧率退化**：头 2–3 s 干净 165 Hz，之后降到 ~37 Hz（疑似帧泵停住而非降频）。
5. **点击精度**：RMS 0.81 mm / 最大 1.22 mm，与用户瞄准噪声（0.30 mm）同量级 ⇒ 判定不需要补偿。

---

## 8 参考

- 内核补丁与全部实测记录：`github.com/mashen11/MateBookEgo-linux-kernel`
  （`docs/stylus/`，80+ 篇，含逐次 A/B 与失败路径）
- 26.04 迁移全记录：该仓 `docs/stylus/MIGRATION-26.04-2026-10-21.md`
- 本项目 MCU 协议：`docs/penservice_events.md`、`docs/ipc_interface_protocol.md`
