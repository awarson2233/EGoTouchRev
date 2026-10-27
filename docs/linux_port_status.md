# Linux 移植进展（gaokun3 / Huawei MateBook E Go）

> 截至 2026-10-26。机器：sc8280xp，内核 7.2.12-gaokun3+（此前在 7.1.0-rc3-gaokun3+ 上开发）。
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
  构建用 preset（Ninja + clang，按宿主系统只显示对应的 preset）：
  `cmake --preset linux-Release && cmake --build --preset linux-Release`（产物在 `build/linux-Release`，Debug 用 `linux-Debug`）。
  内核模块：`cmake --build --preset linux-kmod`（即 `make -C Kernel/himax-thp`，见 §2.2）。输出目录约定：用户态 `build/<preset>/`，
  内核模块 `build/kernel/<内核版本>/`，install 前缀 `out/install/<preset>/`；源码目录不留编译产物。
- `TsaCore/src/App/Property.cpp`：越界 memset 改为逐字段清零（编译过，**未用录制数据回放验证逐位一致**）。
- **`Tools/HimaxThpProbe`**：跑 `Chip::Init` 后连续取帧、按 Chip 的规则校验两路、打印 heatmap 峰值。
  选项：`--with-slave`、`--raw`（两路最小寄存器读写对照）、`--poll`（绕开驱动读帧，直接轮询事件栈并打印寄存器；
  只在 Windows 上可用，Linux 驱动一路只给一个文件绑定）、`--dump FILE`（配合 `analyze_dump.py` 离线分析）。
- **触摸输出** `Port/TouchSink.h`（`ITouchSink`、`CreateTouchSink`）、`linux/TouchSinkLinux.cpp`：uhid 虚拟 HID 触摸屏，
  报告沿用 VHF 的 32 字节触摸报告（`VhfTouchPacket::Build` 直接复用），描述符手写，hid-multitouch 接管成
  `EGoTouch Touchscreen`。hid-multitouch 按默认类处理，不会自动抬起没出现的触点，所以 sink 记下按着的 ID，
  `ReleaseAll` 逐个发抬起。Windows 仍走 VhfReporter，没有经这个接口。
- **`egotouchd`**（`EGoTouchService/linux/egotouchd.cpp`，只在非 WIN32 下构建）：取帧 → StylusPipeline → TSACore →
  AFE 闭环 → uhid，与 DeviceRuntime 的流式一帧相同。建链简化为：总线就绪才 Init，失败退避重试；SOS 或总线掉线时
  抬起所有触点、`OnAfeStopped`，回到等总线。选项 `--transpose`、`--master-only`。笔还没有上报。
  实测：120 fps，手指方向正确（GNOME 旋转后自动映射），按下 / 抬起成对，IC 进 IDLE 后稳定。
- **安装包**（自带 himax-thp 与 touch-spi dtb 的 gaokun3 内核）：每次发版 Release 里附 `egotouch_<版本>_arm64.deb` 与
  `egotouch-<版本>-1.aarch64.rpm`，`sudo apt install ./egotouch_*.deb` / `sudo dnf install ./egotouch-*.rpm` 后重启即可，卸载再重启回到 i2c-hid。
  包由 [build-packages.sh](/scripts/linux/package/build-packages.sh) 打，CI 在 ubuntu:26.04 的 arm64 容器里编（[ci-build.sh](/scripts/linux/package/ci-build.sh)），
  egotouchd 全静态链接，deb 与 rpm 用同一个可执行文件。装的东西：`/usr/bin/egotouchd`、`/usr/bin/egotouch-dtb`、udev 规则、
  `egotouchd.service`、modules-load 的 uhid。首次安装时调 [egotouch-dtb](/scripts/linux/egotouch-dtb) `enable`：内核带 himax-thp 和 touch-spi dtb、
  `/etc/kernel/devicetree` 没设或是默认 dtb 时，备份原值（`devicetree.egotouch-orig`）、换成 touch-spi dtb、重新生成当前内核的启动项；
  否则只提示、不切（例如原来指向 EL2 的 dtb，上游 EL2 暂停，touch-spi 没有 EL2 版本）。升级不再切，卸载时 `disable` 换回原值。
  `egotouch-dtb status` 看当前状态。**两种包还没在实机上装过**。
