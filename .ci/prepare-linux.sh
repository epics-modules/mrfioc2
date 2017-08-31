#!/bin/sh
set -e -x

[ "$KVER" ] || exit 0

install -d "$HOME/linux"

curl -s https://cdn.kernel.org/pub/linux/kernel/v$KSER/linux-$KVER.tar.xz | tar -C "$HOME/linux" --strip-components=1 -xJ

make -C "$HOME/linux" defconfig

make -C "$HOME/linux" modules_prepare
