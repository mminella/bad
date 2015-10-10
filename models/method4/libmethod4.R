#!/usr/bin/env Rscript
# We copy the data twice, ending up with three copies: in, intermediate, out
suppressMessages(library(dplyr))
library('methods')
library('argparser')

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
m4.BUCKETS_N   <- 2

m4.bucketSize <- function(machine, nodes, data) {
  perNode  <- dataAtNode(machine, nodes, data, m4.DATA_MULT)
  perDisk  <- ceiling(perNode / machine$disks)
  bcktSize <- (machine$mem - m4.MEM_RESERVE) / machine$disks / m4.BUCKETS_N
  bcktSize <- bcktSize %/% REC_SIZE * REC_SIZE
  min(bcktSize, perDisk / m4.BUCKETS_N)
}

m4.startup <- function(machine, nodes, data) {
  nodeData <- dataAtNode(machine, nodes, data, m4.DATA_MULT)
  time     <- shuffleAll(machine, nodes, nodeData)
  data.frame(operation="startup", nodes=nodes, time.startup=time)
}

m4.readAll <- function(client, machine, nodes, data, oneC) {
  nodeData <- dataAtNode(machine, nodes, data, m4.DATA_MULT)
  startup  <- m4.startup(machine, nodes, data)$time.startup
  p2DiskR  <- sequentialRead(machine, nodeData)
  p2DiskW  <- sequentialWrite(machine, nodeData)

  if (oneC) {
    p2Net  <- networkSend(machine, nodes, client, 1, data)
    p2Time <- inSequence(p2DiskR, inParallel(p2DiskW, p2Net))
  } else {
    p2Time <- inSequence(p2DiskR, p2DiskW)
  }

  time <- inSequence(startup, p2Time)
  data.frame(operation="all", nodes=nodes, one.spot=oneC, start=0,
             length=data/REC_SIZE, time.total=time, time.min=toMin(time),
             time.hr=toHr(time), time.startup=startup, time.op=p2Time,
             cost=toCost(machine, nodes, time))
}

m4.readFirst <- function(client, machine, nodes, data, oneC) {
  if (!m4.LAZY) {
    # Same cost as ReadAll
    model <- m4.readAll(client, machine, nodes, data, F)
    mutate(model, operation="first", one.spot=oneC, length=1)
  } else {
    # Only need to sort one bucket, so cheaper than ReadAll
    bktSize  <- m4.bucketSize(machine, nodes, data)
    startup  <- m4.startup(machine, nodes, data)$time.startup
    p2DiskR  <- sequentialRead(machine, bktSize, disks=1)
    p2DiskW  <- sequentialWrite(machine, bktSize, disks=1)
    # ^ XXX: There is a design choice here, we could not write back to disk

    timeOp <- inSequence(p2DiskR, p2DiskW)
    time   <- inSequence(startup, timeOp)
    data.frame(operation="first", nodes=nodes, one.spot=oneC, start=0,
               length=1, time.total=time, time.min=toMin(time),
               time.hr=toHr(time), time.startup=startup, time.op=timeOp,
               cost=toCost(machine, nodes, time))
  }
}

# time to transfer a range to a single machine
m4.bucketNetSend <- function(client, machine, nodes, data, len) {
  rangeSize <- len * REC_SIZE
  bktSize   <- m4.bucketSize(machine, nodes, data)
  layerBkts <- nodes * machine$disks
  bktsOut   <- rangeSize / bktSize

  # maximum amount of buckets we need to transfer from any one machine
  layersOut <- bktsOut %/% layerBkts
  partOut   <- min(bktsOut %% layerBkts, 1 )
  netBytes  <- (layersOut + partOut) * bktSize
  p2NetOut  <- networkOut(machine, netBytes)

  # ingress time to single client
  p2NetIn  <- networkIn(client, rangeSize)

  inParallel(p2NetOut, p2NetIn)
}

m4.readRangeS <- function(client, machine, nodes, data, oneC, n, len) {
  # Shuffle + Sort
  nodeData <- dataAtNode(machine, nodes, data, m4.DATA_MULT)
  startup  <- m4.startup(machine, nodes, data)$time.startup
  p2DiskR  <- sequentialRead(machine, nodeData)
  p2DiskW  <- sequentialWrite(machine, nodeData)

  # Transfer Range to one client
  timeOp <- if (oneC) {
    p2Net <- m4.bucketNetSend(client, machine, nodes, data, len)
    inSequence(p2DiskR, p2DiskW, p2Net)
  } else {
    inSequence(p2DiskR, p2DiskW)
  }

  time <- inSequence(startup, timeOp)
  data.frame(operation="nth", nodes=nodes, one.spot=oneC, start=n,
             length=len, time.total=time, time.min=toMin(time),
             time.hr=toHr(time), time.startup=startup, time.op=timeOp,
             cost=toCost(machine, nodes, time))
}

