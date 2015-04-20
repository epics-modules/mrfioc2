#!/bin/bash
set -o errexit

SYS="CSL-IFC1"
EVG="EVG0"
s_flag=0 # reset s_flag (-s is required)

usage()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -s <system name>     The system/project name"
    echo "    -g <EVG name>        Event Generator name (default: $EVG)"
    echo "    -h                   This help"
}

while getopts ":s:g:h" o; do
    case "${o}" in
        s)
            SYS=${OPTARG}
            s_flag=1
            ;;
        g)
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

# -s is not present
if [ $s_flag -eq 0 ]; then
    usage
    exit 1
fi

macro="EVG=$SYS-$EVG"
caqtdm -macro "$macro" G_EVG_VM_master.ui &
#echo caqtdm -macro "$macro" G_EVG_VME_master.ui &