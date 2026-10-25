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
# # Frame -> coordinate / tilt solver. Algorithm and constants are ported from the
# Android-side upstream gk3pend `solver.h` (SPDX: GPL-2.0 AND MIT), whose HPP3
# step was itself ported from EGoTouchRev(-rebuild). The dual license lets this
# port be distributed under MIT alone; the original notices are preserved below
# in the module docstring.
# ---
"""gk3pen_solver.py —— 从片笔帧的解算（坐标 / 压感 / 倾斜）

算法与常数移植自上游 `gk3pend` 的 `device/huawei/gaokun3/stylus/solver.h`
（SPDX: GPL-2.0 AND MIT）；而它的 HPP3 步骤又移植自
EGoTouchRev-rebuild（https://github.com/awarson2233/EGoTouchRev-rebuild，
EGoTouchService/Solvers/StylusSolver 的 hpp3/CoordinateSolver.hpp、
hpp3/GridFeatureExtractor.hpp、hpp3/PressureSolver.hpp、hpp3/TiltProcess.hpp
与 shared/CoorReviseProcess.hpp）。移植部分的 MIT 许可：

    Copyright (c) 2025 Detach2233
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

──── 本仓实测补正（2026-10-11，见 artifacts/手写笔路线规划-从片路线-2026-10-11.md）
  · 锚点字段的位置由**逐字节比对**独立确认：`b[0x04]`=TX1 锚点 row、`b[0x06]`=TX1 锚点 col、
    `b[0xAA]`/`b[0xAC]`=TX2 的 row/col；`SENSOR_ROWS 40` / `SENSOR_COLS 60`。
  · **本机（BOE 屏，横屏 2560×1600）实测轴映射**：屏幕左上 ⇒ 锚点 (row≈37, col≈57)；
    右下 ⇒ (3, 2)。⇒ `screen_x ∝ (59 − col)`、`screen_y ∝ (39 − row)`，
    即**两轴相对面板都翻转**（上游默认只翻列，因为其目标是 Android 的竖屏 raw 空间）。
    ⇒ 本模块用 `flip_row=True, flip_col=True` 输出**横屏屏幕像素**。
  · 9×9 窗口**不是位置图**（argmax 6276 帧恒为 (4,4)），它只给"锚点格内的小数偏移"。

自检（不需要设备）：`python3 gk3pen_solver.py`
"""
import math

# ── 常数（solver.h）───────────────────────────────────────────────
UNIT = 1024                      # 1/1024 cell
REGION_FLOOR = 100               # GridFeatureExtractor m_peakRegionFloor
SENSOR_COLS = 60
SENSOR_ROWS = 40

PITCH_END = 66                   # BOE：两端 10 列各 33/32 = 66/64 个标称格宽
PITCH_MID = 63                   # BOE：中间 40 列各 63/64

PITCH_COLS = (0.0, -1.7109151490662926, 0.005959771652221362, -5.113555667385272e-06)
PITCH_ROWS = (0.0, -1.4495726495726495, 0.004745726495726496, -3.7393162393162394e-06)

TILT_LEN_COLS = 1330
TILT_LEN_ROWS = 1418
TILT_HIST = 10
REVISE_COLS = 1
REVISE_ROWS = 5

PEAK_MIN = 200                   # HPP3 seed 阈值 199 ⇒ 用 200

# ★★ 接触判据阈值（2026-10-11 本机实测）：TX1 窗口中心格 peak 能干净区分悬空与接触
#     悬空 3~5 mm：peak 中位 332、p90 452、最大 519        （press-20261011-084128 air 段）
#     抬笔后     ：peak 中位 702、p90 781、最大 884
#     接触（轻/中/重）：peak p10 2184~3092、中位 2381~3169
#   ⇒ 1500 两侧各留 ~550 余量（实测确认：air 最大 945、light p10 2035）
#   ⚠ 用途**只限"维持"接触**：压感仍是"开始接触"的唯一依据（上游语义），
#     这样不会因为悬停就凭空出笔。放在 solver 里是因为它与 PEAK_MIN 同属"帧级阈值"。
PEAK_CONTACT = 1500

