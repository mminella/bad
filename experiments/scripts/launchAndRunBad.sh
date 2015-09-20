#!/bin/bash
FILE=$1

# Launch cluster
./launchCluster.sh $1 $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11}

# Run experiment
./runBad.sh $1 $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11}

# Kill cluster
./shutdown-cluster.sh ${FILE}
