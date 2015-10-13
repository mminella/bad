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
SAVE=$1
FILE=$2
SIZE=$3
OP=$4
ARG1=$5
CLIENT=$#

# Args
if [ $# != 5 -a $# != 6 ]; then
  echo "runBad.sh <log path> <cluster file> <data size> <op> <arg1> [<client>]"
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

NSTART=1
if [ $CLIENT == 6 ]; then
  NSTART=2
fi

# ===================================================================
# Experiment Helper Functions

# Get start time
D=$( date )

# All nodes
all() {
  for i in `seq $NSTART $MN`; do
    declare MV="M${i}"
    ${SSH} ubuntu@${!MV} $1 &
  done
  wait
}

client() {
  ${SSH} ubuntu@${M1} $1 &
}


# ===================================================================
# Setup Experiment

# Prepare disks
all "setup_all_fs 2> /dev/null > /dev/null"

# Remove any old files
all "rm -rf ${BUCKETS_ALL} ${FILES_ALL} ~/bad.log"

# Generate data files
START=0
for i in `seq $NSTART $MN`; do
  declare MV="M${i}"
  ${SSH} ubuntu@${!MV} "gensort_all ${START} ${SIZE_B} recs" &
  START=$(( ${START} + ${SIZE_B} ))
done
wait

# Create valid config file for cluster
if [ $CLIENT == 6 ]; then
  all "cat cluster.conf | grep -e \"M[0-9]=\" | cut -d'=' -f2 \
    | sed -e \"s/\(.*\)/\1:${PORT}/\" > ${CONFFILE}"
else
  all "echo \"localhost:7777\" > ${CONFFILE}"
  all "cat cluster.conf | grep -e \"M[0-9]=\" | cut -d'=' -f2 \
    | sed -e \"s/\(.*\)/\1:${PORT}/\" >> ${CONFFILE}"
fi

# Clear buffer caches for fresh start
all "sudo clear_buffers"

echo "
===================================================
Experiment setup! (${NAME})
==================================================="


# ===================================================================
# Run Experiment & Copy Logs

# Run
client "meth4_client ${PORT}"
START=0
for i in `seq $NSTART $MN`; do
  declare MV="M${i}"
  ${SSH} ubuntu@${!MV} \
    "echo \"# ${D}\" > ~/bad.log; \
    LD_PRELOAD=/usr/local/lib/libjemalloc.so \
    meth4_node ${START} ${PORT} ${CONFFILE} ${OP} ${ARG1} ${FILES_ALL} \
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
