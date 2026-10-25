#!/usr/bin/env bash
# 在 linux-gaokun 的 himax-spi 与 himax-thp 之间切换 Master，并管理 Slave 的 DT overlay（开发用，需要 root）。
#   thp-bind.sh on [--no-slave] [--gsi]
#                                 加载 himax-thp 并接管 Master（原驱动解绑，触摸屏在切回之前不可用）；
#                                 再加载 thp-slave-dt 打开 spi20，Slave 由 himax-thp 绑定（spi20 跑 GSI）。
#                                 --gsi：再把 Master 所在的 spi6 控制器解绑重绑，让 overlay 加的
#                                 qcom,force-gsi-mode 生效（Master 设备会重建，总线号可能变）
#   thp-bind.sh off               撤掉 overlay，交还 Master 给 himax-spi，卸载 himax-thp；
#                                 用过 --gsi 的话也重绑 spi6，回到开机时的 FIFO 模式
#   thp-bind.sh show              当前绑定情况
set -eu

SYS=/sys/bus/spi
HERE="$(cd "$(dirname "$0")" && pwd)"
KO="$HERE/himax-thp.ko"
SLAVE_KO="$HERE/thp-slave-dt.ko"
# Master 所在的 spi6 控制器（QUP 0x998000）
MASTER_CTRL=998000.spi
GSI_FLAG=/run/thp-bind-gsi

bound() { basename "$(readlink -f $SYS/devices/$1/driver 2>/dev/null)" 2>/dev/null || true; }
# 按 compatible 找 spi 设备：总线号是动态分配的，控制器重绑之后会变
spi_dev() {
    local d
    for d in $SYS/devices/spi*; do
        [ -e "$d/of_node/compatible" ] || continue
        grep -q "$1" "$d/of_node/compatible" && { basename "$d"; return; }
    done
    true
}
master_dev() { spi_dev 'himax,hx83121a-ts'; }
slave_dev() { spi_dev 'egotouch,hx83121a-thp-slave'; }

rebind_master_ctrl() {
    echo $MASTER_CTRL > /sys/bus/platform/drivers/geni_spi/unbind
    echo $MASTER_CTRL > /sys/bus/platform/drivers/geni_spi/bind
    # 等 Master 设备重新出现，原驱动 himax-spi 会先绑上去
    local i
    for i in $(seq 50); do [ -n "$(master_dev)" ] && return; sleep 0.1; done
    echo "master spi device did not come back"; exit 1
}

case "${1:-show}" in
on)
    shift
    slave=1; gsi=0
    for a in "$@"; do
        case "$a" in
        --no-slave) slave=0 ;;
        --gsi) gsi=1 ;;
        *) echo "unknown option $a"; exit 2 ;;
        esac
    done
    [ $gsi = 1 ] && [ $slave = 0 ] && { echo "--gsi needs the overlay, drop --no-slave"; exit 2; }
    [ -f "$KO" ] || { echo "missing $KO, build it first"; exit 1; }
    lsmod | grep -q '^himax_thp ' || insmod "$KO"
    if [ $slave = 1 ]; then
        [ -f "$SLAVE_KO" ] || { echo "missing $SLAVE_KO, build it first"; exit 1; }
        lsmod | grep -q '^thp_slave_dt ' || insmod "$SLAVE_KO"
    fi
    if [ $gsi = 1 ]; then
        rebind_master_ctrl
        touch $GSI_FLAG
    fi
    DEV=$(master_dev)
    [ "$(bound $DEV)" = himax-spi ] && echo $DEV > $SYS/drivers/himax-spi/unbind
    echo himax-thp > $SYS/devices/$DEV/driver_override
    [ "$(bound $DEV)" = himax-thp ] || echo $DEV > $SYS/drivers/himax-thp/bind
    ;;
off)
    # 先撤 overlay：Slave 设备随之消失、从 Master 上断开
    lsmod | grep -q '^thp_slave_dt ' && rmmod thp_slave_dt
    DEV=$(master_dev)
    [ "$(bound $DEV)" = himax-thp ] && echo $DEV > $SYS/drivers/himax-thp/unbind
    echo > $SYS/devices/$DEV/driver_override
    if [ -e $GSI_FLAG ]; then
        rebind_master_ctrl
        rm -f $GSI_FLAG
    fi
    DEV=$(master_dev)
    [ "$(bound $DEV)" = himax-spi ] || echo $DEV > $SYS/drivers/himax-spi/bind
    lsmod | grep -q '^himax_thp ' && rmmod himax_thp
    ;;
show) ;;
*) echo "usage: $0 on [--no-slave] [--gsi] | off | show"; exit 2 ;;
esac

M=$(master_dev)
S=$(slave_dev)
echo "master -> ${M:-(none)}${M:+ -> $(bound "$M")}$([ -e $GSI_FLAG ] && echo ' (spi6 GSI)')"
echo "slave  -> ${S:-(no overlay)}${S:+ -> $(bound "$S")}"
ls -l /dev/himax_thp 2>/dev/null || true
dmesg | grep -E 'himax-(thp|spi)|thp-slave-dt|geni_spi|spi_geni' | tail -6
