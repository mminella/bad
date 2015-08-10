#!/bin/bash
KEY=$( hostname )
N=2
LOG='~/bad.log'
ITERS=1
PORT=9000
MNT=/mnt/b
READER_OUT=${MNT}/out/
FILE=$1
SAVE=$2
SIZE=$3
CHUNK=$4

if [ -z "${FILE}" -o -z "${SAVE}" -o -z "${SIZE}" -o -z "${CHUNK}" ]; then
  echo "runBad.sh <cluster file> <log path> <size> <chunk>"
  exit 1
fi

# Launch
./launchBAD.rb -f ${FILE} -k ${KEY} -c ${N} 'Meth1-%d' -d bad.tar.gz

source ${FILE}

SIZE_B=$(( $SIZE * 1024 * 1024 * 1000 / 100 ))
CHUNK_B=$(( $CHUNK * 1024 * 1024 * 1000 / 100 ))

# Backend nodes
BACKENDS=""
for i in `seq 2 $MN`; do
  declare MV="M${i}"
  BACKENDS="${!MV}:${PORT} ${BACKENDS}"
done

# All nodes
all() {
  for i in `seq 1 $MN`; do
    declare MV="M${i}"
    ssh ubuntu@${!MV} $1
  done
}

# Run a command on all backends
backends() {
  for i in `seq 2 $MN`; do
    declare MV="M${i}"
    ssh ubuntu@${!MV} $1
  done
}

# Run a command on the reader
reader() {
  ssh ubuntu@${M1} "echo '$1' >> ${LOG};" \
    "meth1_client $2 ${READER_OUT} $3 ${BACKENDS} >> ${LOG}"
}

# 1 - odirect
# 2 - op
experiment() {
  for i in `seq 1 $ITERS`; do
    backends "sudo start meth1 && sudo start meth1_node ODIRECT=${1} FILE=${MNT}/recs"
    reader "# $(( $N - 1 )), ${SIZE}, ${CHUNK}, ${1}, ${2}" \
      ${CHUNK_B} ${2}
    backends "sudo clear_buffers"
    backends "sudo stop meth1"
  done
}

# Run experiment
all "setup_fs b"
backends "gensort -t16 ${SIZE_B},buf ${MNT}/recs"
experiment false read

# Create log directory
mkdir -p $SAVE

# Copy bad-node.log
for i in `seq 2 $MN`; do
  declare MV="M${i}"
  scp "ubuntu@${!MV}:~/bad-node.log" $SAVE/${i}.bad.node.log
done

# Copy bad.log
scp ubuntu@$M1:~/bad.log $SAVE/1.bad.log

# Kill cluster
./shutdown-cluster.sh ${FILE}

