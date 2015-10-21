#!/bin/bash

# Shutdown a B.A.D cluster.

SCP="scp -o UserKnownHostsFile=/dev/null \
  -o StrictHostKeyChecking=no -o LogLevel=ERROR"
FILE=$1
IN=$2
OUT=$3
shift

if [ -z "${FILE}" -o -z "${IN}" -o -z "${OUT}" ]; then
  echo "cp.sh <cluster file> <local file> <remote file>"
  exit 1
fi

source ${FILE}

for i in `seq 1 $MN`; do
  declare MV="M${i}"
  ${SCP} ${IN} ubuntu@${!MV}:${OUT} &
done
wait

