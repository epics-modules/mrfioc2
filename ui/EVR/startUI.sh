#!/bin/bash
set -o errexit

SYS="MTEST-VME-BSREAD"
EVR="EVR0"

usage()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -s <system name>     The system/project name"
    echo "    -r <EVR name>        Event Receiver name (default: $EVR)"
    echo "    -h                   This help"
}

while getopts ":s:r:h" o; do
    case "${o}" in
        s)
            SYS=${OPTARG}
            ;;
        r)
            EVR=${OPTARG}
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

if [ $OPTIND -le 1 ]; then
    usage
    exit 1
fi

macro="EVR=$SYS-$EVR"
caqtdm -macro "$macro" evr_master.ui &
#echo caqtdm -macro "$macro" evr_master.ui &