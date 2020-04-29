#!/bin/sh
set -e -x

[ "$KVER" ] || exit 0

make -j2 $EXTRA
