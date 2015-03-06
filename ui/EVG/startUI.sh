#!/bin/bash
set -o errexit

SYS="CSL-IFC1"
EVG="EVG0"

usage()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -s <system name>     The system/project name"
    echo "    -g <EVG name>        Event Generator name (default: $EVG)"
    echo "    -h                   This help"
}

while getopts ":s:r:h" o; do
    case "${o}" in
        s)
            SYS=${OPTARG}
            ;;
        r)
            EVG=${OPTARG}
            ;;
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

#if [ $OPTIND -le 1 ]; then
#    usage
#    exit 1
#fi

macro="EVG=$SYS-$EVG"
caqtdm -macro "$macro" evg_master.ui &
#echo caqtdm -macro "$macro" EVG_master.ui &