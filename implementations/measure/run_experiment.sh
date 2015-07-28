#!/bin/bash

# 1KB = 1024
# 1MB = 1024*1KB
# 1GB = 1000*1MB

ITERS="A B C"
BLOCKS="512 4096 1048576"
QUEUES="1 15 31 62"
READ_SIZE=1048576000 # 1GB

FILES=$@

# SYNC
for f in ${FILES}; do
  dd if=/dev/zero of=${FILES} bs=1048576 count=1000 conv=fdatasync,notrunc
done

test_sync() {
  BLOCK=$1
  COUNT=$(( ${READ_SIZE} / ${BLOCK} ))
  DIRECT=$2
  NAME=S_${BLOCK}
  OD="F"
  if [ -n "${DIRECT}" ]; then
    OD="T"
  fi
  for i in ${ITERS}; do
    test_sync.py --name ${NAME}.${OD}.${i} --block ${BLOCK} ${DIRECT} ${FILES}
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
  dd if=/dev/zero of=${FILES} bs=1048576 count=100 conv=fdatasync,notrunc
done

test_async() {
  BLOCK=$1
  QDEPTH=$2
  DIRECT=$3
  NAME=A_${BLOCK}_${QDEPTH}
  OD="F"
  if [ -n "${DIRECT}" ]; then
    OD="T"
  fi
  for i in ${ITERS}; do
    test_async.py --name "${NAME}.${OD}.${i}" --block ${BLOCK} --qdepth ${QDEPTH} ${DIRECT} ${FILES}
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

