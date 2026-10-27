#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# Copyright (c) 2026 mashen11
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# --- Provenance (read before judging the license) ---------------------------
# # Independent Python implementation of a userspace stylus consumer:
# slave-pen frames from /dev/gk3_pen + pressure from the keyboard-case MCU
# (USB 12d1:10b8, interface 0, ReportID 0x55) fused into one uinput stylus.
# The *protocol facts* (register conventions, frame layout, MCU event ids) were
# established by our own experiments and cross-checked against this repository's
# docs/penservice_events.md; no code is copied from EGoTouchRev.
# Design notes reference the Android-side gk3pend.c (device/huawei/gaokun3/
# stylus/), whose solver.h is SPDX: GPL-2.0 AND MIT -- the dual license allows
# this file to be distributed under MIT alone.
# ---
"""gk3pend.py —— gaokun3 手写笔用户态消费者（v0.2 骨架）

对应上游 `gk3pend`。职责：把**从片**的笔帧（`/dev/gk3_pen`）与**笔 MCU** 的
压感/侧键/电量**融合**成一支 uinput 触控笔交给输入栈。

── v0.2 相对 v0.1 的关键修正（2026-10-11，实机踩到）
   v0.1 假设 `/dev/gk3_pen` 吐的是**裸 336 B 帧**，于是自己按 magic 找帧头。
   实际上驱动吐的是 **`struct hs_frame`（560 B）**：48 B 已解码头部 + `data[512]`，
   裸帧在 `data` 里（偏移 48）。而且驱动的 `hs_dev_read()` **非阻塞**：
   环空时返回 `-EAGAIN`（`hs_dev_read()`: `if (!hs->ring_count) return -EAGAIN;`）。
   v0.1 的两个后果：① 找不到帧头 ⇒ 一个帧都不产出；② 用 `read` 阻塞等数据 ⇒
   表现成"挂住"。现在改为：读 560 B 记录、用头部字段、fd 开 `O_NONBLOCK` + 轮询。
   ★ 另：v0.1 把 magic 写成 `unpack("<H") == 0x5AA5`，而帧头字节是 `5a a5`，
     小端 u16 应是 **0xA55A** —— 比较必须按**字节**（`b"\\x5a\\xa5"`），这才不会写错。

── v0.3 记录格式改为**双格式自适应**（2026-10-11，路线 A：内核从片驱动统一到上游）
   路线 A 之后内核用的是**上游驱动** `himax_hx83121a_pen.c`，它吐的是 **`struct pen_rec`（352 B）**：
   `u64 t_ns; u32 seq; u32 flags; u8 data[336]` ⇒ 裸帧在偏移 **16**，`seq` 在偏移 **8**，
   `flags bit0 = TX1 anchor valid`。
   ⚠️ **别漏 `t_ns`**：按 "seq+flags+data=344" 算过一次，真机读出来的却是 352
   ⇒ 分支判错、seq 读成 t_ns 低 32 位、裸帧整体错位 8 字节 ⇒ 笔一支都解不出来。
   它与旧的 `hs_frame`（560 B）**裸帧完全一致**，只差记录头。本版按 `len(read())`
   自动二选一（560 ⇒ 旧，352 ⇒ 新）⇒ **同一份 gk3pend 在换核前后都能跑**，
   回退内核也不用改脚本。★ 上游 `gk3pend.c` 用的同样是 `STALE_MS=150`/`RELEASE_DELAY=30`，
   与下面一致，故判据无需改动（上游驱动虽只交付"内容变化"的帧，但静止时从片仍有噪声）。

── 本版能做的（不依赖帧语义）
     1. uinput 笔设备（BTN_TOOL_PEN/RUBBER/TOUCH/STYLUS/STYLUS2 +
        ABS_X/Y/PRESSURE/DISTANCE/TILT_X/TILT_Y + INPUT_PROP_DIRECT）
     2. 读从片帧并 1 Hz 统计（用驱动已解码的 peak1/peak2/valid1/valid2/ctr）
     3. `--selftest` 发合成轨迹 —— 端到端证明"应用能看到一支笔"，与帧语义无关
     4. `--dump-arrays N` 打印前 N 帧的 tx1/tx2 9×9 阵列（T0.1「是不是位置图」的原料）

── 不做的（后续）
     · 从片帧 → 坐标/悬停/倾斜的**解算**（要先定案"81 槽阵列是不是位置图"）
     · MCU 压感/侧键/电量（★ MCU 的 usbfs 接口 1 目前由 `pen-dock` 独占，
       两个进程不能同时 claim ⇒ 融合要么走 IPC，要么把 MCU 收归本 daemon）

用法：
    sudo python3 gk3pend.py                    # 建笔 + 读帧 + 1 Hz 日志
    sudo python3 gk3pend.py --selftest         # 只发合成轨迹（验 uinput 通路）
    sudo python3 gk3pend.py --no-uinput --dump-arrays 3 --seconds 12
"""
import argparse
import ctypes
import errno
import fcntl
import os
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    import gk3pen_solver as S
except ImportError:                     # 允许和 solver 放同一目录之外
    S = None

# ─────────────────────── 设备与帧格式 ───────────────────────
FRAME_DEV = "/dev/gk3_pen"
UINPUT_DEV = "/dev/uinput"

# ── 记录格式：**两种内核并存**（路线 A 前后），按**实际读到的长度**自动判形态 ──
#   ① 自制驱动（2026-10-11 前，已退役）: struct hs_frame，560 B
#        u32 seq, len, nonzero, ctr, f1a, f1b, f2a, f2b;  int peak1, peak2;  u32 valid1, valid2;
#        u8  data[HS_MAX_FRAME=512]                       ← 裸帧在偏移 48
#   ② 上游驱动（路线 A 现役）: struct pen_rec，**352 B**
#        u64 t_ns;  u32 seq;  u32 flags;  u8 data[FRAME_LEN=336]
#        ⇒ 裸帧在偏移 **16**、seq 在偏移 **8**（见下面 HDR_FMT_NEW 的说明）
#        flags bit0 = "TX1 anchor valid"（≈ 旧的 valid1）
#   ★ 两种格式的**裸帧完全相同**（336 B；magic / anchor / TX1 / TX2 偏移一致 —— 上游
#     pen_account() 的 WORDS_OFF=0x04、TX2_WORD=83 与下面的 OFF_T1*/OFF_T2* 逐一对得上），
#     差别只在记录头 ⇒ 只在"解头"这一处分叉，solver 与坐标链一行不动。
HDR_FMT_OLD = "<12I"                     # 12 × u32；peak1/peak2 是 int，同宽
REC_LEN_OLD = 48 + 512                   # = 560
DATA_OFF_OLD = 48
# 上游 struct pen_rec（himax_hx83121a_pen.c:156）：
#     u64 t_ns; u32 seq; u32 flags; u8 data[FRAME_LEN];
#   ⇒ 8 + 4 + 4 + 336 = **352**，裸帧偏移 **16**，seq 偏移 **8**
# ★★ 2026-10-11 真机实测的教训：一开始按 "seq+flags+data = 344"（**漏了 t_ns**）写，
#    于是 len(rec)=352 走错分支、`seq` 从偏移 0 读（读成 t_ns 低 32 位）、裸帧整体错位
#    8 字节 ⇒ 驱动明明在出帧，用户态却一支笔都解不出来。**结构大小必须从驱动源码量**。
HDR_FMT_NEW = "<QII"                     # t_ns(u64), seq(u32), flags(u32)
REC_LEN_NEW = 8 + 4 + 4 + 336            # = 352（sizeof(struct pen_rec)）
DATA_OFF_NEW = 16                        # 裸帧在 t_ns+seq+flags 之后
SEQ_OFF_OLD = 0                          # seq 在旧格式头里的偏移
SEQ_OFF_NEW = 8                          # seq 在新格式头里的偏移
REC_LEN_MAX = max(REC_LEN_OLD, REC_LEN_NEW)   # 读时按最大读，靠返回长度判形态
FRAME_LEN = 336                          # HS_FRAME_LEN == pen_rec.data 长度
# 兼容旧名（历史代码与日志里还在用）
REC_LEN = REC_LEN_MAX
HDR_FMT = HDR_FMT_OLD
DATA_OFF = DATA_OFF_OLD
MAGIC = b"\x5a\xa5"                      # ★ 按字节比，别用 unpack("<H")==0x5AA5
NLOT = 81                                # HS_FRAME_NLOT
COLS = 9                                 # 9×9 排布；col 0..1 恒零 ⇒ 真实通道 63 = 9×7
REAL_COLS = 7
OFF_T1A, OFF_T1B, OFF_T1 = 0x04, 0x06, 0x08
OFF_T2A, OFF_T2B, OFF_T2 = 0xAA, 0xAC, 0xAE

# input 事件码
EV_SYN, EV_KEY, EV_ABS = 0x00, 0x01, 0x03
ABS_X, ABS_Y = 0x00, 0x01
ABS_PRESSURE, ABS_DISTANCE, ABS_TILT_X, ABS_TILT_Y = 0x18, 0x19, 0x1A, 0x1B
BTN_TOOL_PEN, BTN_TOOL_RUBBER = 0x140, 0x141
BTN_TOUCH, BTN_STYLUS, BTN_STYLUS2 = 0x14A, 0x14B, 0x14C
BTN_RIGHT = 0x111
BTN_MIDDLE = 0x112
BTN_LEFT = 0x110
INPUT_PROP_DIRECT = 0x01
BUS_USB = 0x03


def _IOC(d, t, nr, size):
    return (d << 30) | (size << 16) | (ord(t) << 8) | nr


UI_SET_EVBIT = _IOC(1, 'U', 100, 4)
UI_SET_KEYBIT = _IOC(1, 'U', 101, 4)
UI_SET_ABSBIT = _IOC(1, 'U', 103, 4)
UI_SET_PROPBIT = _IOC(1, 'U', 110, 4)
UI_DEV_CREATE = _IOC(0, 'U', 1, 0)
UI_DEV_DESTROY = _IOC(0, 'U', 2, 0)
UI_DEV_SETUP = _IOC(1, 'U', 3, 92)
UI_ABS_SETUP = _IOC(1, 'U', 4, 28)


class InputAbsinfo(ctypes.Structure):
    _fields_ = [("value", ctypes.c_int32), ("minimum", ctypes.c_int32),
                ("maximum", ctypes.c_int32), ("fuzz", ctypes.c_int32),
                ("flat", ctypes.c_int32), ("resolution", ctypes.c_int32)]


class UinputAbsSetup(ctypes.Structure):
    _fields_ = [("code", ctypes.c_uint16), ("pad", ctypes.c_uint16),
                ("absinfo", InputAbsinfo)]


class InputId(ctypes.Structure):
    _fields_ = [("bustype", ctypes.c_uint16), ("vendor", ctypes.c_uint16),
                ("product", ctypes.c_uint16), ("version", ctypes.c_uint16)]


