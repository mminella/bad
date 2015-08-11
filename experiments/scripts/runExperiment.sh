#!/bin/bash
SSH=ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no
KEY=$( hostname )
FILE=$1
SAVE=$2
N=$3

if [ -z "${FILE}" -o -z "${SAVE}" -o -z "${N}" ]; then
  echo "runExperiment.sh <cluster file> <log path> <nodes>"
  exit 1
fi

# launch
./launchBAD.rb -f ${FILE} -k ${KEY} -c ${N} 'Measure-%d' -d measure.tar.gz

source ${FILE}

# run experiments
for i in `seq 1 $MN`; do
  declare MV="M${i}"
  ${SSH} ubuntu@${!MV} run_experiment &
done
wait

# copy logs
for i in `seq 1 $MN`; do
  declare MV="M${i}"
  mkdir -p ${SAVE}/${i}/
  scp ubuntu@${!MV}:log_* ${SAVE}/${i}/ &
done
wait

# shutdown
./shutdown-cluster.sh ${FILE}

