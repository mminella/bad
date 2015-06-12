# Experiment Notes

## AWS Info

http://aws.amazon.com/ec2/instance-types
http://docs.aws.amazon.com/AWSEC2/latest/UserGuide/InstanceStorage.html
http://docs.aws.amazon.com/AWSEC2/latest/UserGuide/i2-instances.html

Instance Size | Read IOPS | First Write IOPS
i2.xlarge     |  35,000   |  35,000
i2.2xlarge    |  75,000   |  75,000
i2.4xlarge    | 175,000   | 155,000
i2.8xlarge    | 365,000   | 315,000

First Write: performance degrades as the disk approaches being full.
Recommended to leave 10% of disk un-partitioned to allow SSD controller to use
it as a buffer to optimize writes.

## Experiment 1

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

