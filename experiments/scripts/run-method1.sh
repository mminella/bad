#!/bin/bash

# Run a B.A.D experiment on an existing cluster (likely launched with
# `launchCluster.sh`).

# ===================================================================
# Arguments

SSH="ssh -o UserKnownHostsFile=/dev/null \
  -o StrictHostKeyChecking=no -o LogLevel=ERROR"
SCP="scp -o UserKnownHostsFile=/dev/null \
  -o StrictHostKeyChecking=no -o LogLevel=ERROR"
USERNAME=ubuntu
KEY=$( hostname )
LOG='~/bad.log'
ITERS=1
PORT=9000
READER_OUT="/mnt/b/out"
SAVE=$1
FILE=$2
SIZE=$3
CHUNK=$4
CMDD=$5
NOPREP=0

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

if [ $M2_TYPE = "i2.xlarge" ]; then
  FILES_ALL="/mnt/b/recs"
elif [ $M2_TYPE = "i2.2xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs"
elif [ $M2_TYPE = "i2.4xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs /mnt/d/recs /mnt/e/recs"
elif [ $M2_TYPE = "i2.8xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs /mnt/d/recs /mnt/e/recs \
    /mnt/f/recs /mnt/g/recs /mnt/h/recs /mnt/i/recs"
fi

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
  ${SSH} ${USERNAME}@${M1} "echo \"# ${D}\" >> ${LOG}; echo '$1' >> ${LOG};" \
    "meth1_client $2 ${READER_OUT} $3 ${BACKENDS} 2>> ${LOG} >> ${LOG}"
}

# Run an experiment (start + run + stop method)
# 1 - op
experiment() {
  for i in `seq 1 $ITERS`; do
    backends "sudo start meth1 && sudo start meth1_node NOW=\"${D}\" FILE=\"${FILES_ALL}\""
    reader "# $(( $MN - 1 )), ${SIZE}, ${CHUNK}, ${1}" \
      ${CHUNK_B} ${1}
    backends "sudo stop meth1"
  done
}


# ===================================================================
# Setup Experiment

if [ ${NOPREP} == 0 ]; then
  D=$(date)
  echo "==================================================="
  echo "Setting up experiment: ${D}"

  # Prepare disks
  all "setup_all_fs 2> /dev/null > /dev/null"

  # Generate data files
  START=0
  for i in `seq 2 $MN`; do
    declare MV="M${i}"
    ${SSH} ${USERNAME}@${!MV} "gensort_all ${START} ${SIZE_B} recs" &
    START=$(( ${START} + ${SIZE_B} ))
  done
  wait
  all "sudo clear_buffers"

  echo "Experiment setup! (${NAME})
  ===================================================
  "
fi


# ===================================================================
# Run Experiment & Copy Logs

D=$(date)
echo "==================================================="
echo "Running experiment: ${D}"

# Run experiment
experiment ${CMDD}

# Create log directory
mkdir -p $SAVE

# Copy bad-node.log
for i in `seq 2 $MN`; do
  declare MV="M${i}"
  ${SCP} "${USERNAME}@${!MV}:~/bad-node.log" $SAVE/${i}.bad.node.log
done

# Copy bad.log
${SCP} ${USERNAME}@$M1:~/bad.log $SAVE/1.bad.log


# ===================================================================
# Finish Experiment

echo "Experiment [${FILE}] done!
===================================================
"

