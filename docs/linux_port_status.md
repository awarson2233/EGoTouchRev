# Linux 移植进展（gaokun3 / Huawei MateBook E Go）

> 截至 2026-10-25。机器：sc8280xp，内核 7.1.0-rc3-gaokun3+（linux-gaokun 的 recommended 补丁集）。
> 本文记录 Linux 适配做到了哪一步、在实机上量出来的事实、走过的弯路，以及还没解决的问题。

## 1 目标与架构

保持 Windows 的分层，只把最底下那层换成 Linux 的：

| | Windows | Linux |
|---|---|---|
| 内核驱动 | SpbThpTool.sys（华为，IOCTL） | `Kernel/himax-thp`（misc 字符设备 `/dev/himax_thp`） |
| 芯片协议、Init | HimaxChip（用户态） | 同一份代码，经 `Port::ITouchBus` |
| 手指解算 | TsaCore | 同一份 |
| 输出 | VHF | uhid → hid-multitouch（未做） |

笔先不做（Slave 已接通，但只读到无笔帧）。

## 2 已完成

### 2.1 用户态

- **Port 层** `EGoTouchService/Port/`：`TouchBus.h`（`ITouchBus`、`TouchChannel`、`CreateTouchBus`）、`Time.h`（`SleepMs`）；
  `win32/TouchBusWin32.cpp`（原 HalDevice 原样搬入，**未在 Windows 上编译验证**）、`linux/TouchBusLinux.cpp`。
- **HimaxCore** 静态库：`Device/himax/Himax*.cpp` 改为经 Port 访问总线，Linux 上可编译。
  `Chip::SetSlaveEnabled(false)` 可只用 Master。
- CMake：非 WIN32 只构建 Common / Port / HimaxCore / TsaCore / 工具；Windows 专属目标仍只在 WIN32 下构建。
- `TsaCore/src/App/Property.cpp`：越界 memset 改为逐字段清零（编译过，**未用录制数据回放验证逐位一致**）。
- **`Tools/HimaxThpProbe`**：跑 `Chip::Init` 后连续取帧、按 Chip 的规则校验两路、打印 heatmap 峰值。
  选项：`--with-slave`、`--raw`（两路最小寄存器读写对照）、`--poll`（绕开驱动读帧，直接轮询事件栈并打印寄存器）、
  `--dump FILE`（配合 `analyze_dump.py` 离线分析）。

### 2.2 内核

- **`himax-thp.ko`**：spi_driver，匹配 `himax,hx83121a-ts`（Master）和 `egotouch,hx83121a-thp-slave`（Slave）。
  每一路各有自己的 spi 设备、中断、INT 脚；Slave 作为第二个 spi 设备挂到 Master 实例上，用户态仍只看到一个节点。
  复位脚、面板联动（drm_panel_follower）只在 Master 上。ABI 见 `himax_thp.h`。
- **`thp-slave-dt.ko`**：运行时 DT overlay（`gaokun3-thp-slave.dtso`），insmod 时启用 spi20 和其下的 Slave，
  rmmod 时撤销，不改启动用的 dtb。
- **`thp-bind.sh on|off|show`**：在原驱动 himax-spi 与 himax-thp 之间切换 spi0.0，并加载/撤销 overlay。
  测完用 `off` 恢复系统触摸。
- 构建：`make -C Kernel/himax-thp KDIR=~/repos/kernel/linux-7.1-rc3 KBUILD_MODPOST_WARN=1`
  （源码树只做过 modules_prepare，没有 Module.symvers；整编内核会过热关机，见 §6）。

## 3 硬件事实（实机测得）

### 3.1 两颗芯片的接法

主线 DT 只描述了 Master。Slave 的信息来自本机 ACPI DSDT（用一个只读模块从 EFI systab 给出的 RSDP 把表复制出来，
iasl 反编译）：

| | ACPI 设备 | 总线 | INT | 复位 |
|---|---|---|---|---|
| Master | `\_SB.THPA`（HIMX0001） | SPI7 = DT spi6 / `spi0.0`（0x998000） | gpio175，Edge ActiveLow | gpio99 |
| Slave | `\_SB.THPB`（HIMX0002） | SP21 = DT **spi20**（0x890000，qup20：gpio87–90） | **gpio39**，Edge ActiveLow | 无（共用） |

