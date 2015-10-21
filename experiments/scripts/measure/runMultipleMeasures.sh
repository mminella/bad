#!/bin/bash
WD=/home/davidt/Projects/bad/experiments/scripts

MACHINES_N=6
MACHINES=(i2.xlarge i2.2xlarge i2.4xlarge i2.8xlarge)
USE_ZONES=(us-east-1a us-east-1b us-east-1d us-east-1e)
SG_ZONES=(ap-southeast-1a ap-southeast-1b)

USE=$WD/../measure-logs/us-east
SNG=$WD/../measure-logs/sg

function nextN() {
  local last=$( ls $1 | sort -n | tail -1 )
  local next=$(( $last + 1 ))
  echo $next
}

function atRandom() {
  declare -a arr=("${!1}")
  local n=$(( $RANDOM % ${#arr[@]} ))
  echo ${arr[$n]}
}

function runExperiment() {
  declare -a zones=("${!1}")
  local path=$2
  
  local zone=$( atRandom zones[@] )
  local machine=$( atRandom MACHINES[@] )

  local logN=$( nextN $path )
  local pathN="$path/$logN"
  local date=$( date )

  echo "$logN, $date, $zone, $machine" >> $path/README.md

  $WD/runExperiment.sh .measure.config $pathN $MACHINES_N $machine $zone
}

runExperiment USE_ZONES[@] $USE
runExperiment SG_ZONES[@] $SNG

