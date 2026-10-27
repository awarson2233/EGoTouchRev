#!/usr/bin/env bash
# CI 用：在 ubuntu:26.04 的 arm64 容器里编 egotouchd 并打 deb / rpm，产物在 build/linux-packages/。
#   ci-build.sh <版本>
# egotouchd 全静态链接（只用 ioctl / uhid，不走 NSS），同一个可执行文件放进两种包，不挑发行版的 glibc。
set -euo pipefail

VERSION="$1"
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT"

export DEBIAN_FRONTEND=noninteractive
apt-get update -q
apt-get install -y -q --no-install-recommends clang cmake ninja-build linux-libc-dev dpkg-dev rpm file

cmake --preset linux-Release -DEGO_BUILD_TOOLS=OFF -DCMAKE_EXE_LINKER_FLAGS=-static
cmake --build --preset linux-Release --target egotouchd
file build/linux-Release/egotouchd

scripts/linux/package/build-packages.sh "$VERSION" build/linux-Release/egotouchd build/linux-packages
