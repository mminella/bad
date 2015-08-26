#!/bin/bash

# 1KB = 1024
# 1MB = 1024*1KB
# 1GB = 1000*1MB

ITERS="A B"
SYNC_BLOCKS="1048576 10485760" # 1MB, 10MB
ASYNC_BLOCKS="1048576 10485760" # 1MB, 10MB
QUEUES="1 15 31 62"
READ_SIZE=524288000 # 500MB
WRITE_SIZE=1048576000 # 1000MB
DISK_PATH=/dev/xvd
CPU_LOG=$HOME/log_cpu.txt

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
for b in ${SYNC_BLOCKS}; do
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

for b in ${SYNC_BLOCKS}; do
  test_sync $b
done

for b in ${SYNC_BLOCKS}; do
  test_sync $b "--odirect"
done

# ASYNC
for f in ${FILES}; do
  dd if=/dev/zero of=${f} bs=1048576 count=$(( $WRITE_SIZE / 1048576 )) \
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

for b in ${ASYNC_BLOCKS}; do
  for q in ${QUEUES}; do
    test_async $b $q
  done
done

for b in ${ASYNC_BLOCKS}; do
  for q in ${QUEUES}; do
    test_async $b $q "--odirect"
  done
done

# CPU
echo "allocator, libc" >> $CPU_LOG
echo "# memcpy" >> $CPU_LOG
memcpy_speed >> $CPU_LOG
echo "# memcmp" >> $CPU_LOG
memcmp_speed >> $CPU_LOG
echo "# malloc" >> $CPU_LOG
NPROC=$( nproc )
for i in `seq 1 $NPROC`; do
  memalloc_speed $i >> $CPU_LOG
done

export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so
echo "allocator, jemalloc" >> $CPU_LOG
echo "# memcpy" >> $CPU_LOG
memcpy_speed >> $CPU_LOG
echo "# memcmp" >> $CPU_LOG
memcmp_speed >> $CPU_LOG
echo "# malloc" >> $CPU_LOG
NPROC=$( nproc )
for i in `seq 1 $NPROC`; do
  memalloc_speed $i >> $CPU_LOG
done

export LD_PRELOAD=/usr/lib/libtcmalloc.so
echo "allocator, tcmalloc" >> $CPU_LOG
echo "# memcpy" >> $CPU_LOG
memcpy_speed >> $CPU_LOG
echo "# memcmp" >> $CPU_LOG
memcmp_speed >> $CPU_LOG
echo "# malloc" >> $CPU_LOG
NPROC=$( nproc )
for i in `seq 1 $NPROC`; do
  memalloc_speed $i >> $CPU_LOG
done

export LD_PRELOAD=/usr/lib/libtbbmalloc_proxy.so
echo "allocator, tbb" >> $CPU_LOG
echo "# memcpy" >> $CPU_LOG
memcpy_speed >> $CPU_LOG
echo "# memcmp" >> $CPU_LOG
memcmp_speed >> $CPU_LOG
echo "# malloc" >> $CPU_LOG
NPROC=$( nproc )
for i in `seq 1 $NPROC`; do
  memalloc_speed $i >> $CPU_LOG
done

