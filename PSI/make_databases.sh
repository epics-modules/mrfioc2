#!/bin/bash
set -o errexit

OUTPUT_DIR="./db"
MRFIOC2_DIR=".."
DB_LIST=(evgSoftSeq.template evg-vme evr-pulserMap.template evr-softEvent.template evr-specialFunctionMap.template evr-delayModule.template evr-vmerf230 evr-pcie-300 evr-cpci-230)
DB_REL_PATH="Db/PSI"

# Verbose output
VO=0

usage()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -t <top dir>       Top folder of mrfioc2 (default: $MRFIOC2_DIR)"
    echo "    -o <output dir>    Output dir for databases (default: $OUTPUT_DIR)"
    echo "    -v                 Verbose output (default: no output)"
    echo "    -h                 This help"
}

while getopts ":t:ovh" o; do
    case "${o}" in
        t)
            MRFIOC2_DIR=${OPTARG}
            ;;
        o)
            OUTPUT_DIR=${OPTARG}
            ;;
        v)
            VO=1
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
shift $((OPTIND-1))

if [ ! -d "$MRFIOC2_DIR/evrMrmApp" ]; then
    >&2 echo "ERROR: MRFIOC2 top folder ($MRFIOC2_DIR) not correct."
    usage
    exit 1
fi

exec 3>&1 4>&2
if [ $VO -eq 0 ]; then
    exec &>/dev/null
fi

mkdir -p $OUTPUT_DIR
for db in ${DB_LIST[@]}; do
    db_name=`basename "$db" | cut -d. -f1`
    sub_file=$(find $MRFIOC2_DIR/*MrmApp/$DB_REL_PATH -name $db_name.substitutions 2>/dev/null)
    if [ ! -z $sub_file ]; then
        msi -I$(dirname $sub_file) -S$sub_file > "$OUTPUT_DIR/$db_name.template"
        echo "EXPANDED: $sub_file"
    else
        db_file=$(find $MRFIOC2_DIR/*MrmApp/$DB_REL_PATH -name $db_name.template 2>/dev/null)
        if [ ! -z db_file ]; then
            cp $db_file $OUTPUT_DIR
            echo "COPIED: $db_file"
       else
            >&4 echo "ERROR $db not found!"
       fi
    fi
done
exec 1>&3 2>&4

echo "DONE: databases expanded and copied to: $OUTPUT_DIR"