class UinputSetup(ctypes.Structure):
    _fields_ = [("id", InputId), ("name", ctypes.c_char * 80),
                ("ff_effects_max", ctypes.c_uint32)]


class InputEvent(ctypes.Structure):
    _fields_ = [("sec", ctypes.c_int64), ("usec", ctypes.c_int64),
                ("type", ctypes.c_uint16), ("code", ctypes.c_uint16),
                ("value", ctypes.c_int32)]


# ── ★★ 输出坐标框（2026-10-11 实测定案，用户报"笔上下动→箭头左右动"）─────────────
# 本机面板原生是 **1600×2560 竖屏**，Mutter 用 **transform 270°** 把它显示成横屏
# 2560×1600（证据：Mutter DisplayConfig GetCurrentState 里 logical monitor 的
# transform=uint32 3；`cat /sys/class/drm/card1-DSI-1/modes` = 1600x2560）。
#
# 对 `INPUT_PROP_DIRECT` 设备，**合成器会按输出的 transform 再转一次**。
# 同机的"手指触摸屏"（Himax Capacitive TouchScreen）在 libinput 里报的是
# **Size 160x256mm（竖屏框）**，它工作正常 —— 说明要按面板**原生竖屏框**发坐标，
# 让 Mutter 去转。我们原先按横屏发（Size 256x160mm）⇒ 被多转了一次 ⇒ 上下变左右。
#
# 反推验证（与用户现象一致）：原代码 `emit_x=Lx, emit_y=Ly` ⇒ 光标x ∝ emit_y = Ly
#   ⇒ "笔往下(Ly↑) → 光标往右" ✓ 正是用户看到的。⇒ 90° 纯旋转 ⇒ 交换 + 一个反向。
#
# 实现：内部仍一切照旧算**横屏像素 (x 0..2559, y 0..1599)**（solver/自检/画线测试都基于它），
# 只在**最后 emit 那一步**换框：
#       emit_x(0..1599) = 1599 - y
#       emit_y(0..2559) = x
# 配套：uinput 的 ABS 量程也要跟着改成 **X 0..1599 / Y 0..2559**（否则 libinput 报的 Size 还是横屏）
FRAME_PORTRAIT = "portrait"
FRAME_LANDSCAPE = "landscape"
FRAME_PORTRAIT_SWAP = "portrait-swap"


def frame_emit(x, y, frame=FRAME_PORTRAIT, flip_x=False, flip_y=False):
    """横屏像素 (x,y) → 按指定帧要发出去的 (ex, ey)。

    ★★ 2026-10-11 两次实测定案（我第一版推导"交换"是错的，如实记下）：

      | 版本 | 声明框 | 用户实测耦合 | 反推的 Mutter 映射 |
      |---|---|---|---|
      | 上一版 | 横屏 256x160mm | 笔下 → 光标右 | **cx←emit_y, cy←emit_x**（有旋转）|
      | 当前版 | 竖屏 160x256mm | 笔右 → 光标**下** | **cx←emit_x, cy←emit_y**（identity）|

      ⇒ 改声明框会**改变 Mutter 的映射**（device 的 absinfo aspect 与输出匹配时它不转）。
        我第一版错在假设"Mutter 一定按输出 transform 转一次"。

      ⇒ 正确做法：**声明框保持竖屏**（libinput 的 Size 必须等于手指屏的 160x256mm，
        这也是"aspect 匹配"的前提），**但坐标值按横屏语义逐轴重标度**塞进两个槽位 —— 不交换。
    """
    if frame == FRAME_LANDSCAPE:
        ex, ey = x, y
    elif frame == FRAME_PORTRAIT_SWAP:
        # ★★ 2026-10-11（同一天第二次）：**必须**配合 `--usb-id` 用。
        #
        # 起因：加了 `--usb-id` 让 libwacom 认下这支笔（修压感/倾角不可见）之后，
        # 用户报**光标方向又错了**（笔上→光标左、笔下→光标右）。
        #
        # 实测（`pen-inject.py --raster 40` 灌 80 个互异坐标，`pen-input-probe.py
        # --motion-log` 落盘，79/80 配对成功）：
        #     注入 ax 从 0→满（ay 固定） ⇒ 客户端 x 跨度 **0.0**、y 跨度 786
        #     注入 ay 从 0→满（ax 固定） ⇒ 客户端 x 跨度 1215、y 跨度 293
        #   拟合: client_x = 58 + 0.508·ey        （只跟 ey 走，且递增）
        #         client_y = 923.6 − 0.570·ex     （只跟 ex 走，且递减）
        # ⇒ Mutter 对**被认作平板的**设备做了 **轴转置 + ex 反向**。
        #
        # 补偿：预先把 (x,y) 转置着塞进槽位 ——
        #     ex ← 1599 − y·1599/1600   （y 越大 ex 越小；经 client_y = 923.6−0.570·ex
        #                                得到 client_y 随 y **递增** ✓）
        #     ey ← x·2559/2560          （client_x = 58+0.508·ey 随 x **递增** ✓）
        #
        # ⚠ `--frame landscape` **不能**代替它：实测 landscape 框下客户端输出与
        #   portrait 逐点相同（映射是归一化的，换框不改转置）。
        # ⚠ 不加 `--usb-id` 时**不要**用这个（那时映射不转置，用了反而错）。
        ex = 1599 - y * 1599 // 1600
        ey = x * 2559 // 2560
    else:
        # 逐轴重标度：横屏的量程 → 槽位量程（portrait 声明：X 0..1599、Y 0..2559）
        ex = x * 1599 // 2559          # 横屏 x（0..2559）→ ABS_X 槽（0..1599）
        ey = y * 2559 // 1599          # 横屏 y（0..1599）→ ABS_Y 槽（0..2559）
    if flip_x:
        ex = (2559 if frame == FRAME_LANDSCAPE else 1599) - ex
    if flip_y:
        ey = (1599 if frame == FRAME_LANDSCAPE else 2559) - ey
    return ex, ey


def tilt_axes(out_c, out_r, sx=1, sy=1):
    """物理倾斜分量 (out_c, out_r) → 发出去的 (ABS_TILT_X, ABS_TILT_Y)。

    ★★ 2026-10-11 实测定案（`scripts/stylus/pen-tilt-cal.py`，5 段，笔尖位移 <25 px 全有效）：

      | 姿势        | Δraw_dc（列轴差） | Δraw_dr（行轴差） |
      |---|---|---|
      | 右倾        | **+1051**  | −252  |
      | 左倾        | **−1449**  |  +13  |
      | 笔尾朝自己  |    +2      | **+926**  |
      | 笔尾朝上    |  −162      | **−1490** |

      ⇒ **左右倾只改列轴（out_c）、前后倾只改行轴（out_r）**，分离度 4×~460×，没有歧义。
      ⇒ 本机 `frame_emit` 是逐轴 identity 映射（见上），
        设备 ABS_X ↔ 横屏 x、ABS_Y ↔ 横屏 y ⇒ `tilt_x ← out_c`、`tilt_y ← out_r`。

    ⚠ 旧代码照上游写成 `tilt_x = out_r` / `tilt_y = out_c` —— **两轴是反的**。
      （上游面向的是它自己的另一套坐标框，且它的注释里把 flip 与"换轴"混在一起，
        不能直接搬。**以本机实测为准**。）

    符号照 libinput 官方定义（doc/tablet-support.dox）：
        "a positive tilt angle thus means that the stylus' top is tilted
         towards the logical right and/or bottom of the tablet"
      实测右倾 Δout_c>0、笔尾朝自己 Δout_r>0 ⇒ **两轴符号都是 +1**，与上表自洽。
      方向若在真实应用里仍反，用命令行 `--tilt-sx -1` / `--tilt-sy -1`（不要改这里的默认值）。
    """
    return out_c * sx, out_r * sy


# 坐标系 = 面板**原生框**（默认 portrait）。理由见上。
# ⚠ 改 frame 必须同时改这里的量程，否则 libinput 的 Size 会对不上、映射仍然错。
ABS_SPEC_PORTRAIT = [
    # ⚠ 2026-10-13 试过把 resolution 10→8/9（想让映射覆盖全屏）：**实测无效** ——
    #   重测出来的映射与改前逐个数字相同 ⇒ 尺寸不取自内核 absinfo，可能来自 libwacom 的
    #   `Width/Height`（现为 10x6，真实屏 256x160mm）。已回退，保持 10。
    (ABS_X,           0,   1599,     0,    0,   10),
    (ABS_Y,           0,   2559,     0,    0,   10),
    (ABS_PRESSURE,    0,   4095,     0,    0,    0),
    (ABS_DISTANCE,    0,    255,     0,    0,    0),
    (ABS_TILT_X,    -90,     90,     0,    0,    0),
    (ABS_TILT_Y,    -90,     90,     0,    0,    0),
]
ABS_SPEC_LANDSCAPE = [
    (ABS_X,           0,   2559,     0,    0,   10),
    (ABS_Y,           0,   1599,     0,    0,   10),
    (ABS_PRESSURE,    0,   4095,     0,    0,    0),
    (ABS_DISTANCE,    0,    255,     0,    0,    0),
    (ABS_TILT_X,    -90,     90,     0,    0,    0),
    (ABS_TILT_Y,    -90,     90,     0,    0,    0),
]
ABS_SPEC = ABS_SPEC_PORTRAIT          # 兼容旧引用（selftest 等）
KEY_SPEC = [BTN_TOOL_PEN, BTN_TOOL_RUBBER, BTN_TOUCH, BTN_STYLUS, BTN_STYLUS2]


def log(*a):
    print(time.strftime("%m-%d %H:%M:%S"), *a, flush=True)


