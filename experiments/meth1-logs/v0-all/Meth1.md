# Method 1 Experiments

Parameters:
  - read ahead
  - block size
  - max mem (in terms of records)
  - data file size
  - operation

## Model

i2.xlarge:
  - 35,000 IOPS (both R+W)
  - 4KB * IOPS = 136.7MB/s throughput
  - Network = 10Gb/s

Time = Read + Net + Write

1) 100MB -- 7.3s   + 0.08s + 0.73s =   8.1s
2) 500MB -- 36.5s  + 0.4s  + 3.6s  =  40.5s
3) 1GB   -- 73.5s  + 0.8s  + 7.3s  =  81.6s
4) 5GB   -- 365.7s + 4s    + 36.5s = 406.2s
5) 10GB  -- 731.5s + 8s    + 73.1s = 812.6s

## Experiment 1 -- All Records

Cluster setup:
  - 1 i2.xlarge machine with data
  - 1 i2.xlarge machine reading

Parameters:
  - read ahead = 10% of data file
  - block size = 10% of data file
  - max mem = all of memory

1) 100MB --     7s
2) 500MB --    42s
3) 1GB   --    93s
4) 5GB   --   537s
5) 10GB  -- 1,138s

## Experiment 2 -- First Recod

Cluster setup:
  - 1 i2.xlarge machine with data
  - 1 i2.xlarge machine reading

Parameters:
  - max mem = all of memory

1) 100MB -- 0s
2) 500MB -- 2s
3) 1GB   -- 4s
4) 5GB   -- 20s
5) 10GB  -- 39s

## Experiment 3 -- All Records

Cluster setup:
  - 1 i2.xlarge machine with data
  - 1 i2.xlarge machine reading

Parameters:
  - max mem = all of memory

1) 100MB (1% batch)   --     42s
5) 10GB (0.01% batch) -- 40,173s