- **开机自启** `scripts/linux/`（从源码装）：`install.sh [--now]` / `uninstall.sh`。新内核下与安装包装的是同一套（egotouchd 在 /usr/local/bin），
  切 dtb 同样经 egotouch-dtb。下面是旧内核（没有树内 himax-thp）的装法：
  `/usr/local/bin/egotouchd`；当前内核的 `updates/egotouch/{himax-thp,thp-slave-dt}.ko`（depmod 后 udev 按 compatible 自动加载
  himax-thp）；`/etc/modprobe.d/egotouch.conf`（当前内核装了 himax-thp 就不加载 himax-spi，没装照常加载，升级内核后触摸退回原驱动）；
  `/etc/modules-load.d/egotouch.conf`（thp_slave_dt、uhid）；`/etc/udev/rules.d/60-egotouch.rules` + `egotouchd.service`
  （`/dev/himax_thp` 出现即拉起，`BindsTo` 设备）。日志在 `/var/log/egotouchd/`。不改 dtb、不改 initrd（himax-spi 不在 initrd 里）。
  `thp-bind.sh` 装过就用 modprobe，`off` 时用 `--ignore-install` 加载 himax-spi。
- **复位脉冲**：`ITouchBus::ResetPulse(holdMs)`。`hx_hw_reset_ahb_intf` 拉低 5 ms（同 himax-spi），安全模式重试 20 ms。
  Linux 上是驱动的 `RESET` ioctl，由内核计时、不足 5 ms 按 5 ms；Windows 上是 SpbThpTool 的 SET_RESET 低、等、高。
  脉冲太短 IC 没有真正复位，同一进程里第二次 Init 会在 FW reload 超时（早先 Linux 的 SET_RESET 只改电平时踩过）。

### 2.2 内核

- **`himax-thp.ko`**：spi_driver，匹配 `himax,hx83121a-ts`。Master 与 Slave 是同一颗硅片、同一个 compatible，被别的节点 `himax,cascade` 指向的是 Slave（**改成单一 compatible 后还没上实机复测**）。
  himax-spi 也匹配这个 compatible，打 overlay 时它必须不在，否则会去抢 Slave；`thp-bind.sh on` 先卸掉它。
  每一路各有自己的 spi 设备、中断、INT 脚，用户态只看到一个节点。DT 见 `bindings/himax,hx83121a-ts.yaml`：
  - Master 的 `himax,cascade` 指向 Slave。Master probe 时 Slave 还没注册或没绑定就 `-EPROBE_DEFER`，等 Slave
    绑定后驱动核心重试；接上后建 device link（Master 是 consumer），Slave 解绑前 Master 先被解绑。没有
    `himax,cascade` 时只有 Master 一路，绑定 Slave 那一路（`SET_CHANNEL(1)`）返回 `-ENODEV`。不再有全局变量。
  - `mode-gpios`（gpio174）：probe 时拉低（SPI 模式），之后不碰；复位后的 FW reload 采样它。可选，旧 DT 靠 pinctrl。
  - 复位脚、mode 脚、面板联动（drm_panel_follower）只在 Master 上。
  - 实测（7.2.12）：两路 1200 帧全部有效；解绑 Slave → Master 跟着解绑；重载模块时 Master 先 probe、被推迟、
    Slave 绑定后重试成功；不打 overlay 只有 Master 一路也正常。注意 sysfs 手动 `bind` 走 `device_driver_attach()`，
    推迟了不会进延迟队列，测延迟 probe 要重载模块。

  ABI 见 `include/uapi/linux/himax_thp.h`（与内核树同一相对路径，驱动和 TouchBusLinux 都写 `<linux/himax_thp.h>`；
  ioctl 号段 0xB9，说明文档在 PR 分支的 Documentation/misc-devices/himax-thp.rst）：
  - 一路只给一个文件绑定：打开后先 `SET_CHANNEL`，别的文件占着返回 `-EBUSY`；关掉文件，这一路停止读帧、清队列。
  - 驱动只提供总线原语：`BUS_READ` 发 `[读功能码, cmd, 0]` 收 len 字节，`BUS_WRITE` 发 `[写功能码, cmd, 数据]`。
    功能码与总线头由驱动按所绑通道填，用户态给不出任意字节（取代原来的全双工 `XFER`）。芯片的 Init、AFE 命令、
    读帧策略和 SOS 判定都在用户态，与 Windows 共用 `Device/himax/`。
  - `read()` 给「帧头 + 帧数据」：帧头有 CLOCK_MONOTONIC 中断时刻（硬中断里记）、序号、长度、累计丢帧数；
    帧数据不含 3 字节总线头，`TouchBusLinux` 在前面补回 3 字节，帧在缓冲里的布局仍与 Windows 一致。
  - 每一路一个深度 2 的帧队列，满了丢掉新读到的帧、计入 dropped，序号照样递增。这与 SpbThpTool 一致
    （§4：完成回调在 `REACH THP_LIST_MAX_FRAMES` 时直接返回，不入队）。丢帧可以用
    `echo 'module himax_thp +p' > /sys/kernel/debug/dynamic_debug/control` 看。
  - 面板状态字：bit0 = prepared（上电），bit1 = enabled（点亮），四个 follower 回调都会更新。
  没有模块参数：中断触发方式、SPI 模式都按 DT。