- 两者都是 12 MHz、ACPI 里写的 CPOL 0 / CPHA 0（mode 0）。实测 mode 3（linux-gaokun 的设置）和 mode 0 结果一样。
- THPA 还有第三个中断 gpio8（Edge ActiveBoth，ExclusiveAndWake）；Linux 上它被 DSI 用作 `mdp_vsync`。SpbThpTool 把它接到
  一个只完成 `WAIT_INT` 的 ISR，Windows 用户态的面板电源线程（PanelPowerWatcher）挂的就是它；120 Hz 出帧期间它不翻转，和出帧无关。
- **此前在 spi0.0 上用 F4/F5 读 Slave 全是 0，原因是 Slave 根本不在那条总线上。** 接上 spi20 后 Slave 应答正常
  （IC ID `0x83121A00`，寄存器读写、`RAW_OUT_SEL=0xF6` 都能写进去）。
- Slave 的功能码仍是 F4/F5：Windows 用户态就是这么发给 SPBTESTTOOL_SLAVE 的（windbg trace 里有 `f4 00 ec 72 00 10 f6 …`）。

### 3.2 帧格式

- Master 5063 字节、Slave 339 字节（SpbThpTool 里也是硬编码的 `0x13C7` / `0x153`）。读帧命令 `[F3/F5, 0x30, 0x00]`。
- 布局：3 字节总线头 + 4 字节数据头（`5A A5 xx xx`）+ 数据。Chip 的校验从第 5 字节起按 u16 求和，低 16 位为 0。
- Slave 无笔时的帧是 `5A A5` 加全 0（Chip 的 AllZero → `MarkNoStylus`），属于正常帧。

### 3.3 INT 与出帧（已解决：gpio174 主机接口选择脚）

