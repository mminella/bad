#!/bin/bash
SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"
KEY=$( hostname )
LOG='~/bad.log'
ITERS=1
PORT=9000
MNT=/mnt/b
READER_OUT=${MNT}/out/
FILE=$1
SAVE=$2
SIZE=$3
CHUNK=$4
MACHINE=$5
N=$6
ODIR=$7
CMDD=$8
SIZE_B=$( calc "$SIZE * 1024 * 1024 * 1000 / 100" )
CHUNK_B=$( calc "$CHUNK * 1024 * 1024 * 1000 / 100" )

# Args
if [ $# != 8 ]; then
  echo "runBad.sh <cluster file> <log path> <size> <chunk> <machine> <nodes> <o_direct> <cmd>"
  exit 1
fi

# Launch
./launchBAD.rb -f ${FILE} -k ${KEY} -c ${N} -n 'Meth1-%d' -d bad.tar.gz -i ${MACHINE}

source ${FILE}

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
    ${SSH} ubuntu@${!MV} $1
  done
}

# Run a command on all backends
backends() {
  for i in `seq 2 $MN`; do
    declare MV="M${i}"
    ${SSH} ubuntu@${!MV} $1
  done
}

# Run a command on the reader
# 1 - header
# 2 - chunk size
# 3 - op
reader() {
  ${SSH} ubuntu@${M1} "echo '$1' >> ${LOG};" \
    "meth1_client $2 ${READER_OUT} $3 ${BACKENDS} 2>&1 >> ${LOG}"
}

# Run an experiment (start + run + stop method)
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
all "setup_all_fs"
backends "gensort -t16 ${SIZE_B},buf ${MNT}/recs"
experiment ${ODIR} ${CMD}

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