- **进 gaokun3/linux 的 PR**（分支 gaokun3-touch-spi，WSL 的 ~/gaokun/linux-touch-pr，已推到 awarson2233/linux，上游 PR 未开）：binding、dts（默认恢复主线的 i2c-hid，
  两路 SPI 写全但默认关，sc8280xp-huawei-gaokun3-touch-spi.dtb 打开 SPI）、himax-thp 进 drivers/misc、删掉 himax-spi。
  详见 [himax-thp-upstream-review.md](/docs/artifacts/himax-thp-upstream-review.md) §2、§5。PR 的内核装上之前，本机继续用下面的过渡方案。
- **`thp-slave-dt.ko`**（过渡）：运行时 DT overlay（`gaokun3-thp-slave.dtso`），内容与 PR 分支 dts 里的 SPI 描述相同：
  启用 spi20 和其下的 Slave，给 Master 节点加 `himax,cascade` 与 `mode-gpios`。insmod 时应用、rmmod 时撤销，
  不改启动用的 dtb。Master 只在 probe 时看 `himax,cascade`，所以 overlay 必须先于 himax_thp：
  `egotouch-modprobe.conf` 里有 `softdep himax_thp pre: thp_slave_dt`。装好后实测（7.2.12）：卸掉三个触摸模块、
  对 spi0.0 重放 udev `add`（开机 coldplug 的路径），himax-spi 被挡住，thp_slave_dt 先于 himax_thp 加载，Master 带
  Slave 绑定，egotouchd 被拉起并出帧。2026-10-26 重启验证：开机 4.9 s overlay、5.3 s Master 带 Slave 绑定、
  6.1 s uhid 触摸屏出现，himax-spi 未加载；实机触摸正常。
- **`thp-bind.sh on [--no-slave]|off|show`**：在原驱动 himax-spi 与 himax-thp 之间切换 Master。`on` 先加载 overlay
  再绑定 Master；`--no-slave` 不打 overlay。测完用 `off` 恢复系统触摸。
- 构建：`make -C Kernel/himax-thp`，默认 `KDIR=/lib/modules/$(uname -r)/build`（7.2.12 起装了 linux-headers-gaokun3，自带 Module.symvers）。
  经 Kbuild 的 `MO=` 把产物放到 `build/kernel/<KDIR 的内核版本>/`，`thp-bind.sh`、`install.sh` 都从那里取。
  在 WSL 里交叉编：`make -C Kernel/himax-thp KDIR=~/gaokun/work/kernel-out ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-`。
  7.1-rc3 下只能对着只做过 modules_prepare 的源码树编：`KDIR=~/repos/kernel/linux-7.1-rc3 KBUILD_MODPOST_WARN=1`。
  内核本身在 Windows 的 WSL 里编（gaokun3/linux merge stable + gaokun3/build 打 deb），不在实机 Linux 上整编（会过热关机，见 §7）。

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

**修法：** 启动 DT 里 Master 的 `ts0_default` 加 `mode-n-pins`（gpio174 `output-low`）。himax-spi probe 时先应用这组
引脚、再复位芯片，芯片开机就是 SPI 模式。源码在 linux-gaokun 本地克隆（`~/repos/kernel/ref/linux-gaokun`）的
`gaokun3-ts-spi-mode` 分支，`dts/sc8280xp-huawei-gaokun3.dts`；启动用的 dtb 见 §7。

