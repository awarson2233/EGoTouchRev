#!/usr/bin/env bash
# 在 linux-gaokun 的 himax-spi 与 himax-thp 之间切换 Master，并管理 Slave 的 DT overlay（开发用，需要 root）。
#   thp-bind.sh on [--no-slave]   Master 从 himax-spi 解绑并卸掉 himax-spi，打上 thp-slave-dt（打开 spi20、
#                                 Master 节点加上 himax,cascade），再加载 himax-thp 绑定两路（触摸屏在切回之前
#                                 不可用）。顺序有讲究：Slave 与 Master 的 compatible 相同，overlay 出现时 himax-spi
#                                 还在的话会去抢 Slave；Master 要在 overlay 之后绑定，probe 时才看得到 himax,cascade。
#                                 --no-slave 不打 overlay，只有 Master 一路
#   thp-bind.sh off               交还 Master 给 himax-spi，撤掉 overlay，卸载 himax-thp
#   thp-bind.sh show              当前绑定情况
# overlay 把 Master 的 INT 从启动 dtb 的低电平改成下降沿，而中断的触发方式在本次开机第一次映射时就定了：
# 这次开机 himax-spi 绑定过 Master 的话 on 会失败（dmesg 里有 type mismatch），反过来 on 之后 off 也一样，要重启。
# --no-slave 不打 overlay，Master 仍是低电平：能切换，但 himax-thp 会告警、每帧可能读两次，只作调试用。
set -eu

SYS=/sys/bus/spi
KO_DIR="$(cd "$(dirname "$0")/../.." && pwd)/build/kernel/$(uname -r)"
KO="$KO_DIR/himax-thp.ko"
SLAVE_KO="$KO_DIR/thp-slave-dt.ko"

# 装过（scripts/linux/install.sh）就按名字加载，没装用 build/kernel/<内核版本>/ 里的
load() { lsmod | grep -q "^$1 " || modprobe "$1" 2>/dev/null || insmod "$2"; }
bound() { basename "$(readlink -f $SYS/devices/$1/driver 2>/dev/null)" 2>/dev/null || true; }
# 按 compatible 找 spi 设备（总线号是动态分配的）。两颗芯片 compatible 相同，有复位脚的是 Master
spi_dev() {
    local d
    for d in $SYS/devices/spi*; do
        [ -e "$d/of_node/compatible" ] || continue
        grep -q 'himax,hx83121a-ts' "$d/of_node/compatible" || continue
        if [ -e "$d/of_node/reset-gpios" ]; then [ "$1" = master ]; else [ "$1" = slave ]; fi &&
            { basename "$d"; return; }
    done
    true
}
master_dev() { spi_dev master; }
slave_dev() { spi_dev slave; }

case "${1:-show}" in
on)
    DEV=$(master_dev)
    echo himax-thp > $SYS/devices/$DEV/driver_override
    [ "$(bound $DEV)" = himax-spi ] && echo $DEV > $SYS/drivers/himax-spi/unbind
    lsmod | grep -q '^himax_hx83121a_spi ' && rmmod himax_hx83121a_spi
    if [ "${2:-}" != --no-slave ]; then
        load thp_slave_dt "$SLAVE_KO"
        load himax_thp "$KO"
    else
        # modprobe 会按 softdep 先打 overlay，这里直接 insmod
        lsmod | grep -q '^himax_thp ' || insmod "$(modinfo -n himax_thp 2>/dev/null || echo "$KO")"
    fi
    [ "$(bound $DEV)" = himax-thp ] || echo $DEV > $SYS/drivers/himax-thp/bind
    ;;
off)
    DEV=$(master_dev)
    [ "$(bound $DEV)" = himax-thp ] && echo $DEV > $SYS/drivers/himax-thp/unbind
    echo > $SYS/devices/$DEV/driver_override
    lsmod | grep -q '^thp_slave_dt ' && rmmod thp_slave_dt
    # 装了 install.sh 之后 himax-spi 不会自动加载（egotouch-modprobe.conf），这里绕过那条规则
    [ -d $SYS/drivers/himax-spi ] || modprobe --ignore-install himax_hx83121a_spi
    [ "$(bound $DEV)" = himax-spi ] || echo $DEV > $SYS/drivers/himax-spi/bind
    lsmod | grep -q '^himax_thp ' && rmmod himax_thp
    ;;
show) ;;
*) echo "usage: $0 on [--no-slave] | off | show"; exit 2 ;;
esac

M=$(master_dev)
S=$(slave_dev)
echo "master -> ${M:-(none)}${M:+ -> $(bound "$M")}"
echo "slave  -> ${S:-(no overlay)}${S:+ -> $(bound "$S")}"
ls -l /dev/himax_thp 2>/dev/null || true
dmesg | grep -E 'himax-(thp|spi)|thp-slave-dt' | tail -4
