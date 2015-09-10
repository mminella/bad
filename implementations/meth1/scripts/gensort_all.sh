#!/bin/bash

START=$1
SIZE=$2
FILE=$3

DISK_PATH=/dev/xvd
DISKS_N=$( ls ${DISK_PATH}[b-z] | wc -l )
DISKS=$( ls ${DISK_PATH}[b-z] )

SIZE_I=$(( $SIZE / $DISKS_N ))
STEP_I=${SIZE_I}

for d in ${DISKS}; do
  X=$( echo $d | sed -e "s/^.*\(.\)$/\1/" )
  FPATH="/mnt/${X}/${FILE}"
  gensort -b${START} -t4 ${SIZE_I} "${FPATH},buf"
  START=$(( ${START} + ${STEP_I} ))
done

