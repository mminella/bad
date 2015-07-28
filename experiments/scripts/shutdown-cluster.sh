#!/bin/sh

source .cluster.conf

instances=""
for i in `seq 1 $MN`; do
  declare MV="M${i}_ID"
  instances="${instances} ${!MV}"
done

aws ec2 --profile bad-project terminate-instances --instance-ids ${instances}

