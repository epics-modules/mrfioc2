#!/bin/sh
set -e -x

[ "$BASE" ] || exit 0

make -j2 $EXTRA

[ "$TEST" = "NO" ] && exit 0

make tapfiles

find . -name '*.tap' -print0 | xargs -0 -n1 prove -e cat -f