**修好之后**（开机默认环境、非 DSC、geni FIFO 模式）：只有 himax-spi、不碰屏幕时 INT175 每秒 120 次；himax-thp
`HimaxThpProbe --with-slave` **120.0 fps**，1200 帧两路全部有效。Master 的 INT 每帧一次（120 次/秒）；
**Slave 每个 8.33 ms 周期出两帧**：读出来的帧都通过校验、内容各不相同，相邻两帧间隔 3.04 / 5.29 ms 交替（相加正好一个周期）。两路按下降沿触发（与 ACPI 一致，原因见 [himax-thp-upstream-review.md](/docs/artifacts/himax-thp-upstream-review.md) §3.1）时 Slave 的 INT 正好 240 次/秒、Master 120 次/秒（2026-10-27，PR 分支内核）；早先按低电平触发时 Slave 只有 190–210 次/秒，推测是电平方式下线程返回、解除屏蔽的时机与 INT 放开赛跑，有的帧被并进了上一次触发里。驱动把每一帧都读进队列，用户态（egotouchd、HimaxThpProbe）每个 Master 周期只取一帧 Slave，另一帧在队列满时被丢掉、计进 `dropped`，和 SPBTESTTOOL 的做法一致。两帧各装什么（笔的两个时隙？）还没对过，Windows 上 Slave 是不是也每周期两帧也还没量。SPI mode 0（DT 默认，与 ACPI 一致）、不设片选等待。

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

### 3.5 笔的蓝牙 MCU（`12d1:10b8`，USB）

笔的连接状态、型号、按键事件和压力都不经过触摸芯片，而是从这颗 MCU 来（Windows 上是 PenEventBridge / PenPressureReader）。
Linux 上它照常枚举，三个接口：

| 接口 | 类 | 端点 | Linux 驱动 | 用途 |
|---|---|---|---|---|
| 0 | HID | 中断 OUT 0x06 / IN 0x83，64 B | usbhid（hid-generic） | Report `0x55` = 压力（Windows `mi_00&col01`）；另有 `0x10` 厂商收发、`0x04` 鼠标、`0x0F` consumer |
| 1 | 厂商类 0xFF | bulk OUT 0x02 / IN 0x85，64 B | 无 | **事件 / 控制通道**（Windows 上的自定义 GUID 设备）：发 `07 …`，收 `02 00 07 00 01 EVT 11 LEN …` |
| 2 | HID | 中断 OUT 0x04 / IN 0x81 | hid-multitouch | 键盘 / 触摸板等，与笔无关 |

- 当前内核和 AUR linux-gaokun3 的配置都**没有 `CONFIG_HIDRAW`**（也没有 hiddev），用户态读不到 hidraw。
  接口 1 没有驱动，可以直接用 usbfs 认领；接口 0 的压力报告要么用 usbfs 把它从 usbhid 摘下来，要么开 HIDRAW。
- 实测（usbfs 认领接口 1）：发 `0x7101`（查笔状态）回 `02 00 07 00 01 71 11 01 00`（没有笔连接），
  发 `0x7701` 回 `… 77 11 01 01`。协议见 `EGoTouchService/Device/penevt/BTMCU_PROTOCOL.md`。
- 没有笔时接口 0 被动监听 120 s 一包都没有。

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

- 每一路在自己的帧中断线程里读整帧；用户态 `INT_ENABLE(1)` 打开某一路时立刻读一帧（同 SpbThpTool 的 `INT_OPEN`）。
- 两路 INT 按下降沿触发（DT），每帧一个沿、读一次，不读 INT 电平。读帧失败时那一路停住，用户态超时后重开恢复。DT 仍是低电平时 probe 会告警。中断的触发方式在本次开机第一次映射时定下：这次开机 himax-spi 按启动 dtb 的低电平绑定过 Master 之后，打上 overlay 的 himax-thp 映射不上 Master 的中断，要重启（`thp-bind.sh on`、`install.sh --now` 都受影响）。
- 帧进每一路深度 2 的队列（满了丢新帧，同 SpbThpTool），read 带帧头（时间戳、序号、丢帧数）。
- 没有模块参数。排查期间加过的轮询取帧、AHB 就绪探测、拼帧读法、触发方式 / SPI 模式覆盖、`read_extra`、
  `cs_setup_us`、gpio174 兜底、GSI 选项都已删掉（都是 I2C-HID 模式下的权宜之计或已排除的方向，见 §3.3）。

