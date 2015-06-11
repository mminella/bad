# Experiment 1

Cluster setup:
  - 1 i2.xlarge machine with data
  - 1 i2.xlarge machine reading

Parameters:
  - read ahead
  - block size
  - max mem (in terms of records)
  - data file size
  - operation

Tests:
  1) Basic
    - read ahead = all of memory
    - block size = say 1 mb
    - max mem = all of memory (same as read ahead)
    - data file size = 10G
    - operation = read whole file and write to disk sorted

