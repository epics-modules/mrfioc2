#!/bin/sh
set -e -x

# Build Base for use with https://travis-ci.org
#
# Set environment variables
# BASE= 3.14 3.15 or 3.16  (VCS branch)
# STATIC=  static or shared

die() {
  echo "$1" >&2
  exit 1
}

[ "$BASE" ] || exit 0

CDIR="$HOME/.cache/base-$BASE-$STATIC"
EPICS_BASE="$CDIR/base"

if [ ! -e "$CDIR/built" ]
then
  install -d "$CDIR"
  ( cd "$CDIR" && git clone --depth 50 --branch $BASE https://github.com/epics-base/epics-base.git base )

  EPICS_HOST_ARCH=`sh $EPICS_BASE/startup/EpicsHostArch`

  case "$STATIC" in
  static)
    cat << EOF >> "$EPICS_BASE/configure/CONFIG_SITE"
SHARED_LIBRARIES=NO
STATIC_BUILD=YES
EOF
    ;;
  *) ;;
  esac

  case "$CMPLR" in
  clang)
    echo "Host compiler is clang"
    cat << EOF >> "$EPICS_BASE/configure/os/CONFIG_SITE.Common.$EPICS_HOST_ARCH"
GNU         = NO
CMPLR_CLASS = clang
CC          = clang
CCC         = clang++
EOF

    # hack
    sed -i -e 's/CMPLR_CLASS = gcc/CMPLR_CLASS = clang/' "$EPICS_BASE/configure/CONFIG.gnuCommon"

    clang --version
    ;;
  *)
    echo "Host compiler is default"
    gcc --version
    ;;
  esac

  make -C "$EPICS_BASE" -j2

  case "$BASE" in
  *3.14*)
    ( cd "$CDIR" && wget https://www.aps.anl.gov/epics/download/extensions/extensionsTop_20120904.tar.gz && tar -xzf extensionsTop_*.tar.gz)

    ( cd "$CDIR" && wget https://www.aps.anl.gov/epics/download/extensions/msi1-7.tar.gz && tar -xzf msi1-7.tar.gz && mv msi1-7 extensions/src/msi)

    cat << EOF > "$CDIR/extensions/configure/RELEASE"
EPICS_BASE=$EPICS_BASE
EPICS_EXTENSIONS=\$(TOP)
EOF

    ( cd "$CDIR/extensions" && make )

    cp "$CDIR/extensions/bin/$EPICS_HOST_ARCH/msi" "$EPICS_BASE/bin/$EPICS_HOST_ARCH/"

    echo 'MSI:=$(EPICS_BASE)/bin/$(EPICS_HOST_ARCH)/msi' >> "$EPICS_BASE/configure/CONFIG_SITE"
    ;;
  *) ;;
  esac

  touch "$CDIR/built"
fi

EPICS_HOST_ARCH=`sh $EPICS_BASE/startup/EpicsHostArch`

CDIR="$HOME/.cache/devlib2"

if [ ! -e "$CDIR/built" ]
then
  install -d "$CDIR"

  ( cd "$CDIR" && git clone --depth 50 --branch master https://github.com/epics-modules/devlib2.git devlib2 )

  DEVLIB2="$CDIR/devlib2"

  echo "EPICS_BASE=$EPICS_BASE" > "$DEVLIB2/configure/RELEASE.local"

  make -C "$DEVLIB2" -j2

  touch "$CDIR/built"
fi

cat << EOF > configure/RELEASE
DEVLIB2=$DEVLIB2
EPICS_BASE=$EPICS_BASE
EOF
