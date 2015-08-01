#!/bin/bash
KEY='black.davidterei.com'
N=2
LOG='~/bad.log'
ITERS=3
PORT=9000
READER_OUT=/mnt/b/out/

# launch
./launchBAD.rb -k ${KEY} -c ${N} 'Measure-%d' -d bad.tar.gz

source .cluster.conf

# setup files
M50=$(( 1024 * 1024 * 50 / 100 ))
M100=$(( 1024 * 1024 * 100 / 100 ))
G1=$(( 1024 * 1024 * 1000 / 100 ))
G10=$(( 1024 * 1024 * 1000 * 10 / 100 ))
ssh ubuntu@${M1} "setup_fs b"
ssh ubuntu@${M1} "gensort -t8 ${G10},buf /mnt/b/10g; gensort -t8 ${G1},buf /mnt/b/1g"
ssh ubuntu@${M2} "setup_fs b"

# Backend nodes
BACKENDS=""
for i in `seq 2 $MN`; do
  declare MV="M${i}"
  BACKENDS="${!MV}:${PORT} ${BACKENDS}"
done
echo $BACKENDS

# Run a command on all backends
backends() {
  for i in `seq 2 $MN`; do
    declare MV="M${i}"
    ssh ubuntu@${!MV} $1
  done
}

# Run a command on the reader
reader() {
  ssh ubuntu@${M1} "echo '$1' >> ${LOG}" \
    "meth1_client $2 ${READER_OUT} $3 ${BACKENDS} >> ${LOG}"
}

# 1GB - 100M
backends "sudo start meth1 && sudo start meth1_node ODIRECT=false FILE=/mnt/b/1g"

## first
for i in `seq 1 $ITERS`; do
  reader "# 1, 1gb, 100m, false, first" ${M100} "first"
  backends "sudo clear_buffers"
done

## read
for i in `seq 1 $ITERS`; do
  reader "# 1, 1gb, 100m, false, read" ${M100} "read"
  backends "sudo clear_buffers"
done

## write
for i in `seq 1 $ITERS`; do
  reader "# 1, 1gb, 100m, false, write" ${M100} "write"
  backends "sudo clear_buffers"
done

backends "sudo stop meth1"


# 1GB - 50M
backends "sudo start meth1 && sudo start meth1_node ODIRECT=false FILE=/mnt/b/1g"

## first
for i in `seq 1 $ITERS`; do
  reader "# 1, 1gb, 50m, false, first" ${M50} "first"
  backends "sudo clear_buffers"
done

## read
for i in `seq 1 $ITERS`; do
  reader "# 1, 1gb, 50m, false, read" ${M50} "read"
  backends "sudo clear_buffers"
done

## write
for i in `seq 1 $ITERS`; do
  reader "# 1, 1gb, 50m, false, write" ${M50} "write"
  backends "sudo clear_buffers"
done

backends "sudo stop meth1"


# 1GB - 50M - O_DIRECT
backends "sudo start meth1 && sudo start meth1_node ODIRECT=true FILE=/mnt/b/1g"

## first
for i in `seq 1 $ITERS`; do
  reader "# 1, 1gb, 50m, true, first" ${M50} "first"
  backends "sudo clear_buffers"
done

## read
for i in `seq 1 $ITERS`; do
  reader "# 1, 1gb, 50m, true, read" ${M50} "read"
  backends "sudo clear_buffers"
done

## write
for i in `seq 1 $ITERS`; do
  reader "# 1, 1gb, 50m, true, write" ${M50} "write"
  backends "sudo clear_buffers"
done

backends "sudo stop meth1"


# 10GB - 1GB
backends "sudo start meth1 && sudo start meth1_node ODIRECT=false FILE=/mnt/b/10g"

## first
for i in `seq 1 $ITERS`; do
  reader "# 1, 10gb, 1gb, false, read" ${G1} "first"
  backends "sudo clear_buffers"
done

backends "sudo stop meth1"
backends "sudo start meth1 && sudo start meth1_node ODIRECT=false FILE=/mnt/b/10g"

## read
for i in `seq 1 $ITERS`; do
  reader "# 1, 10gb, 1gb, false, read" ${G1} "read"
  backends "sudo clear_buffers"
done

backends "sudo stop meth1"
backends "sudo start meth1 && sudo start meth1_node ODIRECT=false FILE=/mnt/b/10g"

## write
for i in `seq 1 $ITERS`; do
  reader "# 1, 10gb, 1gb, false, read" ${G1} "write"
  backends "sudo clear_buffers"
done

backends "sudo stop meth1"

