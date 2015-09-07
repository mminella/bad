#!/bin/bash
SSH="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"
KEY=$( hostname )
LOG='~/bad.log'
ITERS=1
PORT=9000
READER_OUT="/mnt/b/out"
FILE=$1
SAVE=$2
SIZE=$3
CHUNK=$4
MACHINE=$5
N=$6
ODIR=$7
CMDD=$8
TARF=$9
PG=$10
SIZE_B=$( calc "$SIZE * 1024 * 1024 * 1000 / 100" )
CHUNK_B=$( calc "$CHUNK * 1024 * 1024 * 1000 / 100" )

# Args
if [ $# != 10 ]; then
  echo "runBad.sh <cluster file> <log path> <size> <chunk> <machine> <nodes>" \
    "<o_direct> <cmd> <dist_tar> <pgroup>"
  exit 1
fi

if [ $MACHINE = "i2.xlarge" ]; then
  FILES_ALL="/mnt/b/recs"
elif [ $MACHINE = "i2.2xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs"
elif [ $MACHINE = "i2.4xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs /mnt/d/recs /mnt/e/recs"
elif [ $MACHINE = "i2.8xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs /mnt/d/recs /mnt/e/recs /mnt/f/recs /mnt/g/recs /mnt/h/recs /mnt/i/recs"
fi

# Launch
./launchBAD.rb -f ${FILE} -k ${KEY} -c ${N} -n "${SAVE}-%d" -d ${TARF} -i ${MACHINE} -p ${PG}
if [ $? -ne 0 ]; then
  echo "Launching instances failed! (${FILE})"
  ./shutdown-cluster.sh ${FILE}
  exit 1
fi

echo "Machines setup! (${FILE})"

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
    backends "sudo start meth1 && sudo start meth1_node ODIRECT=${1} FILE=\"${FILES_ALL}\""
    reader "# $(( $N - 1 )), ${SIZE}, ${CHUNK}, ${1}, ${2}" \
      ${CHUNK_B} ${2}
    backends "sudo clear_buffers"
    backends "sudo stop meth1"
  done
}

# Run experiment
all "setup_all_fs 2>&1 > /dev/null"
backends "gensort_all ${SIZE_B} recs"
experiment ${ODIR} ${CMDD}

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