# ─────────────────────── uinput 笔 ───────────────────────
class Pen:
    def __init__(self, name="gk3 M-Pencil", frame=FRAME_PORTRAIT, usb_id=None):
        # portrait-swap 只是**发射时**转置，设备声明的槽位量程仍是竖屏那套
        spec = ABS_SPEC_LANDSCAPE if frame == FRAME_LANDSCAPE else ABS_SPEC_PORTRAIT
        self.fd = os.open(UINPUT_DEV, os.O_WRONLY | os.O_NONBLOCK)
        for kind, bits in ((EV_KEY, KEY_SPEC), (EV_ABS, [s[0] for s in spec])):
            fcntl.ioctl(self.fd, UI_SET_EVBIT, kind)
            for b in bits:
                fcntl.ioctl(self.fd, UI_SET_KEYBIT if kind == EV_KEY else UI_SET_ABSBIT, b)
        fcntl.ioctl(self.fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT)
        setup = UinputSetup()
        # ★★ 设备身份决定 GNOME 认不认它是"平板"（2026-10-11 定案，见 `--usb-id`）：
        #   默认 BUS_VIRTUAL + 12d1:cd54（与上游同口径）。
        #   ⚠ 但 **libwacom 的 DeviceMatch 只认 `bus:vid:pid`**，`BUS_VIRTUAL` 永远匹配不上
        #   `/etc/libwacom/gaokun3-m-pencil.tablet` ⇒ `libwacom-list-local-devices` 会说
        #   "is a tablet but not supported by libwacom" ⇒ gsd-wacom/Mutter 建不出工具
        #   ⇒ 客户端拿不到 `GdkDeviceTool` ⇒ **应用收不到压感与倾角**（`GestureStylus` 哑火）。
        #   实测：libinput 侧一切正常（`TABLET_TOOL_AXIS` 带 tilt/pressure/distance 1466 条），
        #   断点就在这一跳。加 `--usb-id 12d1:10b8`（= 键盘套 MCU 的 VID/PID，
        #   10-07 那份 libwacom 记录就是按它写的）即报 BUS_USB，可被 libwacom 匹配。
        if usb_id:
            v, p = (int(x, 16) for x in usb_id.split(":"))
            setup.id = InputId(BUS_USB, v, p, 1)
            log("uinput 设备身份: BUS_USB vid=0x%04X pid=0x%04X（为 libwacom 匹配）" % (v, p))
        else:
            setup.id = InputId(0x06, 0x12D1, 0xCD54, 1)
        setup.name = name.encode()[:79]
        fcntl.ioctl(self.fd, UI_DEV_SETUP, setup)
        for code, mn, mx, fz, fl, res in spec:
            s = UinputAbsSetup()
            s.code = code
            s.absinfo = InputAbsinfo(0, mn, mx, fz, fl, res)
            fcntl.ioctl(self.fd, UI_ABS_SETUP, s)
        fcntl.ioctl(self.fd, UI_DEV_CREATE)
        time.sleep(0.4)
        log("uinput 笔已建立: %s（%s 框，ABS_X 0..%d / ABS_Y 0..%d）"
            % (name, frame, spec[0][2], spec[1][2]))

    def emit(self, etype, code, value):
        # ★★ 2026-10-16 事故：上游算出浮点（`--x-smooth` 低通）⇒ InputEvent 只收 int ⇒
        #   TypeError ⇒ daemon 每次笔一动就崩，笔完全没反应（NRestarts 涨到 10）。
        #   ⇒ 在**唯一的 uinput 出口**处强制取整：以后任何上游浮点都不再能打死服务。
        os.write(self.fd, bytes(InputEvent(0, 0, etype, code, int(value))))

    def syn(self):
        self.emit(EV_SYN, 0, 0)

    def close(self):
        try:
            fcntl.ioctl(self.fd, UI_DEV_DESTROY)
        except OSError:
            pass
        os.close(self.fd)
        log("uinput 笔已销毁")


# ─────────────────── 光标镜像（非平板绝对指针）───────────────────
SCREEN_W, SCREEN_H = 2560, 1600        # 横屏像素（与 frame_emit 的量纲一致）


class CursorMirror:
    """★ 2026-10-13：让 Mutter 的「核心指针」跟着笔走 —— 修「抬笔后光标回跳」。

    ## 根因（Mutter 46.2 `src/backends/native/meta-seat-impl.c` 逐行实证）

        static void
        update_device_coords_in_impl (seat_impl, input_device, coords)
        {
          if (clutter_input_device_get_device_type (input_device) == CLUTTER_TABLET_DEVICE)
            device_native = META_INPUT_DEVICE_NATIVE (input_device);   // 平板：只写设备自己的
          else
            {
              device_native = META_INPUT_DEVICE_NATIVE (seat_impl->core_pointer);
              seat_impl->pointer_x = coords.x;                         // ★ 只有非平板写「核心指针」
              seat_impl->pointer_y = coords.y;
            }
          meta_input_device_native_set_coords_in_impl (device_native, coords.x, coords.y);
        }

    ⇒ **笔（平板）的坐标永远进不了核心指针位置**。于是任何一次"把光标重新同步到
      核心指针"的动作，都会让光标跳回**鼠标/触摸板上次停留的位置** —— 也就是用户看到的
      「手写笔抬起后，光标回到某个固定位置」。

    ## 修法

    再造一个**非平板**的 uinput 设备：`EV_ABS + EV_KEY`，**不设** `INPUT_PROP_DIRECT`。

    - libinput 实测认成 `cap:p`（指针，不是 `cap:t` 平板），事件是
      `POINTER_MOTION_ABSOLUTE`，归一化后 **1:1、无指针加速**
      （注入 (300,200) ⇒ libinput 报 11.72/12.50 = 300/2560、200/1600）；
    - Mutter 46.2 的 `process_device_event()` 对该事件走
      `meta_seat_impl_notify_absolute_motion_in_impl()` ⇒ `update_device_coords_in_impl()`
      的 **else 分支** ⇒ **核心指针位置被更新**；
    - 抬笔后不再发坐标 ⇒ 核心指针就停在笔最后的位置 ⇒ 光标**不再回跳**。

    ## 安全

    - `BTN_LEFT/MIDDLE` **只声明、永不发送**；`BTN_RIGHT` 仅由 `click()` 在「长按右键」
      时发一次（2026-10-17 起由此承担右键）⇒ 不会产生幽灵点击，也不会误发左键；
    - `BUS_VIRTUAL` + 专用 VID/PID ⇒ 不会被 libwacom 当成平板匹配、不进 GNOME 的平板列表；
    - 只发 `ABS_X/ABS_Y + SYN`，不碰任何按键。

    ## 回退

    `--no-cursor-mirror` 关掉即可（或 `systemctl` 的 ExecStart 里加这个参数）。
    """

    def __init__(self, name="gk3 M-Pencil (cursor)", w=SCREEN_W, h=SCREEN_H):
        self.w, self.h = w, h
        self.fd = os.open(UINPUT_DEV, os.O_WRONLY | os.O_NONBLOCK)
        fcntl.ioctl(self.fd, UI_SET_EVBIT, EV_KEY)
        fcntl.ioctl(self.fd, UI_SET_EVBIT, EV_ABS)
        # ★ 必须声明这三个 libinput 才认成指针；实际只有 BTN_RIGHT 会被 click() 用到
        #   （BTN_LEFT/MIDDLE 永不发送，避免幽灵点击）
        for b in (BTN_LEFT, BTN_RIGHT, BTN_MIDDLE):
            fcntl.ioctl(self.fd, UI_SET_KEYBIT, b)
        for code in (ABS_X, ABS_Y):
            fcntl.ioctl(self.fd, UI_SET_ABSBIT, code)
        # ★★ 故意**不**调 UI_SET_PROPBIT(INPUT_PROP_DIRECT) —— 这是"指针 vs 平板"的分界
        setup = UinputSetup()
        setup.id = InputId(0x06, 0x12D1, 0x10B9, 1)      # BUS_VIRTUAL，专用 PID（避 libwacom）
        setup.name = name.encode()[:79]
        fcntl.ioctl(self.fd, UI_DEV_SETUP, setup)
        for code, mx in ((ABS_X, w - 1), (ABS_Y, h - 1)):
            s = UinputAbsSetup()
            s.code = code
            s.absinfo = InputAbsinfo(0, 0, mx, 0, 0, 10)
            fcntl.ioctl(self.fd, UI_ABS_SETUP, s)
        fcntl.ioctl(self.fd, UI_DEV_CREATE)
        time.sleep(0.4)
        log("光标镜像已建立: %s（ABS_X 0..%d / ABS_Y 0..%d，非平板 ⇒ 驱动 Mutter 核心指针）"
            % (name, w - 1, h - 1))

    def move(self, x, y):
        """把核心指针挪到横屏像素 (x,y)。越界先夹住（uinput 对越界值会直接丢事件）。"""
        x = 0 if x < 0 else (self.w - 1 if x > self.w - 1 else int(x))
        y = 0 if y < 0 else (self.h - 1 if y > self.h - 1 else int(y))
        self.emit(EV_ABS, ABS_X, x)
        self.emit(EV_ABS, ABS_Y, y)
        self.emit(EV_SYN, 0, 0)

    def press(self, button=BTN_LEFT):
        """按下左键（配合 release()，把笔尖接触映射成核心指针点击）。

        ★★ 2026-10-21：为什么现在可以（也必须）发 BTN_LEFT
           2026-10-17 那条"绝不能发左键"的前提是**平板路径也在驱动核心指针**
           ⇒ 双源冲突会变幽灵点击。
           但 26.04 / Mutter 50 起，平板工具的 motion/tip **不再传到核心指针**：
           实测 event13 POINTER_MOTION_ABSOLUTE 709 条、POINTER_BUTTON **0 条**，
           而 event12 有 48 次 TABLET_TOOL_TIP。
           代价：只认核心指针的 GTK 应用（Nautilus、系统设置）完全点不中，
           而 GNOME Shell（顶栏）与 Mutter（窗口边框）直接吃 tablet 事件仍正常。
           ⇒ 现在只剩镜像一个源，由它把 tip 映射成标准左键才是正解。
        """
        self.emit(EV_KEY, button, 1)
        self.emit(EV_SYN, 0, 0)

    def release(self, button=BTN_LEFT):
        self.emit(EV_KEY, button, 0)
        self.emit(EV_SYN, 0, 0)

    def click(self, button=BTN_RIGHT):
        """在指针镜像上发一次「按下 + 抬起」。

        ★★ 为什么右键必须走这里（2026-10-17 定案）：
           笔本体 `event12` 是 **tablet**（`ID_INPUT_TABLET=1`，libwacom 已匹配），
           Mutter 把它的 tool button 当 tablet 按钮转发，但 **GNOME Shell / Clutter
           的桌面层不把 tablet 按钮当成鼠标点击** ⇒ 长按后日志"已发右键"、屏幕毫无反应。
           而本设备 `event13` 是 `ID_INPUT_MOUSE=1` 的普通指针，能力位里有 BTN_RIGHT
           ⇒ 在这里发右键就是标准鼠标右键，任何场景都认。

        ⚠ 只允许发 BTN_RIGHT（长按右键）。**绝不能发 BTN_LEFT** ——
          这个设备的定位是"镜像光标位置"，发左键会变成幽灵点击。
        """
        self.emit(EV_KEY, button, 1)
        self.emit(EV_SYN, 0, 0)
        self.emit(EV_KEY, button, 0)
        self.emit(EV_SYN, 0, 0)

    def emit(self, etype, code, value):
        # ★★ 2026-10-16 事故：上游算出浮点（`--x-smooth` 低通）⇒ InputEvent 只收 int ⇒
        #   TypeError ⇒ daemon 每次笔一动就崩，笔完全没反应（NRestarts 涨到 10）。
        #   ⇒ 在**唯一的 uinput 出口**处强制取整：以后任何上游浮点都不再能打死服务。
        os.write(self.fd, bytes(InputEvent(0, 0, etype, code, int(value))))

    def close(self):
        try:
            fcntl.ioctl(self.fd, UI_DEV_DESTROY)
        except OSError:
            pass
        os.close(self.fd)
        log("光标镜像已销毁")


