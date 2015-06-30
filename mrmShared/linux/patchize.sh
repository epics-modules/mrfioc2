#!/bin/sh

#
# Creates a patch which can be applied against a kernel source tree
#
# Usage: ./patchize.sh /location/of/kernel/src
#

die() {
	echo "$1" >&2
	exit 1
}

if [ -z "$1" ]; then
	KDIR=/lib/modules/`uname -r`/build
else
	KDIR="$1"
fi

PDIR=drivers/uio

fdiff() {
	diff "$@" | sed -e "s|^\(--- \)${KDIR}/|\1a/|" -e "s|^\(+++ \)\(.*\)\.tmp|\1b/${PDIR}/\2|"
}

# First do kconfig

sed -e '/if UIO/ r Kconfig' "$KDIR/$PDIR/Kconfig" > Kconfig.tmp \
|| die "Failed to parse Kconfig"

fdiff -u "$KDIR/$PDIR/Kconfig" Kconfig.tmp

rm -f Kconfig.tmp

cp "$KDIR/$PDIR/Makefile" Makefile.tmp

echo 'obj-$(CONFIG_UIO_MRF)   += uio_mrf.o' >> Makefile.tmp

fdiff -u "$KDIR/$PDIR/Makefile" Makefile.tmp

rm -f Makefile.tmp

diff -u /dev/null uio_mrf.c | sed -e "s|^\(+++ \)\(.*\)|\1b/${PDIR}/\2|"