**根因：** 芯片在 FW reload（gpio99 复位后自动触发）时采样 TLMM **gpio174**：低 = SPI 模式（THP 原始数据），
高 = I2C-HID 模式（UEFI 用的，即 DSDT 里被 `_STA` 关掉的 `HIMX1234`）。UEFI 把它留在高电平（本机开机后是
`out high 16mA pull up`），主线 DT 和 linux-gaokun 的 DT 都没配它，芯片就锁在 I2C-HID 模式：SPI 寄存器读写、
FW reload 都正常，但两颗芯片都不按帧拉 INT，Slave 根本不扫描（Init 后只吐一帧）。下表与下面几条都是这个状态下测的。
线索来自 [vahiru/gaokun-android](https://github.com/vahiru/gaokun-android) 的 `patches/0002-…-drive-ts-mode-gpio174-low`
和 [aoripus/easy-for-gaokun](https://github.com/aoripus/easy-for-gaokun) 的 `patches/touch-spi-mode`，两边独立写下的症状和这里一致。

**拉低 gpio174 再复位之后**（开机默认环境、非 DSC、geni FIFO 模式、驱动全默认）：两根 INT 都按帧来，
`HimaxThpProbe --with-slave` **120.0 fps**，1200 帧里 Master 全部有效、Slave 1198–1199 帧有效（只有开头 1–2 帧坏），
和 Windows 一致。只有原驱动 himax-spi 时，无触摸也是 INT175 每秒 120 次。

现在 himax-thp 在 Master probe 时占住 gpio174 输出低（`ts_mode_gpio`，见 §5），解绑后 TLMM 保持低电平。
单独调试原驱动时可用 `Kernel/himax-thp/ts-mode.py spi` 设好再重绑 himax-spi。长期应在 DT 的 `ts0_default` 里把它配成
`output-low`（上面两个仓库的补丁就是这么做的）。

以下是 I2C-HID 模式下（修复前）的记录：


| 场景 | INT175（Master） | INT39（Slave） | 事件栈 |
|---|---|---|---|
| 复位中 / 复位后未初始化 | 高 | — | — |
| 原驱动 himax-spi（只写 Master `RAW_OUT_SEL=0xF6`） | 无触摸时高；触摸时一直低，电平重触发约 190 次/秒 | — | 正常 |
| 我们的 Init，只初始化 Master | **一直低** | — | 约一半帧撕裂/全零 |
| 我们的 Init，Master + Slave | 只在开头低一次，之后一直高 | **从不变低** | 两路都在出帧 |

- 在 Master + Slave 模式下，用 GPIO 字符设备以约 50 万次/秒采样两根 INT，3 秒内跳变数为 0；Master 改成下降沿触发
  （TLMM 边沿锁存）也只收到 1 次。所以不是短脉冲被漏掉，而是芯片确实不再拉 INT。
- 已排除：读帧长度（`read_extra` 扫 0–2048 字节无影响）、SPI 模式（0/3）、Slave 状态残留（开机第一次就带 Slave
  初始化，结果一样）、显示时序（开机加 `panel_himax_hx83121a.enable_dsc=1` 跑 DSC 120Hz，结果一样）。
- **Windows 上 INT 是按帧来的**（用户确认：`INT re-opened` 只在 IDLE 时出现）。原因见本节开头：Windows 的驱动栈在 SPI 模式下工作。
- 同样在 I2C-HID 模式下还排除过：片选建立时间（`cs_setup_us=0`）、geni GSI 模式（补丁 0025 + `qcom,force-gsi-mode`）、面板 `E2`（Windows 120 Hz 下也是 `E2 00`）。gaokun-android 的 Slave 驱动在本机这个状态下也是 0 次中断。

### 3.4 事件栈的读法（I2C-HID 模式下测的，SPI 模式下按 INT 读不再撕裂）

- **事件栈的任何一次读都会把这一帧取走**：只读 7 字节帧头之后再从头读整帧，读到的是 0。
- 读得太早：读到的是 0；读到一半芯片才开始吐帧：撕裂（`5A A5` 出现在帧中间的随机位置），这一帧也就丢了。
- 读的时机对时可以连续拿到每一帧（时间戳每帧 +1），同步时两帧间隔约 9.6 ms（约 104 Hz）。看起来芯片是上一帧被
  取走之后才开始准备下一帧。
- 没备好时的整帧读（3.4 ms）占着总线，同步一旦断掉会连续 ~90 ms 全读到 0，像是在干扰芯片出帧。
- AHB 读 rawdata 缓冲区首字 `0x10000000`：帧备好时是 `5A A5 …`，但**帧被取走后不会清掉**，后两个字节也不是每帧都变，
  不能当"新帧"标志。

## 4 SpbThpTool.sys 反汇编要点

安装包 `Gaokun_THP_Software_1.0.1.39.exe` → `SPIMCUDriver.exe` 里带 `SpbThpTool.sys` 和 **PDB**（有全部函数名）。
提取件与反汇编放在 `~/repos/kernel/ref/spbthptool/`（`spb.asm`、`spb-syms.txt`）。

- `OnInterruptIsrForThp`（INT 边沿）：`FrameBlockFlag`（`SET_BLOCK` 写它，`D0Entry` 里置 1）非 0 且没有读帧在进行时，
  直接发 `[F3/F5, 0x30, 0]` 读整帧。中断从 `PrepareHardware` 起一直连着，所以 Init 期间的边沿也会读帧；
  Windows 上实测在 Init 前 `SET_BLOCK 0`、让 Init 期间不读帧，照样 120 fps（见 [windows-spi-channel-analysis.md](/docs/artifacts/windows-spi-channel-analysis.md)）。
- 读完成回调：把帧拷进新缓冲、入队（最多 2 帧）、触发事件。**不写任何寄存器，不检查 INT 电平。**
- `GET_FRAME`（0x4001C28）：队列里有帧就取；没有就等事件（超时由 SET_TIMEOUT 设）。
- **`INT_OPEN`（0x4001C00）会立刻读一帧**，路径与 ISR 相同。Chip 遇到某一路超时会重开它，靠的就是这一次读。
  → Linux 驱动的 `INT_ENABLE(1)` 已照此实现（打开时立刻读一帧）。补上后 Master 帧 100% 有效（但仍靠超时重开驱动，约 2 fps）。
  `INT_CLOSE`（0x4001C04）什么也不做，直接成功返回；要让驱动不读帧只能 `SET_BLOCK 0`。
- `FullDuplexInterval`：标准 2 段 SPB 全双工（写 + 读，`DelayInUs=0`），等价于 Linux 的一次全双工传输。
- 同一安装包里的 `HxDisableTPINT_reg.x64.exe` 只是把两颗芯片都切进 safe mode，与 INT 机制无关。

## 5 当前驱动状态

驱动默认在各路的帧中断里读帧（`poll_us=0`）。模块参数：

| 参数 | 默认 | 说明 |
|---|---|---|
| `ts_mode_gpio` | 174 | Master probe 时占住这根 TLMM 脚并输出低（SPI 模式），解绑时释放；-1 = 不管（DT 已配好时） |
| `poll_us` | 0 | 0 = 用帧中断；非 0 = 取帧线程按这个周期（µs）轮询，留作对照，绑定时生效 |
| `poll_peek` / `poll_after_us` | 2 / 0 | 只在轮询时有意义：就绪判断方式、读到一帧后至少等多久 |
| `slave_irq_trigger` / `master_irq_trigger` | 2 / 0 | 申请中断时覆盖 DT 的触发类型：Slave 下降沿，Master 按 DT 的低电平；两者都实测可用 |
| `slave_spi_mode` / `master_spi_mode` | 3 / 3 | SPI 模式（0 也可以） |
| `cs_setup_us` | 300 | 每次传输前的片选建立等待（绑定时生效）。geni 片选由硬件控制，这段等待落在片选拉低之前 |
| `read_extra`、`debug_irq` | 0 / N | 实验与调试 |

修复 gpio174 之前用轮询凑出来的最好成绩是约 47 fps（`poll_peek=0 poll_after_us=5500`），现在中断模式下是 120 fps。

## 6 未解决的问题与下一步

1. ~~把 gpio174 写进 DT~~：已完成（2026-10-25），见 §7。驱动的 `ts_mode_gpio` 仍默认 174，给没改 DT 的机器兜底。
2. 熄屏亮屏、面板复位之后的行为：复位会触发 FW reload，只要 gpio174 还是低就没问题，但要实测。
3. Slave 的帧校验：SPI 模式下 Slave 帧已经通过校验（每轮开头 1–2 帧坏，原因待查）。
4. 之后的工作：uhid 输出（UhidReporter / IInputSink）、egotouchd 守护进程、其余 Port（面板电源、输入、笔 USB、系统状态）、
   打包（DKMS / systemd / udev）、TsaRawReplay 逐位验证（需要 Windows 录制的 .dvrbin）、Windows CI 验证 Port 改动。

## 7 环境注意事项

- **整编内核会过热关机**（CPU 热区只有 110 °C 的 critical trip，没有被动降频；EC 温度不进 Linux 热管理），已发生两次。
  只编树外模块。`~/repos/kernel/guarded-build.sh` 是带温控暂停的编译脚本，备用。
- **实际启动用的 ESP 是 sda1（CPRA_A64FRE），不是挂在 `/boot/efi` 的 sda3**；两边 `loader/entries` 各有一份。
  为 DSC 测试加的 `…-gaokun3+-dsc.conf` 在两边都有（oneshot 已用掉，默认项未改），不需要可删除。
- 内核参数里有 `efi=noruntime`，但 `bootctl set-oneshot` 仍能写 EFI 变量。
- 用 GNOME 的 PowerSaveMode 关屏再开屏会触发 DPU `ctl reset failure / vblank timeout`，之后芯片 Init 在 FW reload 超时——
  **不要用关屏来复位 TDDI**。
- `/tmp`（含会话 scratchpad）是 tmpfs，重启即清空；需要保留的提取件放在 `~/repos/kernel/ref/`。
- **默认启动项用的是改过的 dtb** `sc8280xp-huawei-gaokun3-tsmode.dtb`（两个 ESP 上都有）：只在启动用的原 dtb 的
  `ts0_default` 里加了 `mode-n-pins`（gpio174 `output-low`），反编译对比只多这一个节点；dts 在 `~/repos/kernel/dt-ts-mode/`。
  实测开机后 gpio174 为低、归 spi0.0，只有 himax-spi 时 INT175 就是 120 次/秒。原 dtb 留着；`…-dsc.conf` 仍指向原 dtb。
  **kernel-install / 升级内核会重写启动项和 dtb**，届时要重新加上这个节点（或把它补进内核树的 dts）。
- `sudo dmesg` 里 overlay 的 `memory leak will occur if overlay removed` 警告是内核对 overlay 撤销的固定提示，可忽略。
