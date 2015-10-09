#!/bin/bash

# Launch a B.A.D cluster, a set of backends and a single client.
# If launching fails, exit code of 1 is returned but the machines are left up
# in a partial state.

# ===================================================================
# Arguments

# We take a bunch of unused arguments to keep the interface between
# launch-client-and-backend.sh and run-method1.sh the same.
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

# ===================================================================
# Launch Cluster

# Client
./launch-backends.rb -f ${FILE} -k ${KEY} -c 1 \
  -n "${SAVE}-client" -d ${TARF} -i ${CLIENT} -p ${PG} -z ${ZONE} &
PID_CLIENT=$!

# Need to sleep to let backends view the newly created placement-group
sleep 10

# Backends
./launch-backends.rb -f ${FILE} -a -s 2 -k ${KEY} -c ${N} \
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
  exit 1
fi

echo "
===================================================
Machines setup! (${NAME})
==================================================="

