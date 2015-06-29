#!/bin/sh
set -e

TAG=$1

exec git archive --prefix=mrfioc2-${1}/ --format tar.gz -9 -o mrfioc2-${1}.tar.gz ${1}