m4.readRangeL <- function(client, machine, nodes, data, oneC, n, len) {
  # How many buckets do we need to sort? The implementation round-robbins each
  # bucket over the nodes and their disks to achieve maximum parallel read
  # throughput for a ranged read. So a 'layer' (before we repeat the same
  # machines disk) is (nodes * disks) buckets in size.
  rangeSize <- len * REC_SIZE
  bktSize   <- m4.bucketSize(machine, nodes, data)
  layerBkts <- nodes * machine$disks
  nBkts     <- rangeSize / bktSize
  layers    <- ceiling(ceiling(nBkts) / layerBkts)
  sortSize  <- bktSize * layers

  # Startup + Sort
  startup  <- m4.startup(machine, nodes, data)$time.startup
  p2DiskR  <- sequentialRead(machine, sortSize, disks=1)
  p2DiskW  <- sequentialWrite(machine, sortSize, disks=1)

  # Network for phase 2
  timeOp <- if (oneC) {
    p2Net <- m4.bucketNetSend(client, machine, nodes, data, len)
    inSequence(p2DiskR, inParallel(p2DiskW, p2Net))
  } else {
    inSequence(p2DiskR, p2DiskW)
  }

  time <- inSequence(startup, timeOp)
  data.frame(operation="nth", nodes=nodes, one.spot=oneC, start=n,
             length=len, time.total=time, time.min=toMin(time),
             time.hr=toHr(time), time.startup=startup, time.op=timeOp,
             cost=toCost(machine, nodes, time))
}

m4.readRange <- function(client, machine, nodes, data, oneC, n, len) {
  if (!m4.LAZY) {
    m4.readRangeS(client, machine, nodes, data, oneC, n, len)
  } else {
    m4.readRangeL(client, machine, nodes, data, oneC, n, len)
  }
}

m4.cdfS <- function(client, machine, nodes, data, oneC, points) {
  model <- m4.readAll(client, machine, nodes, data, F)
  model <- mutate(model, operation="cdf", start=NA, length=points)
  if (oneC) {
    p2Net  <- networkSend(machine, nodes, client, 1, points * REC_SIZE)
    p2Time <- inSequence(model$time.op, p2Net)
    mutate(model, time.op=p2Time)
  }
  model
}

m4.cdfL <- function(client, machine, nodes, data, oneC, points) {
  # How many buckets on a single disk do we need to touch? Only need to sort
  # buckets that contain needed points
  bktSize   <- m4.bucketSize(machine, nodes, data)
  buckets   <- data / bktSize
  touched   <- min(points, buckets)
  layerBkts <- nodes * machine$disks
  tPerDisk  <- max(1, touched / layerBkts)
  bktData   <- tPerDisk * bktSize

  startup <- m4.startup(machine, nodes, data)$time.startup
  p2DiskR <- sequentialRead(machine, bktData, disks=1)
  p2DiskW <- sequentialWrite(machine, bktData, disks=1)
  # ^ XXX: There is a design choice here, we could not write back to disk

  opTime <- if (oneC) {
    p2Net  <- networkSend(machine, nodes, client, 1, points * REC_SIZE)
    inSequence(p2DiskR, inParallel(p2DiskW, p2Net))
  } else {
    inSequence(p2DiskR, p2DiskW)
  }

  time <- inSequence(startup, opTime)
  data.frame(operation="cdf", nodes=nodes, one.spot=oneC, start=NA,
             length=points, time.total=time, time.min=toMin(time),
             time.hr=toHr(time), time.startup=startup, time.op=opTime,
             cost=toCost(machine, nodes, time))
}

m4.cdf <- function(client, machine, nodes, data, oneC, points) {
  if (points > data / REC_SIZE) {
    stop("Asked for more data points in CDF than records exist")
  }

  if (!m4.LAZY) {
    m4.cdfS(client, machine, nodes, data, oneC, points)
  } else {
    m4.cdfL(client, machine, nodes, data, oneC, points)
  }
}

m4.reservoir <- function(client, machine, nodes, data, oneC, samples) {
  model <- m4.readRange(client, machine, nodes, data, oneC, 0, samples)
  mutate(model, operation="reservoir", start=NA, length=samples)
}

m4.minNodes <- function(machine, data) {
  ceiling((data * m4.DATA_MULT)/(machine$disk.size * machine$disks))
}

m4.parseArgs <- function(graph=F) {
  p <- arg_parser("Shuffle All B.A.D Model")

  if (graph) {
    p <- add_argument(p, "--operation",
                      help="Operation to model [all, nth, first, cdf, reservoir]")
    p <- add_argument(p, "--file", help="File to save graph to [pdf]",
                      default="graph.pdf")
  }
  p <- add_argument(p, "--machines", help="Machine description file")
  p <- add_argument(p, "--client", help="Client machine type", default="i2.8x")
  p <- add_argument(p, "--node", help="Backend machine type")
  p <- add_argument(p, "--min-cluster", help="Minimum cluster size", default=1)
  p <- add_argument(p, "--cluster-points",
                    help="Number of cluster sizes to model", default=1)
  p <- add_argument(p, "--data", help="Data size (GBs)", type='numeric')
  p <- add_argument(p, "--one-client", help="Send results all to the client?",
                    flag=T, default=F)
  p <- add_argument(p, "--range-start", help="Ranged read start position",
                    default=0)
  p <- add_argument(p, "--range-size", help="Ranged read size", default=10000)
  p <- add_argument(p, "--cdf", help="CDF points", default=100)
  p <- add_argument(p, "--reservoir", help="Reservoir sampling size",
                    default=10000)

  args <- commandArgs(trailingOnly = T)
  if (length(args) == 0) {
    print(p)
    stop()
  }
  parse_args(p)
}
