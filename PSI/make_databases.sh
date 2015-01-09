#!/bin/bash
set -o errexit

OUTPUT_DIR="./db"
MRFIOC2_DIR=".."
DB_LIST=(evgSoftSeq.db evg-vme.db evrEvent.db evr-vmerf230.db)
DB_REL_PATH="Db/PSI"

usage()
{
    echo "Usage: ./$0 [options]"
    echo "Options:"
    echo "    -t <top dir> (default ..)"
    echo "    -o <output dir> (default ./db)"
}

while getopts ":t:o:" o; do
    case "${o}" in
        t)
            MRFIOC2_DIR=${OPTARG}
            ;;
        o)
            OUTPUT_DIR=${OPTARG}
            ;;
        *)
            usage
            exit 0
            ;;
    esac
done
shift $((OPTIND-1))

if [ ! -d "$MRFIOC2_DIR/evrMrmApp" ]; then
    usage
    exit 1
fi

for arg in $@; do
    DB_LIST+=("$arg")
done

mkdir -p $OUTPUT_DIR
for db in ${DB_LIST[@]}; do
    db_name=`basename "$db" | cut -d. -f1`
    db_file=$(find $MRFIOC2_DIR/*MrmApp/$DB_REL_PATH -name $db_name.db 2>/dev/null)
    if [ ! -z $db_file ]; then
        cp $db_file $OUTPUT_DIR
    else
        sub_file=$(find $MRFIOC2_DIR/*MrmApp/$DB_REL_PATH -name $db_name.substitutions 2>/dev/null)
        if [ ! -z $sub_file ]; then
            msi -I$(dirname $sub_file) -S$sub_file > "$OUTPUT_DIR/$db_name.db"
	else
            echo "$db not found"
	fi
    fi
done
