#!/bin/bash

# launch
./launchBAD.rb -k $1 -c $2 'Measure-%d' -d measure.tar.gz

source .cluster.conf

# run experiments
for i in `seq 1 $MN`; do
  declare MV="M${i}"
  ssh ubuntu@${!MV} run_experiment &
done
wait

# copy logs
for i in `seq 1 $MN`; do
  declare MV="M${i}"
  mkdir -p logs/${i}/
  scp ubuntu@${!MV}:log_* logs/${i}/ &
done
wait

# shutdown
./shutdown-cluster.sh

