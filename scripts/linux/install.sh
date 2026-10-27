#!/usr/bin/env bash
# 把 egotouchd 装成开机自启（需要 root）。
#   install.sh             安装；重启后生效
#   install.sh --now       （只限旧内核）安装后马上切换：停掉手动启动的 egotouchd，Master 交给 himax-thp，
#                          由服务接管（这次开机 himax-spi 绑定过 Master 的话切不过去，见 thp-bind.sh 开头）
#
# 按当前内核分两种：
#   新内核：自带 himax-thp（CONFIG_HIMAX_THP）和 sc8280xp-huawei-gaokun3-touch-spi.dtb。默认 dtb 下触摸走
#           i2c-hid（芯片自己解算）；这里由 egotouch-dtb enable 把 /etc/kernel/devicetree 换成 touch-spi dtb、
#           重新生成启动项，重启后两路 SPI 打开、himax-thp 绑定、egotouchd 由 udev 拉起。与安装包
#           （scripts/linux/package/）装的是同一套，只是 egotouchd 放在 /usr/local/bin。
#   旧内核：没有树内 himax-thp。装本仓库编的 himax-thp.ko 与运行时 overlay（thp-slave-dt.ko），
#           modprobe.d 挡住 himax-spi。模块装在当前内核的 updates/egotouch/ 下，换内核要重编重装。
#
# 先编好（见 docs/linux_port_status.md §2）：
#   cmake --preset linux-Release && cmake --build --preset linux-Release   （旧内核另加 --preset linux-kmod）
# 产物：build/linux-Release/egotouchd、build/kernel/<内核版本>/*.ko。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$ROOT/scripts/linux"
KREL="$(uname -r)"
BIN="$ROOT/build/linux-Release/egotouchd"
SPI_DTB="qcom/sc8280xp-huawei-gaokun3-touch-spi.dtb"

# 树内的 himax-thp 在 kernel/ 下；本仓库装的在 updates/egotouch/ 下，不算
kernel_has_himax_thp() {
    local m
    m="$(modinfo -n himax_thp 2>/dev/null || true)"
    [[ "$m" == "/lib/modules/$KREL/kernel/"* || "$m" == "/usr/lib/modules/$KREL/kernel/"* ]]
}

# kernel-install（90-loaderentry）找 devicetree 的位置
kernel_has_spi_dtb() {
    local d
    for d in "/usr/lib/modules/$KREL/dtb" "/usr/lib/linux-image-$KREL" "/usr/lib/firmware/$KREL/device-tree"; do
        [ -f "$d/$SPI_DTB" ] && return 0
    done
    return 1
}

install_daemon() {
    [ -f "$BIN" ] || { echo "missing $BIN, build it first"; exit 1; }
    install -D -m 755 "$BIN" /usr/local/bin/egotouchd
    install -D -m 644 "$HERE/60-egotouch.rules" /etc/udev/rules.d/60-egotouch.rules
    install -D -m 644 "$HERE/egotouchd.service" /etc/systemd/system/egotouchd.service
}

if kernel_has_himax_thp && kernel_has_spi_dtb; then
    [ "${1:-}" = --now ] && { echo "--now only applies to old kernels; this one switches on reboot"; exit 1; }
    install_daemon
    printf 'uhid\n' > /etc/modules-load.d/egotouch.conf
    # 旧内核那一套留下的东西
    rm -f /etc/modprobe.d/egotouch.conf
    rm -rf "/lib/modules/$KREL/updates/egotouch"
    depmod -a "$KREL"
    install -D -m 755 "$HERE/egotouch-dtb" /usr/local/bin/egotouch-dtb
    systemctl daemon-reload
    udevadm control --reload
    echo "installed: /usr/local/bin/{egotouchd,egotouch-dtb}, udev rule, egotouchd.service"
    /usr/local/bin/egotouch-dtb enable
    exit 0
fi

KO_DIR="$ROOT/build/kernel/$KREL"
MODDIR="/lib/modules/$KREL/updates/egotouch"
for f in "$KO_DIR/himax-thp.ko" "$KO_DIR/thp-slave-dt.ko"; do
    [ -f "$f" ] || { echo "missing $f, build it first (old kernel without in-tree himax-thp)"; exit 1; }
done

install_daemon
install -D -m 644 -t "$MODDIR" "$KO_DIR/himax-thp.ko" "$KO_DIR/thp-slave-dt.ko"
depmod -a
install -D -m 644 "$HERE/egotouch-modprobe.conf" /etc/modprobe.d/egotouch.conf
install -D -m 644 "$HERE/egotouch-modules.conf" /etc/modules-load.d/egotouch.conf
systemctl daemon-reload
udevadm control --reload
echo "installed (old kernel): /usr/local/bin/egotouchd, $MODDIR, /etc/{modprobe.d,modules-load.d}/egotouch.conf,"
echo "           /etc/udev/rules.d/60-egotouch.rules, /etc/systemd/system/egotouchd.service"

if [ "${1:-}" = --now ]; then
    pkill -x egotouchd && sleep 1 || true
    "$ROOT/Kernel/himax-thp/thp-bind.sh" on >/dev/null
    udevadm trigger --action=add --name-match=/dev/himax_thp
    sleep 2
    systemctl --no-pager status egotouchd | head -5
fi
