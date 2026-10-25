#!/usr/bin/env bash
# 撤掉 install.sh 装的东西（需要 root）。正在运行的服务停掉、Master 交还 himax-spi，系统触摸回到原驱动。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

systemctl stop egotouchd 2>/dev/null || true
rm -f /etc/systemd/system/egotouchd.service /etc/udev/rules.d/60-egotouch.rules \
      /etc/modprobe.d/egotouch.conf /etc/modules-load.d/egotouch.conf
systemctl daemon-reload
udevadm control --reload
"$ROOT/Kernel/himax-thp/thp-bind.sh" off >/dev/null || true
rm -rf "/lib/modules/$(uname -r)/updates/egotouch" /usr/local/bin/egotouchd
depmod -a
echo "uninstalled; touch is back on himax-spi"
