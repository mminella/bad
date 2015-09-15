#!/usr/bin/Rscript
library(dplyr)

# ASSUMPTIONS:
# - Uniform key distribution.

# ===========================================
# Units

B  <- 1
KB <- 1024 * B
MB <- 1024 * KB
GB <- 1024 * MB

S  <- 1
M  <- 60 * S
HR <- 60 * M

# ===========================================
# Machines

loadMachines <- function(f) {
  if (!file.exists(f)) {
    stop("File '", f, "' doesn't exist")
  }
  machines <- read.delim(f, header=T, sep=',', strip.white=TRUE, comment.char="#")
  mutate(machines,
    mem = mem * GB,
    disk.size = disk.size * GB,
    diskio = diskio * MB,
    netio = netio * MB)
}

# ===========================================
# Model

REC_SIZE      <- 100
REC_VAL_SIZE  <- 90
REC_ALIGNMENT <- 8
REC_PTR_SIZE  <- 18

MEM_RESERVE <- 500 * MB
NET_BUFFER  <- 500 * MB
DISK_BUFFER <- 4000 * MB

SORT_RATIO   <- 14

# We copy across the dynamic chunk sizing from the meth1 code.
chunkSize <- function(mem, disks) {
  mem <- mem -  MEM_RESERVE
  mem <- mem -  NET_BUFFER * 2
  mem <- mem -  DISK_BUFFER * disks

  valMem <- ceiling(REC_VAL_SIZE / REC_ALIGNMENT) * REC_ALIGNMENT
  div1 <- 2 * REC_PTR_SIZE + valMem
  div2 <- floor((REC_PTR_SIZE + valMem) / SORT_RATIO)

  floor( mem /  (div1 + div2) ) * REC_SIZE
}

dataAtNode <- function(machine, nodes, data) {
  perNode <- ceiling(data / nodes)
  if (perNode > (machine$disk.size * machine$disks)) {
    stop("Not enough disk space for data set")
  }
  perNode
}

nthModel <- function(machine, nodes, data, n) {
  chunk       <- chunkSize(machine$mem, machine$disks)
  dataPerNode <- dataAtNode(machine, nodes, data)
  scans       <- max(ceiling(n*REC_SIZE/chunk/nodes), 1)

  timeNet   <- n*REC_SIZE / machine$netio
  timeDisk  <- round(dataPerNode / (machine$diskio * machine$disks) * scans)
  timeTotal <- timeNet + timeDisk
  cost      <- ceiling(timeTotal / HR) * machine$cost * nodes

  data.frame(operation="nth", start=n, length=1,
             time.total=timeTotal, time.disk=timeDisk,
             time.net=timeNet, cost=cost)
}

allModel <- function(machine, nodes, data) {
  chunk       <- chunkSize(machine$mem, machine$disks)
  dataPerNode <- dataAtNode(machine, nodes, data)
  scans       <- ceiling(dataPerNode/chunk)

  timeNet   <- round(data / machine$netio)
  timeDisk  <- round(dataPerNode / (machine$diskio * machine$disks) * scans)
  timeTotal <- timeNet + timeDisk
  cost      <- ceiling(timeTotal / HR) * machine$cost * nodes

  data.frame(operation="all", start=0, length=data/REC_SIZE,
             time.total=timeTotal, time.disk=timeDisk,
             time.net=timeNet, cost=cost)
}

firstModel <- function(machine, nodes, data) {
  dataPerNode <- dataAtNode(machine, nodes, data)
  timeDisk    <- round(dataPerNode / (machine$diskio * machine$disks))
  cost        <- ceiling(timeDisk / HR) * machine$cost * nodes

  data.frame(operation="first", start=0, length=1,
             time.total=timeDisk, time.disk=timeDisk,
             time.net=0, cost=cost)
}

# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 5) {
  stop("Usage: [machines file] [i2 type] [nodes] [data size (GB)] [nth record]")
}

machines  <- loadMachines(args[1])
machine   <- filter(machines, type==args[2])
nodes     <- as.numeric(args[3])
data      <- as.numeric(args[4]) * GB
nrecs     <- data / REC_SIZE
nth       <- as.numeric(args[5])

if ( nth >= nrecs ) {
  stop("N'th record is outside the data size")
}

chunk       <- chunkSize(machine$mem, machine$disks)
dataPerNode <- ceiling(data / nodes)
scans       <- ceiling(dataPerNode/chunk)

rbind(
  allModel(machine, nodes, data),
  firstModel(machine, nodes, data),
  nthModel(machine, nodes, data, nth)
)
