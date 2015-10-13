#!/bin/bash

# A standalone, automated experiment runner for B.A.D.
# It will:
# 1) Launch a new cluster (and kill it if launching fails)
# 2) Run the experiment
# 3) Shutdown the cluster

SAVE=$1
FILE=$2
SIZE=$3
CHUNK=$4
CLIENT=$5
MACHINE=$6
N=$7
CMDD=$8
TARF=$9
PG=${10}
ZONE=${11}

# Launch cluster
./launch-client-and-backends.sh ${SAVE} ${FILE} ${CLIENT} ${MACHINE} ${N} \
    ${TARF} ${PG} ${ZONE}
if [ $? -ne 0 ]; then
  ./shutdown-cluster.sh ${FILE}
fi

# Run experiment
./run-method1.sh ${SAVE} ${FILE} ${SIZE} ${CHUNK} ${CMDD}

# Kill cluster
./shutdown-cluster.sh ${FILE}
