#!/bin/sh
set -e -x

[ "$BASE" ] || exit 0

find . -name '*.tap' -print0 | xargs -0 -n1 prove -e cat -f
