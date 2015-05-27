package main

// Size constants
const (
  B  = 1 
  KB = 1024 * B
  MB = 1024 * KB
  GB = 1024 * MB
  TB = 1024 * GB
)

// Sortbench constants
const (
  RECORD_SIZE = KEY_SIZE + VAL_SIZE
  KEY_SIZE    = 10 * B
  VAL_SIZE    = 90 * B
)

// Cluster setup
const (
  IOPS     = 100
  MEM      = 1 * GB
  NODES    = 1
  DATA     = 1000 * RECORD_SIZE
  IO_BLOCK = 4 * KB
)
