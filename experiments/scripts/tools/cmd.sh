#!/bin/bash

# Shutdown a B.A.D cluster.

SSH="ssh -o UserKnownHostsFile=/dev/null \
  -o StrictHostKeyChecking=no -o LogLevel=ERROR"
FILE=$1
shift 1

if [ -z "${FILE}" ]; then
  echo "cmd.sh <cluster file> <cmd...>"
  exit 1
fi

source ${FILE}

for i in `seq 1 $MN`; do
  declare MV="M${i}"
  ${SSH} ubuntu@${!MV} $@ &
done
wait