# ─────────────────────── 从片帧 ───────────────────────
def rec_seq(rec):
    """取记录的 seq。★ 两种格式的偏移**不同**：旧格式在 0，上游 pen_rec 在 8
    （前面还有 u64 t_ns）。按错偏移读会把 t_ns 的低 32 位当 seq ⇒ 去重永远失效、
    同一帧被反复当成新帧。"""
    if len(rec) >= REC_LEN_OLD:
        return struct.unpack_from("<I", rec, SEQ_OFF_OLD)[0]
    if len(rec) >= REC_LEN_NEW:
        return struct.unpack_from("<I", rec, SEQ_OFF_NEW)[0]
    return None


def read_records(path, seconds=None, poll_ms=3.0):
    """读从片记录（`hs_frame` **或** 上游 `pen_rec`，按长度自动判），**按 seq 去重**。

    ⚠ 两个实机踩到的性质（2026-10-11）：
      1. 驱动的 `hs_dev_read()` **非阻塞**：环空返 `-EAGAIN`（`if (!hs->ring_count) return -EAGAIN;`）
         ⇒ 这里开 `O_NONBLOCK` + 轮询，不靠 read 阻塞。
      2. 读的是**环里最新那一帧**（`ring[(head-1) % HS_RING]`），**不是队列** ——
         实测裸轮询会以 ~61 kHz 反复拿到**同一帧**（`seq` 不变），纯空转烧 CPU。
         ⇒ 必须按 `seq` 去重；另加 poll 间隔把读速压到帧率量级。
    """
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    t_end = None if seconds is None else time.time() + seconds
    last_seq = None
    try:
        while t_end is None or time.time() < t_end:
            try:
                rec = os.read(fd, REC_LEN_MAX)
            except OSError as e:
                if e.errno == errno.EAGAIN:
                    time.sleep(poll_ms / 1000.0)
                    continue
                raise
            if len(rec) < REC_LEN_NEW:
                time.sleep(poll_ms / 1000.0)
                continue
            seq = rec_seq(rec)
            if seq == last_seq:            # 环里还是同一帧
                time.sleep(poll_ms / 1000.0)
                continue
            last_seq = seq
            yield rec
    finally:
        os.close(fd)


def parse_record(rec):
    """把**两种驱动**的记录归一化成同一组字段（按 `len(rec)` 判形态）。

    返回 dict（字段名沿用旧 `hs_frame` 的 API，另有新增 `fmt`/`d`）：
      seq                       —— 帧序号（两种格式都有）
      fmt                       —— 'old'（自制驱动 560 B）/ 'new'（上游 pen_rec 352 B）
      f1a,f1b,f2a,f2b            —— anchor 字（低字节 = row/col）
      peak1,peak2                —— TX1/TX2 峰值
                                   旧：头部给出；新：**由数组算**（上游不在记录里带）
      valid1                     —— 旧：头部字段；新：`flags & 1`（驱动注释即 "TX1 anchor valid"）
      valid2,ctr,nonzero,flen    —— 仅诊断；新格式没有 ⇒ 置 0 / 用户态算
      d                          —— 裸帧 336 B（两种格式完全一致）
      arr1,arr2,idx1,idx2,magic_ok
    """
    if len(rec) >= REC_LEN_OLD:               # ① sizeof(struct hs_frame) == 560
        (seq, flen, nonzero, ctr, f1a, f1b, f2a, f2b,
         peak1, peak2, valid1, valid2) = struct.unpack_from(HDR_FMT_OLD, rec, 0)
        d = rec[DATA_OFF_OLD:DATA_OFF_OLD + FRAME_LEN]
        fmt = "old"
    elif len(rec) >= REC_LEN_NEW:             # ② sizeof(struct pen_rec) == 352
        _t_ns, seq, flags = struct.unpack_from(HDR_FMT_NEW, rec, 0)
        d = rec[DATA_OFF_NEW:DATA_OFF_NEW + FRAME_LEN]
        valid1, valid2 = (flags & 1), 0
        # 新格式头里省掉的 anchor 直接从**裸帧**取（0x04/0x06/0xAA/0xAC 本就是帧内偏移）
        f1a = struct.unpack_from("<H", d, OFF_T1A)[0]
        f1b = struct.unpack_from("<H", d, OFF_T1B)[0]
        f2a = struct.unpack_from("<H", d, OFF_T2A)[0]
        f2b = struct.unpack_from("<H", d, OFF_T2B)[0]
        peak1 = peak2 = 0
        flen, ctr, nonzero = FRAME_LEN, 0, 0
        fmt = "new"
    else:                                     # 认不出来 ⇒ 当作无效记录
        return None
    a1 = struct.unpack_from("<%dh" % NLOT, d, OFF_T1)
    a2 = struct.unpack_from("<%dh" % NLOT, d, OFF_T2)
    if fmt == "new":                          # 新格式没有头部 peak ⇒ 从数组算（仅诊断用）
        peak1, peak2 = max(a1), max(a2)
        nonzero = sum(1 for v in a1 if v)
    i1 = max(range(NLOT), key=lambda i: a1[i])
    i2 = max(range(NLOT), key=lambda i: a2[i])
    return dict(seq=seq, fmt=fmt, flen=flen, nonzero=nonzero, ctr=ctr,
                f1a=f1a, f1b=f1b, f2a=f2a, f2b=f2b,
                peak1=peak1, peak2=peak2, valid1=valid1, valid2=valid2,
                d=d, arr1=a1, arr2=a2, idx1=i1, idx2=i2,
                magic_ok=(d[:2] == MAGIC))


# ── 压力通道（MCU 12d1:10b8 **接口 0** 的 hidraw，ReportID 0x55）─────────────
# 报格式（上游 gk3pend.c）：rep[0]=0x55, rep[1..2]=freq, rep[3..10]=4×u16 LE；
# 四个槽是"旧,旧,新,新"，取 slots[3] 最新；厂方 ASA 表 GetPressInMapOrder=2=incell
# ⇒ 默认把 4 个槽**逐帧回放**（而不是直接跳到最新）。
PRESS_MAX_AGE = 200.0        # ms：压力报每 10~17 ms 一次，但会有 50+ ms 的空档
PRESS_TAU_MS = 4.0           # 压力低通时间常数（上游默认 4）
# ★★ 2026-10-14 修正：0.150 太激进 ⇒「悬停时钟摆式 enter/leave」⇒ 光标重影 + 点击不灵
#   实测（用户报告「顶栏有时候点不到 + 光标有重影」，journalctl 取证）：
#     · 悬停（**未接触**）时从片帧本来就稀疏 —— 帧间隔实测 0.5~2 s；
#     · 150 ms 没帧就判「离开范围」⇒ 悬停期间以 **≈1 Hz 反复 enter/leave**
#       （实测 17 秒内 toggle 18 次）；
#     · 每次「离开」都 `publish_hover(False)`（扩展**恢复光标**）+ `park_cursor()`
#       （挪核心指针）⇒ 箭头 ~1 Hz 在「笔的上一个位置」闪现 = 用户看到的**重影**；
#       同时平板路径与镜像路径反复抢光标 ⇒ 点击落点不稳 = **顶栏偶尔点不到**。
#   ★ 上游 `gk3pend.c` 的 150 ms 是针对「接触时帧连续(220 Hz)」的场景；
#     我们的**悬停**帧率完全不同 ⇒ 必须加迟滞。改为 1.5 s（可用 --stale-ms 调）。
STALE_MS = 1.500             # 完全没有帧这么久 ⇒ 离开范围（★ 已从 0.150 提高，见上）
RELEASE_DELAY = 0.030        # 无有效帧这么久 ⇒ 准备释放

# ★★ 接触判据（2026-10-11 本机实测，见 artifacts/手写笔-从片坐标打通-2026-10-11.md）
#   从片帧 TX1 窗口中心格 peak 能干净区分"悬空/抬笔"与"接触"：
#       悬空 3~5 mm：peak 中位 332、p90 452、最大 519
#       抬笔后    ：peak 中位 702、p90 781、最大 884
#       接触（轻/中/重）：peak p10 2184~3092、中位 2381~3169
#   ⇒ 阈值 1500 两侧各有 ~600 的余量。
#   ⚠ 用途**只限"维持"**：压感仍是"开始接触"的唯一依据（上游语义），
#     这样**不会**因为悬停就凭空出笔；只解决"轻按时 MCU 压感报断流 ⇒ 笔画被切断"。
#   ★ 阈值本体定义在 `gk3pen_solver.py`（与 PEAK_MIN 同属帧级阈值），这里引用，避免两处各写一个数。
#
# ⚠⚠⚠ 2026-10-16 **上面这组数字与新的全量实测直接冲突，查清前 `peak` 不得当接触判据**（铁律十一）
#   反方证据（同一台机器，`/var/tmp/pen-jitter-20261016-214150.csv` 5666 帧全量重算）：
#       touch=0（悬停/未接触，5481 帧）：peak 中位 **2101**、p90 3747、**最大 4650**
#       touch_p=1（真接触，82 帧）      ：peak 中位 **1939**、p90 3478、最大 3645
#   ⇒ **悬停的 peak 比真接触还高**；`peak >= 1500` 的 3351 帧里只有 **69 帧（2.1%）** 是真接触。
#   ⇒ 「悬空 ≤519 / 接触 ≥2184」与「悬停中位 2101」**不可能同时为真**。
#   真因**未查清**（怀疑与 `raw_out_sel` 面板扫描配置、或"窗口中心格"的确切取样位置有关）。
#   ★ 影响面：`hold = touching and g[40] >= args.peak_contact` —— 若 peak 在悬停时也常过 1500，
#     这条会把"悬停"一直维持住（**幽灵笔**），与"peak 只用于维持"的设计意图相反。
#   ★ 处置：查清前 —— ① 不得用 peak 判"是否接触"；② `--peak-contact` 默认 1500 视为**待复核**。
#   详见技能 `gaokun3-touch-stylus` 铁律十一 + §四十一 ③。
PEAK_CONTACT = S.PEAK_CONTACT

