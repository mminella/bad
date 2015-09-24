#!/bin/bash

# Run a B.A.D experiment on an existing cluster (likely launched with
# `launchCluster.sh`).

# ===================================================================
# Arguments

SSH="ssh -o UserKnownHostsFile=/dev/null \
  -o StrictHostKeyChecking=no -o LogLevel=ERROR"
KEY=$( hostname )
LOG='~/bad.log'
ITERS=1
PORT=9000
CONFFILE=m4.backends.conf
FILE=$1
SAVE=$2
SIZE=$3

# Args
if [ $# != 3 ]; then
  echo "runBad.sh <cluster file> <log path> <data size>"
  exit 1
fi

NAME=${FILE}
SIZE_B=$( calc "round( $SIZE * 1000 * 1000 * 1000 / 100 )" )

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
SIZE_B=$( calc "round( ${SIZE_B} / ${MN} )" )

if [ $M1_TYPE = "i2.xlarge" ]; then
  FILES_ALL="/mnt/b/recs"
elif [ $M1_TYPE = "i2.2xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs"
elif [ $M1_TYPE = "i2.4xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs /mnt/d/recs /mnt/e/recs"
elif [ $M1_TYPE = "i2.8xlarge" ]; then
  FILES_ALL="/mnt/b/recs /mnt/c/recs /mnt/d/recs /mnt/e/recs \
    /mnt/f/recs /mnt/g/recs /mnt/h/recs /mnt/i/recs"
fi

BUCKETS_ALL=$( echo ${FILES_ALL} | sed 's/recs/buckets/g' )

# ===================================================================
# Experiment Helper Functions

# Get start time
D=$( date )

# All nodes
all() {
  for i in `seq 1 $MN`; do
    declare MV="M${i}"
    ${SSH} ubuntu@${!MV} $1 &
  done
  wait
}


# ===================================================================
# Setup Experiment

# Prepare disks
all "setup_all_fs 2> /dev/null > /dev/null"

# Remove any old files
all "rm -rf ${BUCKETS_ALL} ${FILES_ALL}"

# Generate data files
START=0
for i in `seq 1 $MN`; do
  declare MV="M${i}"
  ${SSH} ubuntu@${!MV} "gensort_all ${START} ${SIZE_B} recs" &
  START=$(( ${START} + ${SIZE_B} ))
done
wait

# Clear buffer caches for fresh start
all "sudo clear_buffers"

# Create valid config file for cluster
all "cat cluster.conf | grep -e \"M[0-9]=\" | cut -d'=' -f2 \
  | sed -e \"s/\(.*\)/\1:${PORT}/\" > ${CONFFILE}"

echo "
===================================================
Experiment setup! (${NAME})
==================================================="


# ===================================================================
# Run Experiment & Copy Logs

# Run
START=0
for i in `seq 1 $MN`; do
  declare MV="M${i}"
  ${SSH} ubuntu@${!MV} "meth4_node ${START} ${PORT} ${CONFFILE} ${FILES_ALL} \
    >> ~/bad.log" &
  START=$(( ${START} + 1 ))
done
wait

# Create log directory
mkdir -p $SAVE

# Copy logs
for i in `seq 1 $MN`; do
  declare MV="M${i}"
  scp ubuntu@${!MV}:~/bad.log $SAVE/${i}.bad.log &
done
wait


# ===================================================================
# Finish Experiment

echo "
===================================================
Experiment [${FILE}] done!
===================================================
"
