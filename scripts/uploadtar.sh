#!/bin/sh
set -e

die() {
    echo "$1"
    exit 1
}

[ -f "$1" ] || die "usage $0 <tarfile> ..."

[ "$USER" ] || die "USER not set"

scp "$@" $USER,epics@frs.sourceforge.net:/home/frs/project/epics/mrfioc2/
