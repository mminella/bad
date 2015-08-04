#!/bin/bash
KEY='black.davidterei.com'
N=3
LOG='~/bad.log'
ITERS=1
PORT=9000
MNT=/mnt/b
READER_OUT=${MNT}/out/
FILE=$1
SAVE=$2

if [ -z "${FILE}" -o -z "${SAVE}" ]; then
  echo "runBad.sh <cluster file> <log path>"
  exit 1
fi

# Launch
./launchBAD.rb -f ${FILE} -k ${KEY} -c ${N} 'Meth1-%d' -d bad.tar.gz

source ${FILE}

# Some size constants
M01=$(( 1024 * 1024 * 1 / 100 ))
M25=$(( 1024 * 1024 * 25 / 100 ))
M50=$(( 1024 * 1024 * 50 / 100 ))
M75=$(( 1024 * 1024 * 75 / 100 ))

M100=$(( 1024 * 1024 * 100 / 100 ))
M150=$(( 1024 * 1024 * 150 / 100 ))
M200=$(( 1024 * 1024 * 200 / 100 ))
M250=$(( 1024 * 1024 * 250 / 100 ))

M300=$(( 1024 * 1024 * 300 / 100 ))
M400=$(( 1024 * 1024 * 400 / 100 ))
M500=$(( 1024 * 1024 * 500 / 100 ))
M750=$(( 1024 * 1024 * 750 / 100 ))

G1=$(( 1024 * 1024 * 1000 * 1 / 100 ))
G2=$(( 1024 * 1024 * 1000 * 2 / 100 ))
G3=$(( 1024 * 1024 * 1000 * 3 / 100 ))
G4=$(( 1024 * 1024 * 1000 * 4 / 100 ))
G5=$(( 1024 * 1024 * 1000 * 5 / 100 ))
G6=$(( 1024 * 1024 * 1000 * 6 / 100 ))
G7=$(( 1024 * 1024 * 1000 * 7 / 100 ))
G8=$(( 1024 * 1024 * 1000 * 8 / 100 ))
G9=$(( 1024 * 1024 * 1000 * 9 / 100 ))

G10=$(( 1024 * 1024 * 1000 * 10 / 100 ))
G20=$(( 1024 * 1024 * 1000 * 20 / 100 ))
G30=$(( 1024 * 1024 * 1000 * 30 / 100 ))
G40=$(( 1024 * 1024 * 1000 * 40 / 100 ))
G50=$(( 1024 * 1024 * 1000 * 50 / 100 ))
G60=$(( 1024 * 1024 * 1000 * 60 / 100 ))
G70=$(( 1024 * 1024 * 1000 * 70 / 100 ))
G80=$(( 1024 * 1024 * 1000 * 80 / 100 ))
G90=$(( 1024 * 1024 * 1000 * 90 / 100 ))

G100=$(( 1024 * 1024 * 1000 * 100 / 100 ))
G200=$(( 1024 * 1024 * 1000 * 200 / 100 ))
G300=$(( 1024 * 1024 * 1000 * 300 / 100 ))
G400=$(( 1024 * 1024 * 1000 * 400 / 100 ))
G500=$(( 1024 * 1024 * 1000 * 500 / 100 ))

# Backend nodes
BACKENDS=""
for i in `seq 2 $MN`; do
  declare MV="M${i}"
  BACKENDS="${!MV}:${PORT} ${BACKENDS}"
done

# All nodes
all() {
  for i in `seq 1 $MN`; do
    declare MV="M${i}"
    ssh ubuntu@${!MV} $1
  done
}

# Run a command on all backends
backends() {
  for i in `seq 2 $MN`; do
    declare MV="M${i}"
    ssh ubuntu@${!MV} $1
  done
}

# Run a command on the reader
reader() {
  ssh ubuntu@${M1} "echo '$1' >> ${LOG};" \
    "meth1_client $2 ${READER_OUT} $3 ${BACKENDS} >> ${LOG}"
}

# 1 - odirect?
# 2 - file size? 1g 10g
# 3 - op?
# 4 - read-ahead
experiment() {
  for i in `seq 1 $ITERS`; do
    backends "sudo start meth1 && sudo start meth1_node ODIRECT=${1} FILE=${MNT}/${2}"
    reader "# $(( $N - 1 )), ${2}, ${4}, ${1}, ${3}" ${4} "${3}"
      backends "sudo clear_buffers"
    backends "sudo stop meth1"
  done
}

all "setup_fs b"

# 100G -- chunk-size
backends "gensort -t16 ${G100},buf ${MNT}/100g"
experiment false 100g first         ${M01}
experiment false 100g chunk-${M01}  ${M01}
experiment false 100g chunk-${M100} ${M100}
experiment false 100g chunk-${M500} ${M500}
experiment false 100g chunk-${G1}   ${G1}
experiment false 100g chunk-${G2}   ${G2}
experiment false 100g chunk-${G5}   ${G5}
experiment false 100g chunk-${G10}  ${G10}

# Create log directory
mkdir -p $SAVE

# Copy bad-node.log
for i in `seq 2 $MN`; do
  declare MV="M${i}"
  scp "ubuntu@${!MV}:~/bad-node.log" $SAVE/${i}.bad.node.log
done

# Copy bad.log
scp ubuntu@$M1:~/bad.log $SAVE/1.bad.log

# Kill cluster
./shutdown-cluster.sh ${FILE}