# ★★★ `qc` 只描述**笔在格内的位置**，**不得**当帧质量判据用（2026-10-17 定案）
#   `_axis_q()` 算的是 `qc = (峰列左邻 + 峰列右邻) / 峰列`。笔在**格心**时左右邻只有一点点
#   泄漏 ⇒ qc ≈ 0.293；笔在**格边**时两侧邻格被照亮 ⇒ qc ≈ 1.008。它随笔的亚格位置**连续**
#   变化（U 形、**无断层**），与信噪比无关：按 qc 分 6 桶量噪声，σ = 0.38/0.40/0.41/0.40/
#   0.38/0.42 px —— **完全相同**（对照：完全没做后处理的 y 轴 σ=0.42–0.49 px）。
#
#   ⚠ 历史坑（2026-10-16，已撤回）：曾据此取 QC_MIN=0.40 把 `qc<0.40` 判成"列向退化帧"、
#     对 x 做冻结/外推。真实后果是 **46.1–46.8%** 的帧（=格心帧，位置最干净的那批）被误冻，
#     并在**跨列格边界**处产生 42.67px 一格的台阶（1 格 = SCREEN_W/SENSOR_COLS = 2560/60）。
#     当时的三条论据全部不成立（详见 docs 与技能 §四十三）：lc 并不随机（自相关 +0.91~+0.97）、
#     跨格帧 |Δx| 实测只有 0–2px、"0.3 桶只有 2/900 帧"实测为 46.8%。
#   ⇒ 冻结分支已从 gk3pend.py 整块删除。**保留此常量只为离线分析分组**（pen-ring-analyze.py /
#     pen-x-regression.py 用它在"格心 vs 格边"之间切桶）。**不要再拿它 gate 任何输出。**
QC_MIN = 0.40


def cdiv(a, b):
    """C 的整数除法（向零截断）。Python 的 // 是向下取整，负数会差 1。"""
    if b == 0:
        return 0
    q = abs(a) // abs(b)
    return -q if (a < 0) != (b < 0) else q


# ── 坐标：格内三角插值 ────────────────────────────────────────────
def tri3(l, c, r):
    """CoordinateSolver::TriangleAlgUsing3Point，返回 [0, UNIT)。"""
    if r < l:
        mn = r if c > r else c - 1
        den = c - mn
        return cdiv(UNIT, 2) - cdiv(cdiv((l - mn) * UNIT, den), 2)
    mn = l if c > l else c - 1
    den = c - mn
    return cdiv(cdiv((r - mn) * UNIT, den), 2) + cdiv(UNIT, 2)


def pitch_comp(coor, k):
    """CoordinateSolver::ApplyPitchCompensation（厂方多项式）。"""
    rem = ((coor % UNIT) + UNIT) % UNIT
    x = (0x200 - rem) if rem < 0x201 else (rem - 0x200)
    dx = float(x)
    comp = int(k[0] + k[1] * dx + k[2] * dx * dx + k[3] * dx * dx * dx)
    return (coor - comp) if rem >= 0x201 else (coor + comp)


def col_bound(k):
    """第 k 列左边缘（1/64 格）。"""
    if k <= 10:
        return k * PITCH_END
    if k <= 50:
        return 10 * PITCH_END + (k - 10) * PITCH_MID
    return 10 * PITCH_END + 40 * PITCH_MID + (k - 50) * PITCH_END


def col_pitch_map(col):
    """BOE 屏列不等距 → 物理位置（同单位）。"""
    if col < 0:
        k = 0
    elif col >= SENSOR_COLS * UNIT:
        k = SENSOR_COLS - 1
    else:
        k = col // UNIT
    lo = col_bound(k)
    return (lo * UNIT + (col - k * UNIT) * (col_bound(k + 1) - lo)) // 64