## 6 未解决的问题与下一步

1. 熄屏亮屏、面板复位之后的行为：复位会触发 FW reload，只要 gpio174 还是低就没问题，但要实测。
2. Slave 的帧校验：SPI 模式下已经通过（偶尔每轮开头 1–2 帧坏，原因待查）。
3. 开机自启已重启验证：himax-thp 自动绑定 Master + Slave，himax-spi 未加载，服务开机约 5 s 进入出帧。
   换内核要重新编、重新装模块（以后做 DKMS）。
4. 偶发的取帧停顿：Init 之后约 8 s（IC 还没进 IDLE）两路都不来帧约 1 s，判 SOS 后重建链恢复。开机后、
   从手动进程切到服务时各见过一次，之后不再出现。期间触摸约 1 s 无响应。诱因未明（停顿前日志里没有别的记录，
   AFE 命令只在失败时记日志）。IDLE 里静默 3.6 s 判 SOS 是原厂常态，不算在内。
5. 之后的工作：笔（等有笔再做：MCU 通道的 Linux 传输、笔状态 → AFE 的 InitStylus / SetStylusId、压力进 StylusPipeline、Report 8 的 uhid 上报，见 §3.5）、egotouchd 跟随亮灭屏（现在靠总线探测）、其余 Port（面板电源、输入、笔 USB、系统状态）、
   DKMS、TsaRawReplay 逐位验证（需要 Windows 录制的 .dvrbin）、Windows CI 验证 Port 改动。

## 7 环境注意事项

- **整编内核会过热关机**（CPU 热区只有 110 °C 的 critical trip，没有被动降频；EC 温度不进 Linux 热管理），已发生两次。
  只编树外模块。`~/repos/kernel/guarded-build.sh` 是带温控暂停的编译脚本，备用。
- **实际启动用的 ESP 是 sda1（CPRA_A64FRE），不是挂在 `/boot/efi` 的 sda3**；两边 `loader/entries` 各有一份。
  装内核包只会写 sda3，要把 `ubuntu/<版本>/` 和 `ubuntu-<版本>.conf` 手动复制到 sda1。sda1 的 `loader.conf` 默认项已改成 7.2.12，
  原文件备份为 `loader.conf.bak-7.1`；7.1-rc3 的两个启动项仍在，作为退路（它的模块已从备份 `/root/kernel-backup-7.1.0-rc3-gaokun3+` 放回）。
  为 DSC 测试加的 `…-gaokun3+-dsc.conf` 在两边都有（oneshot 已用掉，默认项未改），不需要可删除。
- 内核参数里有 `efi=noruntime`，但 `bootctl set-oneshot` 仍能写 EFI 变量。
- 用 GNOME 的 PowerSaveMode 关屏再开屏会触发 DPU `ctl reset failure / vblank timeout`，之后芯片 Init 在 FW reload 超时——
  **不要用关屏来复位 TDDI**。
- `/tmp`（含会话 scratchpad）是 tmpfs，重启即清空；需要保留的提取件放在 `~/repos/kernel/ref/`。
- **默认启动项用的是改过的 dtb** `sc8280xp-huawei-gaokun3-tsmode.dtb`（两个 ESP 上都有）：只在启动用的原 dtb 的
  `ts0_default` 里加了 `mode-n-pins`（gpio174 `output-low`），反编译对比只多这一个节点；dts 在 `~/repos/kernel/dt-ts-mode/`。
  实测开机后 gpio174 为低、归 spi0.0，只有 himax-spi 时 INT175 就是 120 次/秒。原 dtb 留着；`…-dsc.conf` 仍指向原 dtb。
  **kernel-install / 升级内核会重写启动项和 dtb**：届时用 linux-gaokun 的 `gaokun3-ts-spi-mode` 分支（§3.3）重新编 dtb。
  注意内核树 `~/repos/kernel/linux-7.1-rc3` 里的 gaokun3 dts 是主线版（触摸写成 I2C-HID），不是启动 dtb 的来源。
- `sudo dmesg` 里 overlay 的 `memory leak will occur if overlay removed` 警告是内核对 overlay 撤销的固定提示，可忽略。
