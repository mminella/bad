#!/bin/bash

# 1KB = 1024
# 1MB = 1024*1KB
# 1GB = 1000*1MB

ITERS="A B C"
BLOCKS="512 4096 1048576"
QUEUES="1 15 31 62"
READ_SIZE=524288000 # 500MB
WRITE_SIZE=104857600 # 100MB
DISK_PATH=/dev/xvd

# What disks to test?
DISKS=$(ls ${DISK_PATH}[a-z] | grep -v "a$")
MNTS=""
FILES=""
for d in ${DISKS}; do
  X=$( echo $d | sed -e "s/^.*\(.\)$/\1/" )
  MNTS="${MNTS} /mnt/${X}"
  FILES="/mnt/${X}/file ${FILES}"
done

# DD
for b in ${BLOCKS}; do
  COUNT=$(( $WRITE_SIZE / $b ))
  test_dd.py --count ${COUNT} --block $b ${DISKS}
done

# SETUP FS
for d in ${DISKS}; do
  X=$( echo $d | sed -e "s/^.*\(.\)$/\1/" )
  setup_fs $X
done

# SYNC
for f in ${FILES}; do
  dd if=/dev/zero of=${f} bs=1048576 count=$(( $READ_SIZE / 1048576 )) \
    conv=fdatasync,notrunc
done

test_sync() {
  BLOCK=$1
  COUNT=$(( ${READ_SIZE} / ${BLOCK} ))
  DIRECT=$2
  OD="F"
  if [ -n "${DIRECT}" ]; then
    OD="T"
  fi
  for i in ${ITERS}; do
    test_sync.py --count ${COUNT} --block ${BLOCK} ${DIRECT} ${FILES}
  done
}

for b in ${BLOCKS}; do
  test_sync $b
done

for b in ${BLOCKS}; do
  test_sync $b "--odirect"
done

# ASYNC
for f in ${FILES}; do
  dd if=/dev/zero of=${FILES} bs=1048576 count=$(( $WRITE_SIZE / 1048576 )) \
    conv=fdatasync,notrunc
done

test_async() {
  BLOCK=$1
  QDEPTH=$2
  DIRECT=$3
  OD="F"
  if [ -n "${DIRECT}" ]; then
    OD="T"
  fi
  for i in ${ITERS}; do
    test_async.py --block ${BLOCK} --qdepth ${QDEPTH} ${DIRECT} ${FILES}
  done
}

for b in ${BLOCKS}; do
  for q in ${QUEUES}; do
    test_async $b $q
  done
done

for b in ${BLOCKS}; do
  for q in ${QUEUES}; do
    test_async $b $q "--odirect"
  done
done

