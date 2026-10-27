#!/usr/bin/env bash
# 把 egotouchd 打成 deb 与 rpm（arm64 / aarch64），只给自带 himax-thp 的 gaokun3 内核用。
#   build-packages.sh <版本> <egotouchd 可执行文件> <输出目录>
# 需要 dpkg-deb 与 rpmbuild。装上后安装脚本调 egotouch-dtb enable 切到 touch-spi dtb（重启生效），
# 卸载时 egotouch-dtb disable 换回。
set -euo pipefail

VERSION="$1"
BIN="$2"
OUT="$3"
HERE="$(cd "$(dirname "$0")" && pwd)"
LINUX="$HERE/.."
NAME=egotouch
SUMMARY="EGoTouch touchscreen daemon for the Huawei MateBook E Go (gaokun3) SPI touch path"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$OUT"

# 两种包装同一棵文件树
stage_root() {
    local r="$1"
    install -Dm755 "$BIN" "$r/usr/bin/egotouchd"
    install -Dm755 "$LINUX/egotouch-dtb" "$r/usr/bin/egotouch-dtb"
    install -Dm644 "$LINUX/60-egotouch.rules" "$r/usr/lib/udev/rules.d/60-egotouch.rules"
    install -d "$r/usr/lib/systemd/system" "$r/usr/lib/modules-load.d"
    sed 's#/usr/local/bin/egotouchd#/usr/bin/egotouchd#' "$LINUX/egotouchd.service" \
        > "$r/usr/lib/systemd/system/egotouchd.service"
    printf 'uhid\n' > "$r/usr/lib/modules-load.d/egotouch.conf"
}

# 首次安装才切 dtb：升级不动用户手动 disable 过的状态。切不了（内核不对、EL2 dtb）只提示，包照样装上
POST_INSTALL='systemctl daemon-reload || true
udevadm control --reload || true
egotouch-dtb enable || echo "egotouch: touch stays on i2c-hid, see the message above; egotouch-dtb status shows the state"'
PRE_REMOVE='systemctl stop egotouchd.service || true
egotouch-dtb disable || true'
POST_REMOVE='systemctl daemon-reload || true
udevadm control --reload || true'

# ---- deb ----
DEB="$WORK/deb"
stage_root "$DEB"
mkdir -p "$DEB/DEBIAN"
cat > "$DEB/DEBIAN/control" <<EOF
Package: $NAME
Version: $VERSION
Architecture: arm64
Maintainer: Detach0-0 <detachovo@gmail.com>
Homepage: https://github.com/awarson2233/EGoTouchRev
Depends: systemd, udev
Section: utils
Priority: optional
Description: $SUMMARY
 Reads raw frames from the himax-thp driver, runs the TSACore touch
 algorithm and reports touches through uhid. Switches the boot device
 tree to sc8280xp-huawei-gaokun3-touch-spi.dtb on install (reboot to
 take effect) and back on removal.
EOF
cat > "$DEB/DEBIAN/postinst" <<EOF
#!/bin/sh
set -e
if [ "\$1" = configure ] && [ -z "\$2" ]; then
$POST_INSTALL
fi
EOF
cat > "$DEB/DEBIAN/prerm" <<EOF
#!/bin/sh
set -e
if [ "\$1" = remove ]; then
$PRE_REMOVE
fi
EOF
cat > "$DEB/DEBIAN/postrm" <<EOF
#!/bin/sh
set -e
if [ "\$1" = remove ] || [ "\$1" = purge ]; then
$POST_REMOVE
fi
EOF
chmod 755 "$DEB/DEBIAN/postinst" "$DEB/DEBIAN/prerm" "$DEB/DEBIAN/postrm"
dpkg-deb --root-owner-group --build "$DEB" "$OUT/${NAME}_${VERSION}_arm64.deb"

# ---- rpm ----
RPM="$WORK/rpm"
stage_root "$RPM/root"
cat > "$RPM/$NAME.spec" <<EOF
Name:           $NAME
Version:        $VERSION
Release:        1
Summary:        $SUMMARY
License:        MIT
URL:            https://github.com/awarson2233/EGoTouchRev
BuildArch:      aarch64
# egotouchd 静态链接 libstdc++，只依赖 glibc；在 Ubuntu 上打包，不用 rpm 的自动依赖扫描
AutoReqProv:    no
Requires:       systemd

%global debug_package %{nil}
%global __strip /bin/true

%description
Reads raw frames from the himax-thp driver, runs the TSACore touch algorithm
and reports touches through uhid. Switches the boot device tree to
sc8280xp-huawei-gaokun3-touch-spi.dtb on install (reboot to take effect) and
back on removal.

%install
cp -a $RPM/root/. %{buildroot}/

%files
/usr/bin/egotouchd
/usr/bin/egotouch-dtb
/usr/lib/udev/rules.d/60-egotouch.rules
/usr/lib/systemd/system/egotouchd.service
/usr/lib/modules-load.d/egotouch.conf

%post
if [ \$1 -eq 1 ]; then
$POST_INSTALL
fi

%preun
if [ \$1 -eq 0 ]; then
$PRE_REMOVE
fi

%postun
if [ \$1 -eq 0 ]; then
$POST_REMOVE
fi
EOF
rpmbuild -bb --target aarch64 --define "_topdir $RPM/top" --define "_build_id_links none" "$RPM/$NAME.spec"
cp "$RPM"/top/RPMS/aarch64/*.rpm "$OUT/"

ls -l "$OUT"
