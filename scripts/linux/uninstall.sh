#!/usr/bin/env bash
# 撤掉 install.sh 装的东西（需要 root）。
#   新内核：/etc/kernel/devicetree 换回安装前的值、重新生成启动项，重启后触摸回到 i2c-hid
#   旧内核：停掉服务、Master 交还 himax-spi，马上回到原驱动
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
KREL="$(uname -r)"
DT_ORIG=/etc/kernel/devicetree.egotouch-orig

systemctl stop egotouchd 2>/dev/null || true
rm -f /etc/systemd/system/egotouchd.service /etc/udev/rules.d/60-egotouch.rules \
      /etc/modprobe.d/egotouch.conf /etc/modules-load.d/egotouch.conf
systemctl daemon-reload
udevadm control --reload

if [ -e "$DT_ORIG" ]; then
    "$ROOT/scripts/linux/egotouch-dtb" disable --package
    rm -f /usr/local/bin/egotouchd /usr/local/bin/egotouch-dtb
    echo "uninstalled"
    exit 0
fi

"$ROOT/Kernel/himax-thp/thp-bind.sh" off >/dev/null || true
rm -rf "/lib/modules/$KREL/updates/egotouch" /usr/local/bin/egotouchd
depmod -a
echo "uninstalled; touch is back on himax-spi"
