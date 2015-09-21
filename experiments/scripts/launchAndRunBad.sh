#!/bin/bash

# A standalone, automated experiment runner for B.A.D.
# It will:
# 1) Launch a new cluster (and kill it if launching fails)
# 2) Run the experiment
# 3) Shutdown the cluster

FILE=$1

# Launch cluster
./launchCluster.sh $1 $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11}
if [ $? -ne 0 ]; then
  ./shutdown-cluster.sh ${FILE}
fi

# Run experiment
./runBad.sh $1 $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11}

# Kill cluster
./shutdown-cluster.sh ${FILE}
