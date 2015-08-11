#!/bin/bash

./runBad.sh .400g-1 logs-400g/1 400 10 i2.xlarge 2 false &
./runBad.sh .400g-2 logs-400g/2 200 10 i2.xlarge 3 false &
./runBad.sh .400g-3 logs-400g/3 133 10 i2.xlarge 4 false &
./runBad.sh .400g-4 logs-400g/4 100 10 i2.xlarge 5 false &
./runBad.sh .400g-5 logs-400g/5  80 10 i2.xlarge 6 false &
./runBad.sh .400g-6 logs-400g/6  67 10 i2.xlarge 7 false &
./runBad.sh .400g-7 logs-400g/7  57 10 i2.xlarge 8 false &
./runBad.sh .400g-8 logs-400g/8  50 10 i2.xlarge 9 false &
wait

