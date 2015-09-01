#!/bin/sh

DISK_PATH=/dev/xvd

DISKS=$( ls ${DISK_PATH}[b-z] )
for d in ${DISKS}; do
  X=$( echo $d | sed -e "s/^.*\(.\)$/\1/" )
  setup_fs ${X}
done

