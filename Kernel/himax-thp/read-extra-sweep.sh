#!/usr/bin/env bash
# 实验：每帧多读不同字节数时，INT 能否放开、帧是否全部有效（需要 root，himax-thp 已经绑定）。
#   read-extra-sweep.sh [多读字节数...]     默认 0 72 128 512 2048
set -u

PROBE=${PROBE:-$(cd "$(dirname "$0")/../.." && pwd)/build/linux-debug/HimaxThpProbe}
P=/sys/module/himax_thp/parameters
EXTRAS=("$@"); [ ${#EXTRAS[@]} -eq 0 ] && EXTRAS=(0 72 128 512 2048)

[ -w $P/read_extra ] || { echo "himax_thp not loaded or not root"; exit 1; }

for extra in "${EXTRAS[@]}"; do
    echo "$extra" > $P/read_extra
    dmesg -C
    echo 1 > $P/debug_irq
    summary=$("$PROBE" --frames 200 2>/dev/null | grep '^done')
    echo 0 > $P/debug_irq
    log=$(dmesg | grep 'irq dt=')
    total=$(grep -c . <<< "$log")
    released=$(grep -vc 'released=-1us' <<< "$log")
    avg_rel=$(grep -v 'released=-1us' <<< "$log" | sed -E 's/.*released=([0-9]+)us/\1/' | awk '{s+=$1;n++} END{if(n) printf "%d", s/n; else print "-"}')
    hdrs=$(sed -E 's/.*hdr=(....).*/\1/' <<< "$log" | sort | uniq -c | sort -rn | head -3 | awk '{printf "%s:%s ", $2, $1}')
    echo "extra=$extra | $summary"
    echo "          irqs=$total INT released=$released (avg ${avg_rel}us) | hdr top: $hdrs"
done
echo 0 > $P/read_extra
