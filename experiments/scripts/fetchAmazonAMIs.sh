#!/bin/bash

URL=$1
PAGE=$( curl -s $URL )

if [ -z "$URL" ]; then
  echo "Usage: http://aws.amazon.com/amazon-linux-ami/"
  echo ""
  echo "Example: ./fetchAMIHashes.sh  http://aws.amazon.com/amazon-linux-ami/"
  exit 1
fi

printCluster() {
  cluster=$1
  name=$2
  amis=$( echo "$PAGE" | grep -A4 "$name" | tail -4 | cut -d'>' -f2 | cut -d'<' -f1 )
  first=1

  echo "# $name"
  echo ":\"$cluster\" =>"
  echo -n "  %w["
  for ami in $amis; do
    if [ $first -eq 1 ]; then
      echo "$ami"
      first=0
    else
      echo "     $ami"
    fi
  done
  echo "    ],"
  echo ""
}

printCluster us-east-1 "N. Virginia"
printCluster us-west-1 "N. California"
printCluster us-west-2 Oregon
printCluster eu-west-1 "Ireland"
printCluster eu-west-2 "Frankfurt"
printCluster ap-southeast-1 Singapore
printCluster ap-southeast-2 Sydney
printCluster ap-northeast-1 Tokyo
printCluster sa-east-1 "o Paolo"