def edge_pos(peak, inner):
    """最外格里按 inner/peak 比例外推到传感边界。"""
    q = cdiv(inner * 1000, peak) if peak > 0 else 0
    if q >= 1000:
        return UNIT - 1
    if q >= 200:
        return cdiv((550 + (q - 200) * 55 // 100) * UNIT, 1000)
    if q <= 50:
        return 0
    return cdiv(550 * (q - 50) // 150 * UNIT, 1000)


def solve_axis(p, pk, lo, hi):
    """SolveByTriangle（单轴）。lo/hi = 窗口内第一个/最后一个传感格的索引，或 -1。"""
    if pk == lo and pk + 1 <= 8:
        return pk * UNIT + edge_pos(p[pk], p[pk + 1])
    if pk == hi and pk - 1 >= 0:
        return (pk + 1) * UNIT - edge_pos(p[pk], p[pk - 1])
    return pk * UNIT + tri3(p[pk - 1] if pk else 0, p[pk], p[pk + 1] if pk < 8 else 0)


def _edge_index(idx):
    return idx if 0 <= idx <= 8 else -1


def _solve_local(g, ar, ac):
    """9×9 窗口内 TX1 峰的**局部**位置（1/UNIT 格，相对窗口起点）。

    g: 81 个 int，行优先；ar/ac: 锚点（窗口第 4 格对应的传感格）。
    返回 (col, row, qc, qr)：col/row 在 [0, 9*UNIT)；
    qc/qr = 列向/行向**质量**（见 QC_MIN），**99.0 = 判不了**（峰贴窗口边/无信号）。
    """
    inreg = [0] * 81
    stack = [40]
    inreg[40] = 1
    r0 = r1 = c0 = c1 = 4

    rlo = 0 if 4 - ar < 0 else 4 - ar
    rhi = 8 if 4 + SENSOR_ROWS - 1 - ar > 8 else 4 + SENSOR_ROWS - 1 - ar
    clo = 0 if 4 - ac < 0 else 4 - ac
    chi = 8 if 4 + SENSOR_COLS - 1 - ac > 8 else 4 + SENSOR_COLS - 1 - ac

    while stack:
        k = stack.pop()
        r, c = divmod(k, 9)
        r0, r1 = min(r0, r), max(r1, r)
        c0, c1 = min(c0, c), max(c1, c)
        for dr in (-1, 0, 1):
            for dc in (-1, 0, 1):
                nr, nc = r + dr, c + dc
                n = nr * 9 + nc
                if not (0 <= nr <= 8 and 0 <= nc <= 8):
                    continue
                if nr < rlo or nr > rhi or nc < clo or nc > chi:
                    continue
                if inreg[n] or g[n] <= REGION_FLOOR:
                    continue
                inreg[n] = 1
                stack.append(n)

    pc = [0] * 9
    pr = [0] * 9
    for i in range(9):
        if clo <= i <= chi:
            for j in range(r0, r1 + 1):
                v = g[j * 9 + i]
                if v > 0:
                    pc[i] += v
        if rlo <= i <= rhi:
            for j in range(c0, c1 + 1):
                v = g[i * 9 + j]
                if v > 0:
                    pr[i] += v

    ic = ir = 4
    for i in range(9):
        if pc[i] > pc[ic]:
            ic = i
        if pr[i] > pr[ir]:
            ir = i

    col = pitch_comp(solve_axis(pc, ic, _edge_index(4 - ac),
                               _edge_index(4 + SENSOR_COLS - 1 - ac)), PITCH_COLS)
    row = pitch_comp(solve_axis(pr, ir, _edge_index(4 - ar),
                               _edge_index(4 + SENSOR_ROWS - 1 - ar)), PITCH_ROWS)
    return col, row, _axis_q(pc, ic, clo, chi), _axis_q(pr, ir, rlo, rhi)


def _axis_q(prof, k, lo, hi):
    """单轴质量 = (左邻 + 右邻) / 峰。

    ⚠ 只在「峰居中 **且** 左右邻都落在可算范围内」时给值，否则 99.0（判不了 ⇒
      调用方按「可用」处理）。**这个范围判断不能省**：峰贴传感边界时，`pc[]` 的
      越界列本来就被 `if clo <= i <= chi` 记 0，不判就会把边界帧误判成「退化」，
      于是光标在**屏幕边缘被冻住** —— 那是比抖更糟的故障。
    """
    if not (0 < k < 8) or k - 1 < lo or k + 1 > hi or prof[k] <= 0:
        return 99.0
    return (prof[k - 1] + prof[k + 1]) / float(prof[k])


def solve_local(g, ar, ac):
    """向后兼容：只要 (col, row)。"""
    col, row, _qc, _qr = _solve_local(g, ar, ac)
    return col, row


def solve_local_q(g, ar, ac):
    """(col, row, qc, qr) —— qc 是**列向质量**，用于识别「列向退化帧」（见 QC_MIN）。"""
    return _solve_local(g, ar, ac)


# ── 压感 ────────────────────────────────────────────────────────
def map_pressure(x):
    """PressureSolver::MapPressure（厂方曲线）：200→408、1000→2625、2000→3786。"""
    if x >= 0x0FFF:
        return 0x0FFF
    if x <= 11:
        return 1 if x > 1 else x
    if x <= 127:
        m = 0.0078740157480315 * x * x
    else:
        d = float(x)
        m = (-409.317785463 + d * (4.39982201266 + d * (-0.00161165641489 +
             d * (2.623779267e-07 + d * -1.60182e-11))))
    if m < 0:
        m = 0
    if m > 0x0FFF:
        m = 0x0FFF
    return int(m)


def sum3x3(g, pr, pc):
    s = 0
    for r in range(pr - 1, pr + 2):
        for c in range(pc - 1, pc + 2):
            if 0 <= r < 9 and 0 <= c < 9 and g[r * 9 + c] > 0:
                s += g[r * 9 + c]
    return s


# ── 倾斜（TX1 笔尖 vs TX2 环形电极偏移）────────────────────────────
def _isqrt(x):
    r, bit = 0, 0x8000
    while bit and x < bit:
        bit >>= 1
    while bit:
        r += bit
        if x < r * r:
            r -= bit
        bit >>= 1
    return r


def _asin(x):
    neg = x < 0
    if neg:
        x = -x
    x = min(x, 1.0)
    r = 1.5707963267948966 - math.sqrt(1.0 - x) * (1.5707288 + x * (-0.2121144 + x * (0.0742610 + x * -0.0187293)))
    return -r if neg else r


def _tilt_deg(diff, length):
    if diff <= -length:
        return -90
    if diff >= length:
        return 90
    return int(_asin(diff / length) * 180.0 / 3.141592657)


def _tilt_len_limit(ratio):
    th = (20, 50, 65, 80)
    sc = (0, 500, 750, 1000)
    scale = 1000
    if ratio <= th[0]:
        return 0
    if ratio > th[3]:
        return TILT_LEN_COLS
    for i in range(3):
        if th[i] < ratio <= th[i + 1]:
            scale = sc[i] + (ratio - th[i]) * (sc[i + 1] - sc[i]) // (th[i + 1] - th[i])
            break
    return TILT_LEN_COLS * scale // 1000


def _avg(b, have, n):
    n = min(n, have)
    if n <= 0:
        return 0
    return sum(b[:n]) // n


def solve_tx2(g2, a2r, a2c):
    """TX2（环形电极）全局位置（1/UNIT 格）与信号。返回 (col, row, sig) 或 None。"""
    v = [x if x > 0 else 0 for x in g2]
    pk, pv = -1, 99
    for k, val in enumerate(v):
        if val > pv:
            pv, pk = val, k
    if pk < 0:
        return None
    pr, pc = divmod(pk, 9)
    r0, r1 = max(0, pr - 2), min(8, pr + 2)
    c0, c1 = max(0, pc - 2), min(8, pc + 2)
    cells = [(r, c) for r in range(r0, r1 + 1) for c in range(c0, c1 + 1)]
    tot = sum(v[r * 9 + c] for r, c in cells)
    base = tot // len(cells)
    w = wc = wr = 0
    for r, c in cells:
        x = v[r * 9 + c] - base
        if x > 0:
            w += x
            wc += c * x
            wr += r * x
    col = (cdiv(wc * UNIT, w) if w else pc * UNIT) + UNIT // 2 + (a2c - 4) * UNIT
    row = (cdiv(wr * UNIT, w) if w else pr * UNIT) + UNIT // 2 + (a2r - 4) * UNIT
    return col, row, sum3x3(v, pr, pc)


class TiltState:
    """TiltProcess::Process + CoorReviseProcess 的状态。"""

    def __init__(self):
        self.ratio = [0] * TILT_HIST
        self.ratio_n = 0
        self.dc = [0] * TILT_HIST
        self.dr = [0] * TILT_HIST
        self.d_n = 0
        self.tc = [0] * TILT_HIST
        self.tr = [0] * TILT_HIST
        self.last_dc = self.last_dr = 0
        self.out_c = self.out_r = 0
        self.have_out = 0
        self.prev_press = 0
        self.rc = [0] * TILT_HIST
        self.rr = [0] * TILT_HIST
        self.r_n = 0
        self.sm_c = self.sm_r = 0
        self.rev_prev_press = 0

    @staticmethod
    def _push(b, v):
        for i in range(TILT_HIST - 1, 0, -1):
            b[i] = b[i - 1]
        b[0] = v

    def update(self, t1c, t1r, t2c, t2r, sig1, sig2, press):
        if not self.have_out or not self.prev_press:
            self.ratio_n = self.d_n = 0
            self.last_dc = self.last_dr = 0
            for i in range(TILT_HIST):
                self.ratio[i] = self.dc[i] = self.dr[i] = self.tc[i] = self.tr[i] = 0
        ratio = min(cdiv(sig2 * 100, sig1) if sig1 else 0, 500)
        self._push(self.ratio, ratio)
        self.ratio_n = min(self.ratio_n + 1, TILT_HIST)
        lim = _tilt_len_limit(_avg(self.ratio, self.ratio_n, 3))

        dc = t1c - t2c
        dr = t1r - t2r
        if self.d_n == 0:
            dc = max(-lim, min(lim, dc))
            dr = max(-lim, min(lim, dr))
        elif dc > lim or dc < -lim or dr > lim or dr < -lim:
            dc = (dc + self.last_dc * 7) // 8
            dr = (dr + self.last_dr * 7) // 8
        self._push(self.dc, dc)
        self._push(self.dr, dr)
        self.d_n = min(self.d_n + 1, TILT_HIST)
        dc = _avg(self.dc, self.d_n, 5)
        dr = _avg(self.dr, self.d_n, 5)
        self.last_dc, self.last_dr = dc, dr
        if dc > lim or dc < -lim or dr > lim or dr < -lim:
            dc, dr = self.dc[0], self.dr[0]
        mag = _isqrt(dc * dc + dr * dr) or 1
        if lim < mag:
            dc = cdiv(lim * dc, mag)
            dr = cdiv(lim * dr, mag)
        tc = _tilt_deg(dc, TILT_LEN_COLS)
        tr = _tilt_deg(dr, TILT_LEN_ROWS)
        self._push(self.tc, tc)
        self._push(self.tr, tr)
        if self.prev_press and self.out_c:
            tc = _avg(self.tc, self.d_n, 5)
            tr = _avg(self.tr, self.d_n, 5)
        self.out_c = self.out_c if tc == self.out_c else (self.out_c + (1 if tc > self.out_c else -1))
        self.out_r = self.out_r if tr == self.out_r else (self.out_r + (1 if tr > self.out_r else -1))
        self.have_out = 1
        self.prev_press = press

    def revise(self, press, col, row):
        oc = REVISE_COLS * self.out_c
        orr = REVISE_ROWS * self.out_r
        if self.rev_prev_press and not press:
            self.r_n = 0
        self.rev_prev_press = press
        if press:
            if self.r_n:
                oc = max(self.sm_c - 15, min(self.sm_c + 15, oc))
                orr = max(self.sm_r - 15, min(self.sm_r + 15, orr))
            self._push(self.rc, oc)
            self._push(self.rr, orr)
            self.r_n = min(self.r_n + 1, TILT_HIST)
            self.sm_c = _avg(self.rc, self.r_n, 5)
            self.sm_r = _avg(self.rr, self.r_n, 5)
        else:
            self.sm_c, self.sm_r = oc, orr
        return col + self.sm_c, row + self.sm_r


# ── 面板 → 屏幕像素（本机 BOE 横屏 2560×1600）─────────────────────
SCREEN_W = 2560
SCREEN_H = 1600


def to_screen(col, row, col_map=True, flip_col=True, flip_row=True):
    """把绝对传感器坐标（1/UNIT 格）转成**横屏屏幕像素**。

    ★ 轴映射是本机实测的（不是照抄上游）：
        屏幕左上 ⇒ 锚点 (row≈37, col≈57)；右下 ⇒ (3, 2)
        ⇒ screen_x ∝ (59 − col)，screen_y ∝ (39 − row)  ⇒ 两轴都翻。
    """
    if col_map:
        col = col_pitch_map(col)
    if flip_col:
        col = SENSOR_COLS * UNIT - col
    if flip_row:
        row = SENSOR_ROWS * UNIT - row
    x = col * SCREEN_W // (SENSOR_COLS * UNIT)     # 60 格 → 2560
    y = row * SCREEN_H // (SENSOR_ROWS * UNIT)     # 40 格 → 1600
    return max(0, min(SCREEN_W - 1, x)), max(0, min(SCREEN_H - 1, y))


def panel_is_boe(path="/sys/firmware/devicetree/base/soc@0/display-subsystem@ae00000/"
                      "dsi@ae94000/panel@0/compatible"):
    try:
        with open(path, "rb") as f:
            return f.read(4) == b"boe,"
    except OSError:
        return False


# ── 自检（不需要设备）─────────────────────────────────────────────
def _selftest():
    ok = True

    def chk(name, cond, detail=""):
        nonlocal ok
        print("  %s %s%s" % ("PASS" if cond else "FAIL", name, ("  <- " + detail) if (detail and not cond) else ""))
        if not cond:
            ok = False

    # 1) tri3 对对称三角应给半格
    #     ⚠ 用 l==r != c（对称峰）；l==c==r 是退化输入，C 原版同样会返回 UNIT
    print("== tri3 ==")
    chk("对称峰 l=r → UNIT/2", abs(tri3(1000, 2000, 1000) - UNIT // 2) <= 1, repr(tri3(1000, 2000, 1000)))
    chk("对称峰(另一组) → UNIT/2", abs(tri3(500, 1000, 500) - UNIT // 2) <= 1, repr(tri3(500, 1000, 500)))
    chk("左高右低 → 偏左(<UNIT/2)", tri3(3000, 1000, 500) < UNIT // 2)
    chk("右高左低 → 偏右(>UNIT/2)", tri3(500, 1000, 3000) > UNIT // 2)

    # 2) solve_local：把峰放在窗口中心（cell 40）、锚点在传感器中心 (20,30)
    print("== solve_local ==")
    g = [0] * 81
    # 造一个以 (4,4) 为中心的对称 3x3 峰
    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            g[(4 + dr) * 9 + (4 + dc)] = 3000 if (dr == 0 and dc == 0) else 1000
    lc, lr = solve_local(g, 20, 30)
    chk("局部 col ≈ 4.5 格", abs(lc - 4608) < 40, str(lc))
    chk("局部 row ≈ 4.5 格", abs(lr - 4608) < 40, str(lr))
    col = lc + (30 - 4) * UNIT
    row = lr + (20 - 4) * UNIT
    chk("绝对 col ≈ 30.5 格", abs(col - 31232) < 64, str(col))
    chk("绝对 row ≈ 20.5 格", abs(row - 20992) < 64, str(row))

    # 3) 峰移到窗口右上方 ⇒ 局部 col 变大、row 变小
    print("== 峰移动方向 ==")
    g2 = [0] * 81
    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            g2[(2 + dr) * 9 + (6 + dc)] = 3000 if (dr == 0 and dc == 0) else 1000
    lc2, lr2 = solve_local(g2, 20, 30)
    chk("峰右移 ⇒ 局部 col 变大", lc2 > lc, "%d vs %d" % (lc2, lc))
    chk("峰上移 ⇒ 局部 row 变小", lr2 < lr, "%d vs %d" % (lr2, lr))

    # 4) 屏幕映射：把本机实测的两个锚点喂进去，看是否落在屏幕左上/右下附近
    print("== to_screen（用本机实测锚点核对）==")
    # 左上：锚点 (37, 57) → 屏幕应靠左上
    x, y = to_screen(57 * UNIT + UNIT // 2, 37 * UNIT + UNIT // 2, col_map=False)
    chk("锚点(37,57) → 屏幕左上区", x < 400 and y < 250, "(x=%d,y=%d)" % (x, y))
    # 右下：锚点 (3, 2) → 屏幕应靠右下
    x, y = to_screen(2 * UNIT + UNIT // 2, 3 * UNIT + UNIT // 2, col_map=False)
    chk("锚点(3,2) → 屏幕右下区", x > 2100 and y > 1300, "(x=%d,y=%d)" % (x, y))
    # 中心
    x, y = to_screen(28 * UNIT, 21 * UNIT, col_map=False)
    chk("锚点(21,28) → 屏幕中央区", 1000 < x < 1500 and 600 < y < 1000, "(x=%d,y=%d)" % (x, y))

    # 5) 压感曲线抽样（对上游注释的三个点）
    print("== map_pressure ==")
    chk("200 → 408", abs(map_pressure(200) - 408) <= 1, str(map_pressure(200)))
    chk("1000 → 2625", abs(map_pressure(1000) - 2625) <= 1, str(map_pressure(1000)))
    chk("2000 → 3786", abs(map_pressure(2000) - 3786) <= 1, str(map_pressure(2000)))
    chk("0 → 0", map_pressure(0) == 0)
    chk("4095 封顶", map_pressure(9999) == 4095)

    # 6) col_pitch_map 单调且端点自洽
    print("== col_pitch_map ==")
    prev = -1
    mono = True
    for k in range(0, SENSOR_COLS * UNIT):
        v = col_pitch_map(k)
        if v < prev:
            mono = False
            break
        prev = v
    chk("单调不减", mono)
    chk("col=0 → 0", col_pitch_map(0) == 0, str(col_pitch_map(0)))

    # 7) 列向质量判据（2026-10-16 从片路线「抖动/台阶」的判据）
    print("== solve_local_q（列向/行向质量）==")
    gl = [0] * 81
    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            gl[(4 + dr) * 9 + (4 + dc)] = 3000 if (dr == 0 and dc == 0) else 1000
    _c, _r, qc, qr = solve_local_q(gl, 20, 30)
    chk("列向有邻格 ⇒ qc ≥ QC_MIN", qc >= QC_MIN, "qc=%.3f" % qc)
    chk("行向有邻格 ⇒ qr ≥ QC_MIN", qr >= QC_MIN, "qr=%.3f" % qr)
    # 退化：水平邻格只有 40（< REGION_FLOOR）⇒ 区域塌成单列 ⇒ 格内小数塌成格心
    gd = [0] * 81
    for dr in (-1, 0, 1):
        gd[(4 + dr) * 9 + 4] = 3000 if dr == 0 else 1000
    gd[4 * 9 + 3] = gd[4 * 9 + 5] = 40
    cd, _rd, qcd, qrd = solve_local_q(gd, 20, 30)
    chk("列向退化 ⇒ qc < QC_MIN", qcd < QC_MIN, "qc=%.3f" % qcd)
    chk("列向退化 ⇒ 格内小数 = UNIT/2（格心）", cd % UNIT == UNIT // 2, str(cd % UNIT))
    chk("列向退化不影响行向 qr", qrd >= QC_MIN, "qr=%.3f" % qrd)
    chk("solve_local 仍返回 (col,row) 二元组", len(solve_local(gl, 20, 30)) == 2)
    # 峰贴边界 ⇒ 判不了，必须回 99.0（否则屏幕边缘会被误冻）
    gb = [0] * 81
    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            gb[(4 + dr) * 9 + (4 + dc)] = 3000 if (dr == 0 and dc == 0) else 1000
    _cb, _rb, qcb, _qrb = solve_local_q(gb, 20, 0)     # ac=0 ⇒ chi=min(8,4+59)=8, clo=4
    chk("峰在可算范围外 ⇒ qc=99.0（不可判）", qcb == 99.0, "qc=%.3f" % qcb)

    print("\n%s" % ("全部通过 ✓" if ok else "**有失败项 ✘**"))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(_selftest())
