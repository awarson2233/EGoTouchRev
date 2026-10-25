#!/usr/bin/env bash
# 在 linux-gaokun 的 himax-spi 与 himax-thp 之间切换 spi0.0，并管理 Slave 的 DT overlay（开发用，需要 root）。
#   thp-bind.sh on [--no-slave]   加载 himax-thp 并接管 spi0.0（原驱动解绑，触摸屏在切回之前不可用）；
#                                 再加载 thp-slave-dt 打开 spi20，Slave 由 himax-thp 绑定
#   thp-bind.sh off               撤掉 overlay，交还 spi0.0 给 himax-spi，卸载 himax-thp
#   thp-bind.sh show              当前绑定情况
set -eu

DEV=spi0.0
SYS=/sys/bus/spi
HERE="$(cd "$(dirname "$0")" && pwd)"
KO="$HERE/himax-thp.ko"
SLAVE_KO="$HERE/thp-slave-dt.ko"

bound() { basename "$(readlink -f $SYS/devices/$1/driver 2>/dev/null)" 2>/dev/null || true; }
# overlay 打上之后 spi20 下出现的那个 spi 设备（总线号是动态分配的）
slave_dev() {
    local d
    for d in $SYS/devices/spi*; do
        [ -e "$d/of_node/compatible" ] || continue
        grep -q 'egotouch,hx83121a-thp-slave' "$d/of_node/compatible" && { basename "$d"; return; }
    done
    true
}

case "${1:-show}" in
on)
    [ -f "$KO" ] || { echo "missing $KO, build it first"; exit 1; }
    lsmod | grep -q '^himax_thp ' || insmod "$KO"
    [ "$(bound $DEV)" = himax-spi ] && echo $DEV > $SYS/drivers/himax-spi/unbind
    echo himax-thp > $SYS/devices/$DEV/driver_override
    [ "$(bound $DEV)" = himax-thp ] || echo $DEV > $SYS/drivers/himax-thp/bind
    if [ "${2:-}" != --no-slave ]; then
        [ -f "$SLAVE_KO" ] || { echo "missing $SLAVE_KO, build it first"; exit 1; }
        lsmod | grep -q '^thp_slave_dt ' || insmod "$SLAVE_KO"
    fi
    ;;
off)
    # 先撤 overlay：Slave 设备随之消失、从 Master 上断开
    lsmod | grep -q '^thp_slave_dt ' && rmmod thp_slave_dt
    [ "$(bound $DEV)" = himax-thp ] && echo $DEV > $SYS/drivers/himax-thp/unbind
    echo > $SYS/devices/$DEV/driver_override
    [ "$(bound $DEV)" = himax-spi ] || echo $DEV > $SYS/drivers/himax-spi/bind
    lsmod | grep -q '^himax_thp ' && rmmod himax_thp
    ;;
show) ;;
*) echo "usage: $0 on [--no-slave] | off | show"; exit 2 ;;
esac

S=$(slave_dev)
echo "spi0.0 -> $(bound $DEV)"
echo "slave  -> ${S:-(no overlay)}${S:+ -> $(bound "$S")}"
ls -l /dev/himax_thp 2>/dev/null || true
dmesg | grep -E 'himax-(thp|spi)|thp-slave-dt' | tail -4
