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
  lyx -batch -e pdflatex evg-usage.lyx
  pdflatex evg-usage
  mv evg-usage.pdf evr-usage.pdf html/
}

(cd documentation && builddoc)

git checkout gh-pages
cp -r documentation/html/* .
rm -rf documentation
git add .
git commit -m "Last updates to documentation"
git push origin gh-pages
