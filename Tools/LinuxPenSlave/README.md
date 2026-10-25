# LinuxPenSlave — Linux 侧参考实现（从片笔）

> **不属于 CMake 构建**。`Tools/` 下的各项互相独立，顶层 `CMakeLists.txt` 也没有
> `add_subdirectory(Tools/...)`，这两个 Python 文件只是参考实现，方便对照协议。
>
> 背景与全部实测数据见 [`docs/linux-pen-slave.md`](../../docs/linux-pen-slave.md)。

## 这是什么

同一台机器（MateBook E Go / gaokun3，CSOT `ppc357db1-4` + HX83121A **级联从片**）
在 Linux 上的笔用户态实现。与本项目（Windows）走的是**两条不同的路**：

| | 本项目 | 这里 |
|---|---|---|
| 笔坐标 | 主片 / BT-MCU | **级联从片 `spi20` 的原始帧** |
| 平台 | Windows 11 ARM64 | Linux 6.9+（实测 7.2.9）|
| 输出 | HID 注入 | uinput stylus + cursor 镜像 |

两条路共用同一套硬件约定（`raw_out_sel=0xF6`、`rawdata_addr=0x5AA5`、
`ICID=0x83121a00`），所以这份代码的意义主要是**协议对照**，而不是让人在 Windows 上跑。

## 文件

| 文件 | 作用 |
|---|---|
| `gk3pend.py` | 读 `/dev/gk3_pen` 的从片帧 + 读键盘套 MCU（`12d1:10b8` if0，ReportID `0x55`）的压感 → 合成一支 uinput 笔；含镜像设备（Mutter 50 下补 `BTN_LEFT`）|
| `gk3pen_solver.py` | 帧 → 坐标 / 倾斜 / 压感的解算与标定 |

纯标准库，无第三方依赖。

## 前置条件

内核侧不在这里（GPL-2.0）：`mashen11/MateBookEgo-linux-kernel`
→ `patches/common/linux-gaokun/0083`（从片驱动）、`0084`（DT）、`0085`、`0086`、`0110`。

`0110` 那条值得单独看：**从片的初始化必须晚于主片 FW reload**，否则 TX1 锚点有效率
只有 12~28%，表现为"悬停光标满屏乱跳"，详见文档 §3.4。

## 运行

```bash
# 内核加载出 /dev/gk3_pen 之后
sudo systemctl stop gk3pend 2>/dev/null
sudo python3 gk3pend.py --usb-id 12d1:10b8 --frame portrait-swap
```

`--usb-id` 与 `--frame portrait-swap` **必须成对**：前者让 uinput 报 `BUS_USB`
（否则 libwacom 的 `DeviceMatch` 匹配不上、应用读不到压感/倾角），后者抵消
Mutter 对"平板设备"做的轴转置。

## 许可

本目录两个文件按 **MIT** 提供（见文件头 SPDX）。`gk3pen_solver.py` 的算法移植自
Android 侧 `gk3pend/solver.h`（SPDX: `GPL-2.0 AND MIT`），双许可允许以 MIT 分发；
其 HPP3 步骤的更早来源即本项目的 `EGoTouchRev-rebuild`。原始声明保留在模块 docstring 里。
