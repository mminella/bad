#!/bin/bash

FILE=$1

if [ -z "${FILE}" ]; then
  echo "shutdown-cluster.sh <cluster file>"
  exit 1
fi

source ${FILE}

instances=""
for i in `seq 1 $MN`; do
  declare MV="M${i}_ID"
  instances="${instances} ${!MV}"
done

aws ec2 --profile bad-project --region ${MREGION} \
  terminate-instances --instance-ids ${instances}

now=$(date +"%Y%m%d_%H%M")
mkdir -p ./.old_clusters
mv ${FILE} ./.old_clusters/${now}.${FILE}.conf

