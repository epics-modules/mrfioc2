#!/bin/sh
set -e

die() {
    echo "$1"
    exit 1
}

[ "$USER" ] || die "USER not set"

[ -f documentation/mainpage.h ] || die "Run me in the top level"

builddoc() {
  doxygen
  lyx -batch -e pdf evr-usage.lyx
  lyx -batch -e pdf evg-usage.lyx
  mv evg-usage.pdf evr-usage.pdf html/
}

(cd documentation && builddoc)

rsync -av --delete "$@" documentation/html/ $USER,epics@frs.sourceforge.net:/home/project-web/epics/htdocs/mrfioc2/
