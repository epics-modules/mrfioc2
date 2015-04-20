#!/bin/bash
set -o errexit

usage()
{
    echo "Usage: $0 [options] SCREENS_DIR"
  	echo
    echo "Copy all screens (*.ui) from the SCREENS_DIR directory to the"
    echo "\"\$(INSTBASE)/config/qt\" directory. Also copy all scripts (*sh)"
    echo "from SCREENS_DIR directory to the \"\$(INSTBASE)/bin\" directory."
    echo
    echo "Options:"
    echo "    -h                   This help"
}

while getopts ":h" o; do
    case "${o}" in
        h)
            usage
            exit 0
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done

# First argument must be the source dir of the screens and scripts.
if [ "$#" -ne 1 ]||[ ! -d "$1" ]; then # invalid number of args or not a dir
    usage
    exit 1
fi

# destination directory for screens
qt_dir=${INSTBASE}/config/qt
sh_dir=${INSTBASE}/bin

cp -v "$1"/*.ui "$qt_dir"
cp -v "$1"/*.sh "$sh_dir"
#echo cp "$1"/*.ui "$qt_dir"
#echo cp "$1"/*.sh "$sh_dir"