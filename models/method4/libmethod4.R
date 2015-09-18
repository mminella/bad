#!/usr/bin/env Rscript
# We copy the data twice, ending up with three copies: in, intermediate, out
suppressMessages(library(dplyr))

# ASSUMPTIONS:
# - Uniform key distribution.

# ===========================================
# Implementation Type

# Do we sort buckets lazily as needed? This optimization should improve the
# time for operations that don't touch all buckets, without hurting performance
# for those that do.
m4.LAZY <- TRUE

# ===========================================
# Model

m4.DATA_MULT   <- 3
m4.MEM_RESERVE <- 500 * MB
m4.BUCKETS_N   <- 1

m4.bucketSize <- function(machine, nodes, data) {
  perNode  <- dataAtNode(machine, nodes, data, m4.DATA_MULT)
  perDisk  <- perNode / machine$disks
  bcktSize <- (machine$mem - m4.MEM_RESERVE) / machine$disks / m4.BUCKETS_N
  perBckt  <- min(bcktSize, perDisk / m4.BUCKETS_N)
  perBckt
}

m4.startupModel <- function(machine, nodes, data) {
  nodeData <- dataAtNode(machine, nodes, data, m4.DATA_MULT)

  p1Net    <- ceiling(((nodes - 1) * nodeData) / (nodes * machine$netio))
  p1DiskR  <- ceiling(nodeData / (machine$diskio.r * machine$disks))
  p1DiskW  <- ceiling(nodeData / (machine$diskio.w * machine$disks))
  p1Time   <- max(p1Net, p1DiskR, p1DiskW)

  data.frame(operation="startup", nodes=nodes, time.startup=p1Time)
}

m4.allModel <- function(machine, nodes, data) {
  nodeData <- dataAtNode(machine, nodes, data, m4.DATA_MULT)
  startup  <- m4.startupModel(machine, nodes, data)$time.startup

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

m4.firstModel <- function(machine, nodes, data) {
  if (!m4.LAZY) {
    # Same cost as ReadAll
    model <- m4.allModel(machine, nodes, data)
    mutate(model, operation="first", length=1)
  } else {
    # Only need to sort one bucket, so cheaper than ReadAll
    bktSize  <- m4.bucketSize(machine, nodes, data)
    startup  <- m4.startupModel(machine, nodes, data)$time.startup

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

m4.nthModel <- function(machine, nodes, data, n) {
  if (!m4.LAZY) {
    # Same cost as ReadAll
    model <- m4.allModel(machine, nodes, data)
    mutate(model, operation="nth", start=n, length=1)
  } else {
    # Same cost as First
    model <- m4.firstModel(machine, nodes, data)
    mutate(model, operation="nth", start=n, length=1)
  }
}

m4.cdfModel <- function(machine, nodes, data) {
  if (!m4.LAZY) {
    model <- m4.allModel(client, machine, x, data)
    mutate(model, operation="cdf", start=NA, length=100)
  } else {
    # Only need to sort buckets that contain needed points
    bktSize  <- m4.bucketSize(machine, nodes, data)
    buckets  <- data / bktSize
    touched  <- min(100, buckets)

    # How many buckets on a single disk do we need to touch?
    tPerDisk <- max(1, touched / nodes / machine$disks)
    bktData  <- tPerDisk * bktSize

    startup  <- m4.startupModel(machine, nodes, data)$time.startup

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
