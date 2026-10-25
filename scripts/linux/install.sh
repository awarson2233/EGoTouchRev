#!/usr/bin/env bash
# 把 himax-thp、egotouchd 装成开机自启（需要 root）。
#   install.sh             安装；重启后生效
#   install.sh --now       安装后马上切换：停掉手动启动的 egotouchd，Master 交给 himax-thp，由服务接管
#
# 先编好：make -C Kernel/himax-thp ...（见 docs/linux_port_status.md §2.2）和 build/linux-release 的 egotouchd。
# 模块装在当前内核的 updates/egotouch/ 下，换内核之后要重新编、重新装（之前触摸退回 himax-spi）。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$ROOT/scripts/linux"
KO_DIR="$ROOT/Kernel/himax-thp"
BIN="$ROOT/build/linux-release/egotouchd"
MODDIR="/lib/modules/$(uname -r)/updates/egotouch"

for f in "$KO_DIR/himax-thp.ko" "$KO_DIR/thp-slave-dt.ko" "$BIN"; do
    [ -f "$f" ] || { echo "missing $f, build it first"; exit 1; }
done

install -D -m 755 "$BIN" /usr/local/bin/egotouchd
install -D -m 644 -t "$MODDIR" "$KO_DIR/himax-thp.ko" "$KO_DIR/thp-slave-dt.ko"
depmod -a
install -D -m 644 "$HERE/egotouch-modprobe.conf" /etc/modprobe.d/egotouch.conf
install -D -m 644 "$HERE/egotouch-modules.conf" /etc/modules-load.d/egotouch.conf
install -D -m 644 "$HERE/60-egotouch.rules" /etc/udev/rules.d/60-egotouch.rules
install -D -m 644 "$HERE/egotouchd.service" /etc/systemd/system/egotouchd.service
systemctl daemon-reload
udevadm control --reload
echo "installed: /usr/local/bin/egotouchd, $MODDIR, /etc/{modprobe.d,modules-load.d}/egotouch.conf,"
echo "           /etc/udev/rules.d/60-egotouch.rules, /etc/systemd/system/egotouchd.service"

if [ "${1:-}" = --now ]; then
    pkill -x egotouchd && sleep 1 || true
    "$KO_DIR/thp-bind.sh" on >/dev/null
    udevadm trigger --action=add --name-match=/dev/himax_thp
    sleep 2
    systemctl --no-pager status egotouchd | head -5
fi
