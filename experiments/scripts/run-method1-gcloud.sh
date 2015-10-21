#!/bin/bash

# Run a B.A.D experiment on an existing cluster (likely launched with
# `launchCluster.sh`).

# ===================================================================
# Arguments

SSH="ssh -i /home/quhang/.ssh/google_compute_engine -o UserKnownHostsFile=/dev/null \
  -o StrictHostKeyChecking=no -o LogLevel=ERROR"
SCP="scp -i /home/quhang/.ssh/google_compute_engine -o UserKnownHostsFile=/dev/null \
  -o StrictHostKeyChecking=no -o LogLevel=ERROR"
KEY=$( hostname )
LOG='~/bad.log'
ITERS=1
PORT=9000
READER_OUT="~/data/out"
SAVE=$1
FILE=$2
SIZE=$3
CHUNK=$4
CMDD=$5
NOPREP=0
SORT_INPUT="~/data/recs"
USER_NAME="quhang"

# Args
if [ $# != 5 -a $# != 6 ]; then
  echo "Usage: <log path> <cluster file> <cluster data size> <chunk> <cmd>" \
    "[<no prep>]"
  exit 1
fi

if [ $# == 6 ]; then
  NOPREP=1
fi

NAME=${FILE}
SIZE_B=$( calc "round( $SIZE * 1000 * 1000 * 1000 / 100 )" )
CHUNK_B=$( calc "round( $CHUNK * 1024 * 1024 * 1024 / 100 )" )

# Validate
if [ ! -f ${FILE} ]; then
  echo "Cluster config (${FILE}) not found!"
  exit 1
elif [ ! ${SIZE_B} -gt 0 ]; then
  echo "Invalid data set size!"
  exit 1
fi


# ===================================================================
# Configuration

source ${FILE}

# per-node data size
SIZE_B=$( calc "round( ${SIZE_B} / ( ${MN} - 1 ) )" )


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
    ${SSH} ${USER_NAME}@${!MV} $1
  done
}

# Run a command on all backends
backends() {
  for i in `seq 2 $MN`; do
    declare MV="M${i}"
    ${SSH} ${USER_NAME}@${!MV} $1
  done
}

# Run a command on the reader
# 1 - header
# 2 - chunk size
# 3 - op
reader() {
  ${SSH} ${USER_NAME}@${M1} "echo \"# ${D}\" >> ${LOG}; echo '$1' >> ${LOG};" \
    "~/bad/implementations/meth1/app/meth1_client $2 ${READER_OUT} $3 ${BACKENDS} 2>> ${LOG} >> ${LOG}"
}

# Run an experiment (start + run + stop method)
# 1 - op
experiment() {
  all "rm -f ${LOG}"
  backends "killall meth1_node"
  all "killall methd1_client"
  for i in `seq 1 $ITERS`; do
    backends "LD_PRELOAD=/usr/local/lib/libjemalloc.so ~/bad/implementations/meth1/app/meth1_node ${PORT} ${SORT_INPUT} 2>> ${LOG} >> ${LOG} &"
    reader "# $(( $MN - 1 )), ${SIZE}, ${CHUNK}, ${1}" \
      ${CHUNK_B} ${1}
    backends "killall meth1_node"
  done
}


# ===================================================================
# Setup Experiment

if [ ${NOPREP} == 0 ]; then
  # Prepare disks
  all "mkdir -p ~/data"
  all "sudo /usr/share/google/safe_format_and_mount -m \"mkfs.ext4 -F\" /dev/disk/by-id/google-local-ssd-0 ~/data"
  all "sudo chmod a+w ~/data"

  # Generate data files
  START=0
  for i in `seq 2 $MN`; do
    declare MV="M${i}"
    ${SSH} ${USER_NAME}@${!MV} "rm -f ${SORT_INPUT}; ~/bad/gensort/gensort -b${START} -t4 ${SIZE_B} ${SORT_INPUT},buf" &
    START=$(( ${START} + ${SIZE_B} ))
  done
  wait
  all "sudo ~/bad/implementations/meth1/scripts/clear_buffers.sh"

  echo "
  ===================================================
  Experiment setup! (${NAME})
  ==================================================="
fi


# ===================================================================
# Run Experiment & Copy Logs

# Run experiment
experiment ${CMDD}

# Create log directory
mkdir -p $SAVE

# Copy bad-node.log
for i in `seq 2 $MN`; do
  declare MV="M${i}"
  ${SCP} "${USER_NAME}@${!MV}:~/bad.log" $SAVE/${i}.bad.node.log
done

# Copy bad.log
${SCP} ${USER_NAME}@$M1:~/bad.log $SAVE/1.bad.log


# ===================================================================
# Finish Experiment

echo "
===================================================
Experiment [${FILE}] done!
===================================================
"

