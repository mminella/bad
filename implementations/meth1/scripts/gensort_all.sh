#!/bin/bash

SIZE=$1
FILE=$2

DISK_PATH=/dev/xvd
DISKS_N=$( ls ${DISK_PATH}[b-z] | wc -l )
DISKS=$( ls ${DISK_PATH}[b-z] )

SIZE_I=$(( $SIZE / $DISKS_N ))

RECS=""

for d in ${DISKS}; do
  X=$( echo $d | sed -e "s/^.*\(.\)$/\1/" )
  FPATH="/mnt/${X}/${FILE}"
  gensort -t4 ${SIZE_I} "${FPATH},buf"
  RECS="$RECS $FPATH"
done

echo $RECS