# ── ★ 长按 = 右键（2026-10-11 用户要求：「像 Windows 下长按图标出现右击」）──────
# 为什么选长按而不是占用侧键：
#   · 实测**侧键只有双击会报信号**（单击/长按无信号）⇒ 若把侧键给右键，双击截图就得让位；
#   · 长按 = 右键是 Windows / 触摸屏的**标准语义**，且**侧键可以继续做双击截图**（两全）。
# 实现要点（不这么做会有个坑）：
#   长按触发时**必须先收掉左键**（发 BTN_TOUCH=0，让应用看到"一次 tap"），**再**发右键；
#   否则松手时那次左键点击会把刚弹出的右键菜单关掉。
#   触发之后到抬笔为止，**左键一直被压住不发**（suppress），避免再产生一次点击。
LONGPRESS_MS = 800.0         # 按住这么久且没怎么动 ⇒ 右键（0 = 关掉这个功能）
LONGPRESS_TOL = 30           # 横屏像素；位移超过它就当"在划线"，不算长按
# ★ 环形进度的"延迟出现"：起手**先不显示**，按住这么久之后才出环，并在剩下的时间里画满。
#   为什么：不延迟的话「点一下」也会立刻冒出一个环（用户实测反馈）。
#   对齐 Windows 的 press-and-hold：先按约 0.5 s，环才出现并快速闭合。
RING_DELAY_MS = 500.0        # 0 = 一按下就显示
RING_HOLD_DONE = 0.25        # 触发右键后，环画满并保留这么久再收起来（秒）
# ★★ 压感门限：只有【重压】才武装长按（环与右键都跟着它）——
#   目的：写字时的自然停顿 / 慢笔画不会被误判成长按。
#   阈值依据实测分布（用户 7 天日志，press_out 0..4095，非零样本 323 个）：
#     中位 1034 · p75 1895 · p85 2243 · p90 2538 · p95 2951 · max 4084
#   ⇒ 日常书写大多低于 2000；取 2200 时只有约 15% 的书写样本能达到 ⇒ 明显是故意重压。
LONGPRESS_MIN_PRESSURE = 2200.0   # 0 = 不要求压感（回到纯时间判定）

HIDIOCGRDESCSIZE = (2 << 30) | (4 << 16) | (ord('H') << 8) | 0x01
HIDIOCGRDESC = (2 << 30) | (4100 << 16) | (ord('H') << 8) | 0x02   # sizeof(struct hidraw_report_descriptor)


class HidrawReportDescriptor(ctypes.Structure):
    _fields_ = [("size", ctypes.c_uint32), ("value", ctypes.c_uint8 * 4096)]


def open_pressure_hidraw(verbose=False):
    """按**报告描述符**找压力通道（不是按节点号）：UsagePage 0xff0a + ReportID 0x55。

    上游做法：扫 /dev/hidraw0..15，取描述符，若开头是 `06 0a ff` 且前 16 字节内有 `85 55`
    就是这个节点。这样设备重编号也不会找错。
    """
    for i in range(16):
        path = "/dev/hidraw%d" % i
        try:
            fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
        except OSError:
            continue
        try:
            sz = ctypes.c_int(0)
            fcntl.ioctl(fd, HIDIOCGRDESCSIZE, sz)
            if not (9 <= sz.value <= 4096):
                raise OSError("bad desc size")
            rd = HidrawReportDescriptor()
            rd.size = sz.value
            fcntl.ioctl(fd, HIDIOCGRDESC, rd)          # noqa: F841 (就地填充)
            v = bytes(rd.value[:sz.value])
            if len(v) >= 3 and v[0] == 0x06 and v[1] == 0x0A and v[2] == 0xFF:
                for k in range(min(sz.value - 1, 16)):
                    if v[k] == 0x85 and v[k + 1] == 0x55:
                        if verbose:
                            log("压力通道: %s（UsagePage 0xff0a / ReportID 0x55）" % path)
                        return fd, path
        except OSError:
            pass
        os.close(fd)
    return -1, None


