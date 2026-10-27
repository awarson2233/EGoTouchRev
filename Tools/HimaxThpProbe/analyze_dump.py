#!/usr/bin/env python3
"""分析 HimaxThpProbe --dump 存下的 master 帧：坏帧坏在哪里。

  python3 -I analyze_dump.py frames.bin

每条记录 = 1 字节校验结果（0 有效 / 1 全零 / 2 校验错 / 3 总线填充）+ 5063 字节帧。
帧布局见 Common/include/FrameLayout.h：7 字节头 + 40x60 个 u16 heatmap + 128 个 u16 帧尾。
"""
import collections
import struct
import sys

FRAME = 5063
HEADER = 7
CELLS = 40 * 60
SUFFIX_OFF = HEADER + CELLS * 2
SUFFIX_WORDS = 128
TIMESTAMP_WORD = 9          # FrameLayout.h MasterWord::kTimestamp
NAMES = {0: "valid", 1: "zero", 2: "checksum", 3: "filler"}


def words(buf, off, n):
    return struct.unpack_from("<%dH" % n, buf, off)


def residue(frame):
    body = frame[5:]
    s = sum(struct.unpack_from("<%dH" % (len(body) // 2), body))
    return s & 0xFFFF


def main(path):
    data = open(path, "rb").read()
    rec = 1 + FRAME
    frames = [(data[i], data[i + 1:i + rec]) for i in range(0, len(data) - rec + 1, rec)]
    print("records: %d" % len(frames))
    print("checks:", dict(collections.Counter(NAMES[c] for c, _ in frames)))

    # 好坏的节奏：连续段长度
    runs, cur, n = [], None, 0
    for c, _ in frames:
        good = c == 0
        if good == cur:
            n += 1
        else:
            if cur is not None:
                runs.append(("G" if cur else "B") + str(n))
            cur, n = good, 1
    runs.append(("G" if cur else "B") + str(n))
    print("run pattern (first 40):", " ".join(runs[:40]))

    # 帧头
    for label, want in (("valid", True), ("bad", False)):
        heads = collections.Counter(f[3:7].hex(" ") for c, f in frames if (c == 0) == want)
        print("header[3..6] %-5s top:" % label, heads.most_common(4))

    # heatmap 值域：撕裂的帧值域正常，位错误的帧会有离谱的值
    def wild(f):
        return sum(1 for v in words(f, HEADER, CELLS) if abs(v - 0x8000) > 0x3000)
    for label, want in (("valid", True), ("bad", False)):
        sel = [wild(f) for c, f in frames if (c == 0) == want]
        if sel:
            print("wild heatmap cells (|v-0x8000|>0x3000) %-5s: mean %.1f max %d" % (label, sum(sel) / len(sel), max(sel)))

    # 校验余数：撕裂的余数随机，单个位错误的余数是 2 的幂
    def is_pow2(x):
        return x > 0 and (x & (x - 1)) == 0

    res = collections.Counter(residue(f) for c, f in frames if c == 2)
    pow2 = sum(v for k, v in res.items() if is_pow2(k) or is_pow2(0x10000 - k))
    print("bad residues: %d distinct, %d look like single-bit errors, top: %s"
          % (len(res), pow2, [hex(k) for k, _ in res.most_common(5)]))

    # 帧尾时间戳：相邻记录之间的增量
    ts = [words(f, SUFFIX_OFF, SUFFIX_WORDS)[TIMESTAMP_WORD] for _, f in frames]
    deltas = collections.Counter((b - a) & 0xFFFF for a, b in zip(ts, ts[1:]))
    print("timestamp deltas top:", deltas.most_common(6))
    print("first 12 (check, ts):", [(NAMES[c][0], t) for (c, _), t in zip(frames[:12], ts[:12])])

    # 坏帧与前一个好帧逐字比较：从哪里开始不同（撕裂常表现为某个位置之后整段变了）
    last_good = None
    first_diff = []
    for c, f in frames:
        if c == 0:
            last_good = f
        elif c == 2 and last_good is not None:
            a = words(last_good, HEADER, CELLS)
            b = words(f, HEADER, CELLS)
            same = [abs(x - y) < 64 for x, y in zip(a, b)]
            first_diff.append(sum(same) / CELLS)
    if first_diff:
        print("bad vs previous good: fraction of heatmap cells within +-64 -> mean %.2f" % (sum(first_diff) / len(first_diff)))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
