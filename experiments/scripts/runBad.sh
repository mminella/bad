#!/bin/bash

# ===================================================================
# Arguments

SSH="ssh -o UserKnownHostsFile=/dev/null \
  -o StrictHostKeyChecking=no -o LogLevel=ERROR"
KEY=$( hostname )
LOG='~/bad.log'
ITERS=1
PORT=9000
READER_OUT="/mnt/b/out"
FILE=$1
SAVE=$2
SIZE=$3
CHUNK=$4
CLIENT=$5
MACHINE=$6
N=$7
CMDD=$8
TARF=$9
PG=${10}
ZONE=${11}

# Args
if [ $# != 11 ]; then
  echo "runBad.sh <cluster file> <log path> <size> <chunk> <client> <machine>
            <nodes> <cmd> <dist_tar> <pgroup> <zone>"
  exit 1
fi


# ===================================================================
# Configuration

NAME=${FILE}
SIZE_B=$( calc "round( $SIZE * 1000 * 1000 * 1000 / 100 )" )
CHUNK_B=$( calc "round( $CHUNK * 1024 * 1024 * 1024 / 100 )" )

if [ $MACHINE = "i2.xlarge" ]; then
  FILES_ALL="/mnt/b/recs"
elif [ $MACHINE = "i2.2xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs"
elif [ $MACHINE = "i2.4xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs /mnt/d/recs /mnt/e/recs"
elif [ $MACHINE = "i2.8xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs /mnt/d/recs /mnt/e/recs \
    /mnt/f/recs /mnt/g/recs /mnt/h/recs /mnt/i/recs"
fi


# ===================================================================
# Launch Cluster

# Client
./launchBAD.rb -f ${FILE} -k ${KEY} -c 1 \
  -n "${SAVE}-client" -d ${TARF} -i ${CLIENT} -p ${PG} -z ${ZONE} &
PID_CLIENT=$!

# Need to sleep to let backends view the newly created placement-group
sleep 10

# Backends
./launchBAD.rb -f ${FILE} -a -s 2 -k ${KEY} -c ${N} \
  -n "${SAVE}-%d" -d ${TARF} -i ${MACHINE} -p ${PG} -z ${ZONE}
PID_BACKENDS=$!

wait $PID_CLIENT
EXIT_CLIENT=$?
wait $PID_BACKENDS
EXIT_BACKENDS=$?

if [ ${EXIT_CLIENT} -ne 0 -o ${EXIT_BACKENDS} -ne 0 ]; then
  echo "
===================================================
Launching cluster failed! (${NAME})
==================================================="
  ./shutdown-cluster.sh ${FILE}
  exit 1
fi

echo "Machines setup! (${NAME})"
source ${FILE}


# ===================================================================
# Experiment Helper Functions

# Get start time
D=$( date )

# All nodes
ALL_NODES=""
for i in `seq 2 $MN`; do
  declare MV="M${i}"
  ALL_NODES="${!MV} ${ALL_NODES}"
done

# ./deployBAD.rb -d ${ALL_NODES}

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
  ${SSH} ubuntu@${M1} "echo \"# ${D}\" >> ${LOG}; echo '$1' >> ${LOG};" \
    "meth1_client $2 ${READER_OUT} $3 ${BACKENDS} 2>> ${LOG} >> ${LOG}"
}

# Run an experiment (start + run + stop method)
# 1 - op
experiment() {
  for i in `seq 1 $ITERS`; do
    backends "sudo start meth1 && sudo start meth1_node NOW=\"${D}\" FILE=\"${FILES_ALL}\""
    reader "# $(( $N - 1 )), ${SIZE}, ${CHUNK}, ${1}" \
      ${CHUNK_B} ${1}
    backends "sudo stop meth1"
  done
}


# ===================================================================
# Setup Experiment

# Prepare disks
all "setup_all_fs 2> /dev/null > /dev/null"

# Generate data files
START=0
for i in `seq 2 $MN`; do
  declare MV="M${i}"
  ${SSH} ubuntu@${!MV} "gensort_all ${START} ${SIZE_B} recs" &
  START=$(( ${START} + ${SIZE_B} ))
done
wait
all "sudo clear_buffers"


# ===================================================================
# Run Experiment & Copy Logs

# Run experiment
experiment ${CMDD}

# Create log directory
mkdir -p $SAVE

# Copy bad-node.log
for i in `seq 2 $MN`; do
  declare MV="M${i}"
  scp "ubuntu@${!MV}:~/bad-node.log" $SAVE/${i}.bad.node.log
done

# Copy bad.log
scp ubuntu@$M1:~/bad.log $SAVE/1.bad.log


# ===================================================================
# Finish Experiment

# Kill cluster
./shutdown-cluster.sh ${FILE}

# Done!
echo "
=========================
Test [${FILE}] done!
=========================
"