def run_daemon(args, pen, mirror=None):
    """★ 真流水线：从片帧（坐标） + MCU hidraw 0x55（压力） → uinput 触控笔。

    照上游 gk3pend.c：
      · 帧有效性：magic 两字节 + 锚点非 0xff/0xff + 窗口中心格 peak ≥ 200
      · 坐标：solve_local → 锚点绝对坐标 → BOE 列间距 → 翻两轴 → 横屏像素
      · 压力：4 槽 incell 回放 → map_pressure → 时间常数低通（下笔/抬笔不低通，保持瞬时）
      · 接触判据 = **有压力**（不是"在范围内"）
    """
    if S is None:
        print("!! 找不到 gk3pen_solver.py（应与本脚本同目录）")
        return 1
    col_map = S.panel_is_boe()
    print("面板 %s ⇒ 列间距补偿 %s" % ("BOE" if col_map else "非 BOE/未知", "开" if col_map else "关"))

    hid_fd, hid_path = (-1, None)
    if not args.no_pressure:
        hid_fd, hid_path = open_pressure_hidraw(args.verbose)
        if hid_fd < 0:
            print("⚠ 没找到压力通道（接口 0 的 hidraw）⇒ 压力恒为 0，将退化为『在范围内即接触』")
    else:
        print("--no-pressure：跳过压力通道")

    csv = open(args.coords_csv, "w", buffering=1) if args.coords_csv else None
    if csv:
        csv.write("t,seq,ar,ac,a2r,a2c,peak,lc,lr,qc,col,row,x,y,ex,ey,sig1,sig2,tilt_c,tilt_r,"
                  "raw,mapped,press_out,touch_p,hold,touch\n")

    tilt_on = not args.no_tilt
    ts = S.TiltState()
    tilt_x = tilt_y = 0

    # ★ 笔的"在附近/接触"状态发布给桌面（GNOME Shell 扩展据此隐藏光标箭头）
    #   daemon 是唯一真正知道笔状态的人（`in_range` 已有）⇒ 写个文件最省事、最不容易坏。
    #   目录由 systemd 的 RuntimeDirectory=pen-daemon 提供（/run/pen-daemon，重启即清空）。
    hover_path = args.hover_state_file
    hover_fd = -1
    if hover_path:
        try:
            os.makedirs(os.path.dirname(hover_path), exist_ok=True)
            hover_fd = os.open(hover_path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
        except OSError as e:
            log("⚠ 无法写光标状态文件 %s（%s）⇒ 悬停隐藏光标会不工作" % (hover_path, e))
            hover_fd = -1

    def publish_hover(on):
        if hover_fd >= 0:
            try:
                os.lseek(hover_fd, 0, os.SEEK_SET)
                os.write(hover_fd, b"1\n" if on else b"0\n")
                os.ftruncate(hover_fd, 2)
            except OSError:
                pass

    publish_hover(False)          # 先写一个确定的初值（别留 0 字节，让扩展读到明确状态）

    # ── 长按进度（供 GNOME Shell 扩展画「环形进度」，对齐 Windows 长按的视觉反馈）──
    #   格式：`x y 进度\n`，x/y = **横屏像素**（0..SCREEN_W/H，见 gk3pen_solver），
    #   进度 ∈ [0,1]；进度 < 0 表示"当前没有长按"。空文件/读不到 ⇒ 扩展当作未按。
    press_path = args.press_state_file
    press_fd = -1
    if press_path:
        try:
            os.makedirs(os.path.dirname(press_path), exist_ok=True)
            press_fd = os.open(press_path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
        except OSError as e:
            log("⚠ 无法写长按进度文件 %s（%s）⇒ 环形进度不会显示" % (press_path, e))
            press_fd = -1

    def publish_press(x, y, prog):
        if press_fd < 0:
            return
        try:
            s = ("%d %d %.3f\n" % (int(x), int(y), prog)).encode()
            os.lseek(press_fd, 0, os.SEEK_SET)
            os.write(press_fd, s)
            os.ftruncate(press_fd, len(s))
        except OSError:
            pass

    publish_press(0, 0, -1.0)     # 初值：未按


    # ★★★ 2026-10-17 修：**必须是 O_RDWR**。原先写的是 O_RDONLY，而下面压力段有
    #     `os.write(pen_fd, b"u")`（hint：告诉驱动"笔在用"、退出 idle 省电）——
    #     往只读 fd 写只会拿到 EBADF，而它又被 `except OSError: pass` 静默吞掉
    #     ⇒ **hint 从引入那天（de9caac）起一次都没成功过**，快唤醒通道等于没接线。
    #     实测证据：手动 `printf u > /dev/gk3_pen` 能让 `hints 2→3→4`（计数器是活的），
    #     而 300 s 高强度用笔期间 `hints` 纹丝不动、同时 0x55 压感报照常刷出 ⇒ 二者不可同真。
    pen_fd = os.open(args.frame_dev, os.O_RDWR | os.O_NONBLOCK)
    slots = [0, 0, 0, 0]
    play = 4
    pressure = 0
    press_t = 0.0
    press_out = 0.0
    out_t = 0.0
    hint_t = 0.0
    hint_ok = 0                    # 成功写出的 hint 次数（用于自检"快唤醒通道到底通没通"）
    hint_err_logged = False        # 失败只报首次，避免刷爆 journal
    hid_retry_at = 0.0
    touching = False
    # 长按=右键 的状态
    lp_t0 = None
    lp_x = lp_y = 0
    lp_fired = False
    lp_fired_at = 0.0             # 触发时刻（环画满保留 RING_HOLD_DONE 后收起）
    suppress_touch = False
    in_range = False
    last_valid = 0.0
    last_xy = None                # 最近一次有效帧的横屏屏幕坐标（抬笔时用来"停光标"）
    # ★★★ 2026-10-17 定案：`qc < QC_MIN` **不是"帧退化"，只是"笔在格内的位置"** ——
    #   `qc = (峰列左邻 + 峰列右邻) / 峰列` 是格内位置的确定性 U 形函数：格心 0.293、
    #   格边 1.008，**连续、无断层**。所以 QC_MIN=0.40 把 **46.1–46.8%** 的帧判成"列向退化"，
    #   而那些帧恰恰是**格心帧（位置最干净）**：按 qc 分 6 桶量噪声 σ = 0.38/0.40/0.41/
    #   0.40/0.38/0.42 px —— **完全相同**（对照无任何后处理的 y 轴 σ=0.42–0.49 px）。
    #   ⇒ 原"冻结/外推 x"分支（`--x-run-max`）**治的是一个不存在的病**，已整块删除。
    #   ❌ 且它自己制造了病：停滞段起点 x 位移聚在 40–45 / 84–87px（1 格 = 2560/60 =
    #      42.67px）⇒ 台阶由**跨列格边界**触发，与"退化"无关；书写期输出 vs 真值
    #      >8px 占 19.4%（p90 13 / max 22px），落笔帧偏 p90 13px ⇒ 直接点歪。
    #   ✅ 删除后离线回放：RMS 6.8 → 1.9px，帧步 max 18 → 8px，≥25 帧停滞段 30 → 0。
    #   原始信号本身干净：σ 0.39px、帧步 p90 1px、跨格帧 |Δx| ≤ 2px、去趋势 ACF 只有
    #   lag 1–5 单调衰减、lag≥10 无任何 >0.78 的峰 ⇒ **抖动只有冻结这一个来源**。
    #   ⚠ 不要因为"帧间 |Δx| 变大了"再把冻结加回来 —— `pen-x-regression.py` 就是防这个的。
    x_out_prev = None             # 上一次**发出**的 x（供 --x-smooth 低通）
    last_press_pub = 0.0          # 上次写"长按进度"的时间（节流用）
    PRESS_PUB_MS = 0.025          # 进度发布周期（40 Hz，够画圆环又不刷爆）
    last_seq = None

    def park_cursor():
        """★ 只在**抬笔那一刻**把核心指针（Mutter `seat_impl->pointer_x/y`）摆到笔最后的位置。

        ★★ 为什么**不能**连续发（2026-10-13 用户实测"悬停光标抖动明显、点击不灵"）：
           设备里有**两个源**在驱动光标精灵 —— 平板路径（`TABLET_TOOL_AXIS`）与镜像路径
           （`POINTER_MOTION_ABSOLUTE`）。两者的坐标映射并不逐像素重合 ⇒ 精灵在两点之间
           以帧率反复跳 ⇒ 肉眼就是抖动，且 `GtkGestureClick` 的"位移超阈值判成拖拽"被触发
           ⇒ 点击不灵。
           而**回跳只需要一次修正**：Mutter 是在"光标被重新同步到核心指针"那一刻打回去的，
           所以只要在抬笔时把核心指针摆到笔的最后位置，之后再怎么同步都落在笔的位置。
        """
        if mirror is not None and last_xy is not None:
            mirror.move(*last_xy)
            if args.verbose:
                log("抬笔：核心指针停在 (%d,%d)" % last_xy)
    n = 0
    reports = 0
    t0 = time.time()
    last_log = t0
    try:
        while True:
            now = time.time()
            # ── 1) 压力：把 hidraw 里的报全排干（非阻塞）──
            #    上游 gk3pend 同样有 hid_retry_at 这套；常驻服务必须能自愈
            if hid_fd < 0 and not args.no_pressure and now >= hid_retry_at:
                hid_fd, hid_path = open_pressure_hidraw(args.verbose)
                if hid_fd >= 0:
                    log("压力通道已连上: %s" % hid_path)
                else:
                    hid_retry_at = now + 2.0
            # ── 1) 压力：把 hidraw 里的报全排干（非阻塞）──
            if hid_fd >= 0:
                while True:
                    try:
                        rep = os.read(hid_fd, 64)
                    except OSError as e:
                        if e.errno in (errno.EAGAIN, errno.EWOULDBLOCK):
                            break
                        if args.verbose:
                            log("压力通道读失败 %s ⇒ 关闭并稍后重连" % e)
                        os.close(hid_fd)
                        hid_fd = -1
                        hid_retry_at = now + 2.0
                        break
                    if len(rep) >= 11 and rep[0] == 0x55:
                        for k in range(4):
                            slots[k] = rep[3 + 2 * k] | (rep[4 + 2 * k] << 8)
                        pressure = slots[3]
                        play = 0
                        press_t = now
                        reports += 1
                        if now - hint_t >= 0.2:      # 告诉驱动"笔在用"，避开 idle 轮询
                            hint_t = now
                            try:
                                os.write(pen_fd, b"u")
                                hint_ok += 1
                            except OSError as e:
                                # ★★ 别再无声道地吞掉：这个 `pass` 让"往只读 fd 写、
                                #    永远 EBADF"这件事藏了好几天（见 pen_fd 处的注释）。
                                #    只报**首次**，避免 5 Hz 刷爆 journal。
                                if hint_ok == 0 and not hint_err_logged:
                                    hint_err_logged = True
                                    log("!! hint 写 /dev/gk3_pen 失败（%s）⇒ 快唤醒通道失效；"
                                        "检查 pen_fd 是否以 O_RDWR 打开" % e)
            # ── 2) 从片帧（按 seq 去重；驱动给的是环里最新那帧）──
            frame = None
            try:
                rec = os.read(pen_fd, REC_LEN_MAX)
            except OSError as e:
                if e.errno not in (errno.EAGAIN, errno.EWOULDBLOCK):
                    # /dev/gk3_pen 没了（驱动重载/设备消失）⇒ 干净退出，让 systemd 重启我们
                    log("!! /dev/gk3_pen 读失败（%s）⇒ 退出，等服务重启" % e)
                    return 1
                rec = b""
            if len(rec) >= REC_LEN_NEW:
                seq = rec_seq(rec)
                if seq != last_seq:
                    last_seq = seq
                    frame = rec

            if frame is not None:
                # ★ 归一化：自制驱动(560 B hs_frame)/上游驱动(352 B pen_rec) 都走这一条路
                rf = parse_record(frame)
                seq = rf["seq"]
                d = rf["d"]
                ar, ac = rf["f1a"] & 0xFF, rf["f1b"] & 0xFF
                a2r, a2c = rf["f2a"] & 0xFF, rf["f2b"] & 0xFF
                g = list(rf["arr1"])
                g2 = list(rf["arr2"])
                valid = (d[0] == 0x5A and d[1] == 0xA5 and not (ar == 0xFF and ac == 0xFF)
                         and g[40] >= S.PEAK_MIN)
                if valid:
                    lc, lr, qc, _qr = S.solve_local_q(g, ar, ac)
                    col = lc + (ac - 4) * S.UNIT
                    row = lr + (ar - 4) * S.UNIT
                    n += 1
                    last_valid = now

                    # 压力：incell 回放 → 曲线 → 低通（下笔/抬笔不低通）
                    # ★ 双源接触判定：压感负责"开始/结束"，peak 负责"维持"
                    #   touch_p: 压感新鲜 ⇒ 明确的接触
                    #   hold   : 已在接触中，且本帧 peak 仍在接触量级 ⇒ 不释放（治轻按压感断流）
                    touch_p = pressure > 0 and (now - press_t) * 1000.0 <= args.press_age
                    hold = touching and g[40] >= args.peak_contact
                    touch = touch_p or hold
                    raw = slots[play] if (play < 4 and slots[0]) else pressure
                    if play < 4:
                        play += 1
                    fresh = touch_p and (raw > 0 or pressure > 0)
                    mapped = 0.0
                    if not touch:
                        press_out = 0.0
                    elif fresh:
                        mapped = S.map_pressure(raw if raw > 0 else pressure)
                        if touching and press_out > 0 and args.press_tau > 0:
                            dt = max((now - out_t) * 1000.0, 1.0)
                            press_out += (mapped - press_out) * dt / (args.press_tau + dt)
                        else:
                            press_out = float(mapped)      # 下笔瞬间不低通
                    else:
                        # ★ 维持中且没有新的压力值 ⇒ **保持上一次的有效压力**，
                        #   不要回落到 0（否则轻笔画压力会掉到 0，只靠"最小上报 1"撑着）
                        pass
                    out_t = now

                    # ★ Stage C 倾斜：TX1 笔尖 vs TX2 环形电极的偏移 → asin 逐轴
                    #   顺序照上游：**倾斜回推在 pitch 补偿与翻转之前**（都在未翻转的传感器坐标上算）
                    sig1 = sig2 = 0
                    tilt_c = tilt_r = 0
                    if tilt_on:
                        if not (a2r == 0xFF and a2c == 0xFF):
                            t2 = S.solve_tx2(g2, a2r, a2c)
                            if t2:
                                sig1 = S.sum3x3(g, 4, 4)
                                sig2 = t2[2]
                                ts.update(col, row, t2[0], t2[1], sig1, sig2, touch)
                        col, row = ts.revise(touch, col, row)
                        tilt_c, tilt_r = ts.out_c, ts.out_r
                        # ★ 轴向/符号见 tilt_axes() 的注释（本机实测定案，不是照抄上游）
                        tilt_x, tilt_y = tilt_axes(tilt_c, tilt_r,
                                                   args.tilt_sx, args.tilt_sy)

                    x, y = S.to_screen(col, row, col_map=col_map)
                    # ★ 可选轻低通（`--x-smooth`，默认 0.5）。★ IIR 状态保持**浮点**，
                    #   只在发出前取整。实测代价：0.5 ⇒ 一帧滞后(4.4ms)/偏差 p90 2px；
                    #   0.8 ⇒ 4 帧(17.5ms)/p90 8px。它治的是**悬停光标**抖动，与划线无关。
                    if args.x_smooth > 0.0 and x_out_prev is not None:
                        x_out_prev = (1.0 - args.x_smooth) * x + args.x_smooth * x_out_prev
                        x = x_out_prev
                    else:
                        x_out_prev = float(x)
                    # ★★ 必须取整：uinput 的 InputEvent 只接受 int —— 2026-10-16 事故：
                    #    低通把 x 变浮点 ⇒ 第一次笔事件就 TypeError ⇒ daemon 崩（NRestarts=10，
                    #    笔一动不动）。同一个坑已在 NoneType 那次吃过一次。
                    x = int(round(x))
                    # ★ 最后一步换框：内部一律横屏像素，emit 前按 `--frame` 转成面板原生框
                    ex, ey = frame_emit(x, y, args.frame, args.flip_x, args.flip_y)
                    # ⚠ 光标镜像**不在这里发** —— 连续发会让两个源（平板路径 / 镜像路径）
                    #   抢光标精灵 ⇒ 悬停抖动 + 点击不灵。只在抬笔那一刻发一次，见 park_cursor。
                    # ★ 长按 = 右键（Windows/触摸语义）。详见顶部 LONGPRESS_MS 的说明。
                    if args.longpress_ms <= 0:
                        lp_t0 = None
                    elif not touch:
                        lp_t0 = None                       # 抬笔/离开 ⇒ 重置
                        lp_fired = False
                        suppress_touch = False
                    elif lp_t0 is None and press_out >= args.longpress_min_pressure:
                        lp_t0, lp_x, lp_y = now, x, y       # 本次按下的起点
                        lp_fired = False
                        # ★ 校准用：这行给出你按下时的实际压感，调阈值时照它改最准
                        log("长按已武装（压感 %d ≥ %d）"
                            % (int(press_out), int(args.longpress_min_pressure)))
                    # ★★ 必须显式判 lp_t0 is not None：上面"武装"那支带了压感门限，
                    #   压感不足时它会失败 ⇒ 控制流会落到这里 ⇒ 若直接算 now-lp_t0
                    #   就是 TypeError（2026-10-14 真的崩了：笔整个消失，systemd 重启 10 次）。
                    elif (lp_t0 is not None and not lp_fired
                          and (now - lp_t0) * 1000.0 >= args.longpress_ms
                          and abs(x - lp_x) <= args.longpress_tol
                          and abs(y - lp_y) <= args.longpress_tol):
                        lp_fired = True
                        lp_fired_at = now
                        # ① 先收掉左键（应用看到的是一次 tap，不是"按着不放"）
                        pen.emit(EV_ABS, ABS_PRESSURE, 0)
                        pen.emit(EV_KEY, BTN_TOUCH, 0)
                        pen.syn()
                        touching = False
                        suppress_touch = True              # 抬笔前不再发左键
                        # ★★ 2026-10-21：必须同时释放**镜像的左键**。
                        #   长按触发时上面直接把 touching 置 False（绕过了
                        #   `if etouch != touching` 那段），所以镜像的 BTN_LEFT 会一直
                        #   按着 ⇒ 右键菜单被"左键 grab 还在"挡掉，表现为右击无弹窗。
                        if mirror is not None:
                            mirror.release(BTN_LEFT)
                        # ② 再发右键
                        # ★★ 按钮码只能是 BTN_STYLUS2（默认 stylus2）。本设备被
                        #   udev/libwacom 判成 tablet（ID_INPUT_TABLET=1）⇒ libinput
                        #   只处理 BTN_STYLUS/BTN_STYLUS2，其余按钮"ignore on purpose"
                        #   ⇒ 发 BTN_RIGHT 一定白费（能力位也没有它），2026-10-17 定案。
                        btn = {"stylus2": BTN_STYLUS2, "stylus": BTN_STYLUS,
                               "right": BTN_RIGHT}[args.right_button]
                        # ★★★ 但 tablet 按钮在 **GNOME Shell/Clutter 桌面**层不产生点击
                        #   （日志显示已发、屏幕毫无反应，2026-10-17 实测）。
                        #   ⇒ 有 cursor 镜像（event13，ID_INPUT_MOUSE=1、能力位含 BTN_RIGHT）
                        #     时走它发**标准鼠标右键**：先把核心指针摆到笔尖处，
                        #     菜单才会在笔的位置弹出。没有镜像才回落到 tablet 按钮。
                        #
                        # ★★ 坐标必须用**横屏**的 (x, y)，**不能**用 (ex, ey)！（2026-10-17 修）
                        #   `ex/ey` 是 `frame_emit` 为 **tablet 设备**抵消 Mutter 轴转置后的值，
                        #   量纲是竖屏框（ABS_X 0..1599 / ABS_Y 0..2559）；
                        #   而 cursor 镜像声明的是**横屏框**（0..2559 / 0..1599）
                        #   ⇒ 传 (ex, ey) 时 ey 一路超出 1599 被 uinput 夹到最大值，
                        #     实测 libinput 侧 `POINTER_MOTION_ABSOLUTE` 的 **Y 恒为 99.94%**
                        #     （X 正常变化）⇒ 光标永远贴在屏幕最底边，菜单也就不在笔尖弹出。
                        if mirror is not None:
                            mirror.move(x, y)
                            mirror.click(BTN_RIGHT)
                        else:
                            pen.emit(EV_KEY, btn, 1)
                            pen.syn()
                            pen.emit(EV_KEY, btn, 0)
                            pen.syn()
                        log("长按 %.0f ms ⇒ 右键（走 %s；按钮码 %s）"
                            % (args.longpress_ms,
                               "cursor 镜像鼠标" if mirror is not None else "tablet 按钮",
                               args.right_button))

                    # ★ 长按进度 → 状态文件（扩展据此画环形进度；Windows 长按也有这个环）
                    #   ★★ 延迟出现：按住 < RING_DELAY_MS 时**写 -1（不显示）** ——
                    #      否则"点一下"也会立刻冒出一个环。过了延迟才开始画，并在
                    #      (LONGPRESS_MS - RING_DELAY_MS) 内画满。
                    #   状态优先级：已触发(1.0，保留 RING_HOLD_DONE 后收起) > 正按着(0..1) > 未按(-1)
                    if press_fd >= 0 and now - last_press_pub >= PRESS_PUB_MS:
                        held_ms = (now - lp_t0) * 1000.0 if lp_t0 is not None else -1.0
                        if lp_fired:
                            publish_press(x, y,
                                          1.0 if (now - lp_fired_at) < RING_HOLD_DONE else -1.0)
                        elif (held_ms >= RING_DELAY_MS
                              and args.longpress_ms > RING_DELAY_MS
                              # ★ 位移容差**也要查**：否则"压得很重但在划"时
                              #   环照样出现（仿真抓到的 bug）
                              and abs(x - lp_x) <= args.longpress_tol
                              and abs(y - lp_y) <= args.longpress_tol):
                            publish_press(x, y,
                                          min(1.0, (held_ms - RING_DELAY_MS)
                                              / (args.longpress_ms - RING_DELAY_MS)))
                        else:
                            publish_press(0, 0, -1.0)
                        last_press_pub = now

                    # ★ etouch 必须在长按块**之后**算：长按会改 suppress_touch/touching，
                    #   早算会让「触发那一帧」仍按有接触发射　⇒ 又把左键按下、把刚弹出的菜单点掉。
                    etouch = touch and not suppress_touch

                    # ★★ 2026-10-13 光标镜像：这里**只记下笔的最新位置**，不立刻发。
                    #   镜像的包只在"抬笔那一刻"发一次（见 park_cursor 的文档字符串）——
                    #   连续发会让"平板路径"与"镜像路径"两个源抢光标精灵，映射不完全重合
                    #   就变成**悬停抖动 + 点击不灵**（用户实测）。
                    last_xy = (x, y)

                    # ★ 工具位**每帧都带**（真实平板驱动如 wacom 就是这么做的）。
                    #   只在进范围时发一次的话，libinput 会认为笔在每一帧都在
                    #   「离开→再进入」之间抖（2026-10-13 实测：proximity in/out 各 ~7 Hz），
                    #   而 proximity-out 正是把光标同步回核心指针的触发点之一。
                    pen.emit(EV_KEY, BTN_TOOL_PEN, 1)
                    if not in_range:
                        in_range = True
                        publish_hover(True)
                        # ★ 总是打（低频）：这是「光标重影」唯一的可见指标
                        log("悬停开始（进入范围）")
                    pen.emit(EV_ABS, ABS_X, ex)
                    pen.emit(EV_ABS, ABS_Y, ey)
                    if tilt_on:
                        pen.emit(EV_ABS, ABS_TILT_X, int(tilt_x))
                        pen.emit(EV_ABS, ABS_TILT_Y, int(tilt_y))
                    pen.emit(EV_ABS, ABS_PRESSURE, (int(press_out) or 1) if etouch else 0)
                    if etouch != touching:
                        pen.emit(EV_KEY, BTN_TOUCH, 1 if etouch else 0)
                        # ★ 同步镜像左键：Mutter 50 起 tablet tip 不再转 pointer click，
                        #   GTK 应用只认核心指针 ⇒ 必须在这里配对发 BTN_LEFT。
                        if mirror is not None:
                            if etouch:
                                mirror.press(BTN_LEFT)
                            else:
                                mirror.release(BTN_LEFT)
                        touching = etouch
                        if args.verbose:
                            log("%s at (%d,%d) raw=%d mapped=%d out=%d tilt=%d/%d"
                                % ("down" if touch else "up", x, y, raw,
                                   S.map_pressure(raw if raw > 0 else pressure) if raw else 0,
                                   int(press_out), int(tilt_x), int(tilt_y)))
                    pen.syn()
                    # ★★ 2026-10-20：镜像**每帧**跟随笔尖（仅 --mirror-follow 时开）
                    #
                    # 起因（实测）：26.04 / Mutter 50 下，**平板工具的 motion 不再传到
                    #   核心指针**。一次完整书写 session 的 X 侧 XI2 统计：
                    #       device 11 (xwayland-tablet stylus) Motion 1436
                    #       device  6 (xwayland-pointer / 核心指针) Motion **2**
                    #   ⇒ 只认核心指针的应用（Electron/Chromium，如 WorkBuddy 客户端）
                    #     完全无法被笔操作：不跟手、点不动。
                    #
                    # ⚠ 历史包袱要看清：2026-10-13 曾因「两个源同时驱动精灵」导致
                    #   抖动 + 点击不灵（CursorMirror 文档里记着）。但那时平板路径
                    #   **也**在动核心指针 ⇒ 现在是**双源冲突**；如今它不动了
                    #   ⇒ 只剩镜像这一个源，连续驱动才是正解。
                    #   ⇒ 默认仍关，用 --mirror-follow 显式开，便于 A/B 与回退。
                    #
                    # ⚠ 坐标必须用**横屏** (x,y)，不能用 (ex,ey)：见上面长按右键处
                    #   那段注释（ex/ey 是竖屏框量纲，会把 Y 顶到最底边）。
                    if mirror is not None and args.mirror_follow:
                        mirror.move(x, y)
                    if csv:
                        # ⚠ 表头有 26 列 ⇒ 这里必须 26 个占位符（原先表头写了 ex,ey 却没写，
                        #   是既有的表头/行错位；2026-10-16 一并补齐 + 新增 qc）。
                        csv.write("%.4f,%d,%d,%d,%d,%d,%d,%d,%d,%.3f,%d,%d,%d,%d,%d,%d,"
                                  "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n"
                                  % (now - t0, seq, ar, ac, a2r, a2c, g[40], lc, lr, qc,
                                     col, row, x, y, ex, ey,
                                     sig1, sig2, int(tilt_c), int(tilt_r),
                                     raw, mapped if fresh else 0, int(press_out),
                                     1 if touch_p else 0, 1 if hold else 0, 1 if touch else 0))
                    if now - last_log >= 1.0:
                        log("帧 %d | 锚点 %d/%d peak %d → 横屏(%d,%d) 发出(%d,%d) | 压力 %d | 倾斜 %d/%d | 接触 %d"
                            % (n, ar, ac, g[40], x, y, ex, ey, int(press_out),
                               int(tilt_x), int(tilt_y), 1 if touch else 0))
                        last_log = now
                else:
                    if in_range and now - last_valid > args.stale_ms / 1000.0:
                        in_range = False
                        x_out_prev = None   # ★ 离开范围 ⇒ 丢掉低通历史（防下次进场从旧值缓动）
                        publish_hover(False)
                        park_cursor()          # ★ 抬笔：把核心指针停在笔的最后位置（防回跳）
                        publish_press(0, 0, -1.0)   # 抬笔 ⇒ 环形进度消失
                        pen.emit(EV_ABS, ABS_PRESSURE, 0)
                        pen.emit(EV_KEY, BTN_TOUCH, 0)
                        pen.emit(EV_KEY, BTN_TOOL_PEN, 0)
                        pen.syn()
                        touching = False
                        log("悬停结束（离开范围）")
            elif in_range and now - last_valid > args.stale_ms / 1000.0:
                in_range = False
                x_out_prev = None   # ★ 同上（无帧超时路径）
                publish_hover(False)
                park_cursor()                  # ★ 同上（无帧超时路径）
                publish_press(0, 0, -1.0)
                pen.emit(EV_ABS, ABS_PRESSURE, 0)
                pen.emit(EV_KEY, BTN_TOUCH, 0)
                pen.emit(EV_KEY, BTN_TOOL_PEN, 0)
                pen.syn()
                touching = False
                log("悬停结束（离开范围·超时）")

            if args.seconds and now - t0 >= args.seconds:
                break
            time.sleep(0.003)
    except KeyboardInterrupt:
        pass
    finally:
        publish_hover(False)          # 退出时把状态交还（扩展会恢复光标）
        publish_press(0, 0, -1.0)     # 退出时清掉环形进度
        if press_fd >= 0:
            os.close(press_fd)
        if hover_fd >= 0:
            os.close(hover_fd)
        os.close(pen_fd)
        if hid_fd >= 0:
            os.close(hid_fd)
        if csv:
            csv.close()
    log("结束：有效帧 %d，压力报 %d，hint 成功 %d" % (n, reports, hint_ok))
    return 0


def selftest(pen, hold=False, secs=12.0, mirror=None):
    import math
    log("自检：发合成轨迹 %.0fs（Lissajous + 压力斜坡）" % secs)
    t0 = time.time()
    pen.emit(EV_KEY, BTN_TOOL_PEN, 1)
    pen.syn()
    down = False
    while time.time() - t0 < secs:
        t = time.time() - t0
        sx = int(2559 * (1 + math.sin(t * 1.3)) / 2)
        sy = int(1599 * (1 + math.cos(t * 2.1)) / 2)
        pen.emit(EV_ABS, ABS_X, sx)
        pen.emit(EV_ABS, ABS_Y, sy)
        if mirror is not None:
            mirror.move(sx, sy)
        pen.emit(EV_ABS, ABS_DISTANCE, 0 if (down or hold) else 40)
        pen.emit(EV_ABS, ABS_PRESSURE, 1500 + int(1200 * abs(math.sin(t * 0.7))) if (down or hold) else 0)
        pen.emit(EV_ABS, ABS_TILT_X, int(30 * math.sin(t)))
        pen.emit(EV_ABS, ABS_TILT_Y, int(30 * math.cos(t)))
        if not down and not hold and t > 2.0:
            down = True
            pen.emit(EV_KEY, BTN_TOUCH, 1)
        pen.syn()
        time.sleep(0.008)
    pen.emit(EV_KEY, BTN_TOUCH, 0)
    pen.emit(EV_KEY, BTN_TOOL_PEN, 0)
    pen.syn()
    log("自检结束")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--frame-dev", default=FRAME_DEV)
    ap.add_argument("--no-uinput", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--hold", action="store_true")
    ap.add_argument("--draw", action="store_true",
                    help="★ 真流水线：从片帧 → 屏幕坐标 + MCU 0x55 → 压力 → uinput")
    ap.add_argument("--no-pressure", action="store_true", help="不接压力通道（A/B 对照用）")
    ap.add_argument("--press-tau", type=float, default=PRESS_TAU_MS, help="压力低通时间常数 ms")
    ap.add_argument("--press-age", type=float, default=PRESS_MAX_AGE, help="压感新鲜窗口 ms")
    ap.add_argument("--stale-ms", type=float, default=int(STALE_MS * 1000),
                    help="★ 无帧这么久(ms)才判「离开范围」。太小会让光标在悬停时反复闪现"
                         "（重影）——悬停帧率本就稀疏，实测阈值 150ms 会以 ~1Hz 抖动。"
                         "默认 1200；调大=更稳但抬笔后光标回得更慢")
    ap.add_argument("--peak-contact", type=int, default=PEAK_CONTACT,
                    help="peak 接触阈值（仅用于『维持』接触，不会凭空开始出笔）")
    ap.add_argument("--no-tilt", action="store_true", help="关掉倾斜（Stage C）与其笔尖回推")
    ap.add_argument("--tilt-sx", type=int, default=1, help="倾斜 X 轴符号（实测反向时用 -1）")
    ap.add_argument("--tilt-sy", type=int, default=1, help="倾斜 Y 轴符号（实测反向时用 -1）")
    ap.add_argument("--usb-id", metavar="VVVV:PPPP", default=None,
                    help="★ 让 uinput 笔带 USB 身份（报 BUS_USB + 该 VID:PID）："
                         "libwacom 的 DeviceMatch 只认 bus:vid:pid，BUS_VIRTUAL 永远匹配不上，"
                         "GNOME 就建不出平板工具、应用收不到压感/倾角。"
                         "本机用 --usb-id 12d1:10b8 以匹配 "
                         "/etc/libwacom/gaokun3-m-pencil.tablet（usb:12d1:10b8）")
    ap.add_argument("--frame",
                    choices=[FRAME_PORTRAIT, FRAME_LANDSCAPE, FRAME_PORTRAIT_SWAP],
                    default=FRAME_PORTRAIT,
                    help="发给 uinput 的坐标框：portrait=面板原生竖屏（默认）/ landscape / "
                         "portrait-swap=竖屏槽位但**预转置**（配合 --usb-id 时必须用，"
                         "否则 Mutter 会把轴转 90°，见 frame_emit 内实测拟合）")
    # ⚠ argparse 会把 `--fx` 的 dest 推断成 `fx`，不是 `flip_x` ⇒ 必须显式 dest
    ap.add_argument("--fx", dest="flip_x", action="store_true", help="emit_x 再反一次（调方向用）")
    ap.add_argument("--fy", dest="flip_y", action="store_true", help="emit_y 再反一次（调方向用）")
    ap.add_argument("--coords-csv", default=None, help="--draw 时把每帧坐标/压力落 CSV")
    ap.add_argument("--longpress-ms", type=float, default=LONGPRESS_MS,
                    help="笔尖按住不动这么久(ms)就合成右键（0=关掉；Windows/触摸语义）")
    ap.add_argument("--longpress-min-pressure", type=float, default=LONGPRESS_MIN_PRESSURE,
                    help="长按需要的最小压感（press_out 0..4095；0=不要求）。"
                         "低于它不武装长按 ⇒ 环与右键都不会出现。默认 2200")
    ap.add_argument("--longpress-tol", type=int, default=LONGPRESS_TOL,
                    help="长按判定允许的位移(横屏像素)；超过就当在划线")
    ap.add_argument("--right-button", choices=["stylus2", "stylus", "right"], default="stylus2",
                    help="右键用哪个按钮码。★ 本机只能用 stylus2：uinput 笔被 udev/libwacom 判成 "
                         "tablet（ID_INPUT_TABLET=1），libinput 对 tablet 只认 BTN_STYLUS/BTN_STYLUS2，"
                         "BTN_RIGHT 既不在能力位里、发出去也会被 libinput 丢弃（2026-10-17 定案）")
    ap.add_argument("--hover-state-file", default="/run/pen-daemon/hover",
                    help="把「笔在附近(1)/不在(0)」写在这里，供 GNOME Shell 扩展隐藏光标（空串=不写）")
    ap.add_argument("--press-state-file", default="/run/pen-daemon/press",
                    help="把「长按进度」写在这里（`x y 进度`，进度<0 表示未按），"
                         "供 GNOME Shell 扩展画环形进度（空串=不写）")
    ap.add_argument("--no-cursor-mirror", dest="cursor_mirror", action="store_false",
                    help="关掉「光标镜像」设备（默认开）。它是**非平板**的绝对指针，"
                         "用来让 Mutter 的核心指针跟着笔走 —— 不开的话，抬笔后光标会"
                         "跳回鼠标上次的位置。根因见 CursorMirror 的文档字符串。")
    ap.add_argument("--mirror-follow", dest="mirror_follow", action="store_true",
                    help="镜像**每帧**跟随笔尖（默认关）。治 26.04/Mutter 50 下"
                         "「平板工具不再驱动核心指针」⇒ XWayland / Electron 应用"
                         "（如 WorkBuddy 客户端）无法被笔操作。"
                         "副作用：若某环境下平板路径也在动指针，会出现双源抖动 —— "
                         "用 libinput debug-events 看 device 6 有没有 Motion 即可判定。")
    ap.add_argument("--x-smooth", type=float, default=0.5, metavar="A",
                    help="对横屏 x 叠一层一阶低通 y=(1-A)x+A*y_prev（0=关）。治的是**悬停光标**"
                         "抖动，与划线抖动无关（那已由删除 qc 冻结修掉）。实测代价：0.5 ⇒ "
                         "一帧滞后 4.4ms / 偏差 p90 2px；0.8 ⇒ 4 帧 17.5ms / p90 8px")
    ap.add_argument("--verbose", action="store_true")
    ap.add_argument("--seconds", type=float, default=None, help="运行时长（默认无限）")
    ap.add_argument("--dump-arrays", type=int, default=0, help="打印前 N 帧的 9×9 阵列")
    a = ap.parse_args()

    pen = None if a.no_uinput else Pen(frame=a.frame, usb_id=a.usb_id)
    mirror = None
    if pen is not None and a.cursor_mirror:
        mirror = CursorMirror()
    try:
        if a.draw:
            return run_daemon(a, pen, mirror)
        if a.selftest:
            selftest(pen, a.hold, mirror=mirror)
            return 0
        log("打开帧设备", a.frame_dev,
            "（记录 560 B=hs_frame 或 352 B=pen_rec，按长度自动判；裸帧 %d B）" % FRAME_LEN)
        n = 0
        t0 = time.time()
        last = t0
        buf = []
        dumped = 0
        for rec in read_records(a.frame_dev, a.seconds):
            n += 1
            f = parse_record(rec)
            if dumped < a.dump_arrays:
                dumped += 1
                log("ARRAY#%d seq=%d fmt=%s ctr=%d nonzero=%d valid1=%d valid2=%d peak1=%d peak2=%d magic_ok=%s"
                    % (dumped, f["seq"], f["fmt"], f["ctr"], f["nonzero"], f["valid1"],
                       f["valid2"], f["peak1"], f["peak2"], f["magic_ok"]))
                for name, arr in (("tx1", f["arr1"]), ("tx2", f["arr2"])):
                    log("  %s 9x9:" % name)
                    for r in range(COLS):
                        log("   ", [arr[r * COLS + c] for c in range(COLS)])
                continue
            buf.append(f)
            now = time.time()
            if now - last >= 1.0:
                span = now - t0
                p1 = [b["peak1"] for b in buf]
                p2 = [b["peak2"] for b in buf]
                nz = sum(1 for b in buf if b["nonzero"])
                v1 = sum(b["valid1"] for b in buf)
                v2 = sum(b["valid2"] for b in buf)
                log("帧 %d | %.1f Hz | nonzero %d/%d | valid1 %d valid2 %d | "
                    "peak1 %d/%d/%d | peak2 %d/%d/%d | 末帧 tx1 idx=%d(r%d,c%d) tx2 idx=%d(r%d,c%d)"
                    % (n, n / span, nz, len(buf), v1, v2,
                       min(p1), sorted(p1)[len(p1) // 2], max(p1),
                       min(p2), sorted(p2)[len(p2) // 2], max(p2),
                       buf[-1]["idx1"], buf[-1]["idx1"] // COLS, buf[-1]["idx1"] % COLS,
                       buf[-1]["idx2"], buf[-1]["idx2"] // COLS, buf[-1]["idx2"] % COLS))
                buf = []
                last = now
    except KeyboardInterrupt:
        pass
    finally:
        if pen:
            pen.close()
        if mirror:
            mirror.close()


if __name__ == "__main__":
    main()
