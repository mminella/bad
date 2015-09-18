#!/usr/bin/Rscript
suppressMessages(library(dplyr))

# ASSUMPTIONS:
# - Uniform key distribution.

# ===========================================
# Implementation Type

# Do we sort buckets lazily as needed? This optimization should improve the
# time for operations that don't touch all buckets, without hurting performance
# for those that do.
LAZY <- TRUE

# ===========================================
# Units

B  <- 1
KB <- 1024 * B
MB <- 1024 * KB
GB <- 1024 * MB

# The whole 1024 vs 1000 business
HD_GB <- 1000 * 1000 * 1000

S  <- 1
M  <- 60 * S
HR <- 60 * M

# ===========================================
# Machines

DATA_MULTIPLIER <- 3

loadMachines <- function(f) {
  if (!file.exists(f)) {
    stop("File '", f, "' doesn't exist")
  }
  machines <- read.delim(f, header=T, sep=',', strip.white=TRUE, comment.char="#")
  mutate(machines,
    mem = mem * GB,
    disk.size = disk.size * HD_GB,
    diskio.r = diskio.r * MB,
    diskio.w = diskio.w * MB,
    netio = netio * MB)
}

# ===========================================
# Model

REC_SIZE    <- 100
MEM_RESERVE <- 500 * MB
BUCKETS_N   <- 1

dataAtNode <- function(machine, nodes, data) {
  perNode <- ceiling(data / nodes)
  if (perNode * DATA_MULTIPLIER > (machine$disk.size * machine$disks)) {
    stop("Not enough disk space for data set")
  }
  perNode
}

bucketSize <- function(machine, nodes, data) {
  perNode  <- dataAtNode(machine, nodes, data)
  perDisk  <- perNode / machine$disks
  bcktSize <- (machine$mem - MEM_RESERVE) / machine$disks / BUCKETS_N
  perBckt  <- min(bcktSize, perDisk / BUCKETS_N)
  perBckt
}

startupModel <- function(machine, nodes, data) {
  nodeData <- dataAtNode(machine, nodes, data)

  p1Net    <- ceiling(((nodes - 1) * nodeData) / (nodes * machine$netio))
  p1DiskR  <- ceiling(nodeData / (machine$diskio.r * machine$disks))
  p1DiskW  <- ceiling(nodeData / (machine$diskio.w * machine$disks))
  p1Time   <- max(p1Net, p1DiskR, p1DiskW)

  data.frame(operation="startup", nodes=nodes, time.startup=p1Time)
}

allModel <- function(machine, nodes, data) {
  nodeData <- dataAtNode(machine, nodes, data)
  startup  <- startupModel(machine, nodes, data)$time.startup

  p2DiskR  <- ceiling(nodeData / (machine$diskio.r * machine$disks))
  p2DiskW  <- ceiling(nodeData / (machine$diskio.w * machine$disks))
  p2Time   <- p2DiskR + p2DiskW

  time     <- startup + p2Time
  timeM    <- ceiling(time / M)
  timeH    <- round(time / HR, 2)

  costTime <- max(timeM, machine$billing.mintime)
  cost     <- ceiling(costTime / machine$billing.granularity) *
                machine$cost * nodes

  data.frame(operation="all", nodes=nodes, start=0, length=data/REC_SIZE,
             time.total=time, time.min=timeM, time.hr=timeH,
             time.startup=startup, time.op=p2Time,
             cost=cost)
}

firstModel <- function(machine, nodes, data) {
  if (!LAZY) {
    # Same cost as ReadAll
    model <- allModel(machine, nodes, data)
    mutate(model, operation="first", length=1)
  } else {
    # Only need to sort one bucket, so cheaper than ReadAll
    bktSize  <- bucketSize(machine, nodes, data)
    startup  <- startupModel(machine, nodes, data)$time.startup

    p2DiskR  <- ceiling(bktSize / machine$diskio.r)
    # XXX: There is a design choice here, we could not write back to disk
    p2DiskW  <- ceiling(bktSize / machine$diskio.w)
    p2Time   <- p2DiskR + p2DiskW

    time     <- startup + p2Time
    timeM    <- ceiling(time / M)
    timeH    <- round(time / HR, 2)

    costTime <- max(timeM, machine$billing.mintime)
    cost     <- ceiling(costTime / machine$billing.granularity) *
                  machine$cost * nodes

    data.frame(operation="first", nodes=nodes, start=0, length=1,
               time.total=time, time.min=timeM, time.hr=timeH,
               time.startup=startup, time.op=p2Time,
               cost=cost)
  }
}

nthModel <- function(machine, nodes, data, n) {
  if (!LAZY) {
    # Same cost as ReadAll
    model <- allModel(machine, nodes, data)
    mutate(model, operation="nth", start=n, length=1)
  } else {
    # Same cost as First
    model <- firstModel(machine, nodes, data)
    mutate(model, operation="nth", start=n, length=1)
  }
}

cdfModel <- function(machine, nodes, data) {
  if (!LAZY) {
    model <- allModel(client, machine, x, data)
    mutate(model, operation="cdf", start=NA, length=100)
  } else {
    # Only need to sort buckets that contain needed points
    bktSize  <- bucketSize(machine, nodes, data)
    buckets  <- data / bktSize
    touched  <- min(100, buckets)

    # How many buckets on a single disk do we need to touch?
    tPerDisk <- max(1, touched / nodes / machine$disks)
    bktData  <- tPerDisk * bktSize

    startup  <- startupModel(machine, nodes, data)$time.startup

    p2DiskR  <- ceiling(bktData / machine$diskio.r)
    # XXX: There is a design choice here, we could not write back to disk
    p2DiskW  <- ceiling(bktData / machine$diskio.w)
    p2Time   <- p2DiskR + p2DiskW

    time     <- startup + p2Time
    timeM    <- ceiling(time / M)
    timeH    <- round(time / HR, 2)

    costTime <- max(timeM, machine$billing.mintime)
    cost     <- ceiling(costTime / machine$billing.granularity) *
                  machine$cost * nodes

    data.frame(operation="cdf", nodes=nodes, start=NA, length=100,
               time.total=time, time.min=timeM, time.hr=timeH,
               time.startup=startup, time.op=p2Time,
               cost=cost)
  }
}
