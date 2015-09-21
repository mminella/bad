#!/bin/bash

URL=$1
PAGE=$( curl -s $URL )

if [ -z "$URL" ]; then
  echo "Usage: <Ubuntu Release URL>"
  echo ""
  echo "See http://cloud-images.ubuntu.com/releases/14.04/"
  echo ""
  echo "Example: ./fetchAMIHashes.sh  http://cloud-images.ubuntu.com/releases/14.04/release-20150305/"
  exit 1
fi

parsePage() {
  cluster=$1
  arch=$2
  mtype=$3

  echo "$PAGE" | grep -A4 $cluster | grep -A2 $arch | grep -A1 "\s$mtype\s" | tail -1 | sed -n 's/.*\(ami-[0-9a-f]*\).*/\1/p'
}

printCluster() {
  cluster=$1
  echo ":\"$cluster\" =>"
  echo -n "  %w["
  parsePage $cluster 64-bit hvm-ssd
  echo -n "     "
  parsePage $cluster 64-bit ebs
  echo -n "     "
  parsePage $cluster 64-bit instance
  echo "    ],"
  echo ""
}

printCluster ap-northeast-1
printCluster ap-southeast-1
printCluster ap-southeast-2
printCluster eu-west-1
printCluster sa-east-1
printCluster us-east-1
printCluster us-west-1
printCluster us-west-2

