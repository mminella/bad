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
  perDisk  <- ceiling(perNode / machine$disks)
  bcktSize <- (machine$mem - m4.MEM_RESERVE) / machine$disks / m4.BUCKETS_N
  bcktSize <- bcktSize %/% REC_SIZE * REC_SIZE
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

m4.readAll <- function(client, machine, nodes, data, oneC) {
  nodeData <- dataAtNode(machine, nodes, data, m4.DATA_MULT)
  startup  <- m4.startupModel(machine, nodes, data)$time.startup

  p2DiskR  <- ceiling(nodeData / (machine$diskio.r * machine$disks))
  p2DiskW  <- ceiling(nodeData / (machine$diskio.w * machine$disks))
  if (oneC) {
    netio    <- min(client$netio, machine$netio * nodes)
    p2Net    <- ceiling(data / netio)
    p2Time   <- p2DiskR + max(p2DiskW, p2Net)
  } else {
    p2Time   <- p2DiskR + p2DiskW
  }

  time     <- startup + p2Time
  timeM    <- ceiling(time / M)
  timeH    <- round(time / HR, 2)

  costTime <- max(timeM, machine$billing.mintime)
  cost     <- ceiling(costTime / machine$billing.granularity) *
                machine$cost * nodes

  data.frame(operation="all", nodes=nodes, one.spot=oneC, start=0,
             length=data/REC_SIZE, time.total=time, time.min=timeM,
             time.hr=timeH, time.startup=startup, time.op=p2Time,
             cost=cost)
}

m4.firstRec <- function(client, machine, nodes, data, oneC) {
  if (!m4.LAZY) {
    # Same cost as ReadAll
    model <- m4.readAll(client, machine, nodes, data, F)
    mutate(model, operation="first", one.spot=oneC, length=1)
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

    data.frame(operation="first", nodes=nodes, one.spot=oneC, start=0,
               length=1, time.total=time, time.min=timeM, time.hr=timeH,
               time.startup=startup, time.op=p2Time,
               cost=cost)
  }
}

m4.readRange <- function(client, machine, nodes, data, oneC, n, len) {
  if (!m4.LAZY) {
    # Same cost as ReadAll
    model <- m4.readAll(client, machine, nodes, data, oneC)
    model <- mutate(model, operation="nth", start=n, length=len)

    # Network for phase 2
    if (oneC) {
      bktSize   <- m4.bucketSize(machine, nodes, data)
      layerBkts <- nodes * machine$disks
      dataOut   <- len * REC_SIZE
      bktsOut   <- dataOut / bktSize

      # maximum amount of buckets we need to transfer from any one machine
      layersOut <- floor(bktsOut / layerBkts)
      partOut   <- min(bktsOut %% layerBkts, 1 )
      maxOut    <- (layersOut + partOut) * bktSize
      p2NetOut <- maxOut / machine$netio

      # ingress time to single client
      p2NetIn  <- (len * REC_SIZE) / client$netio

      # total network time
      p2Net    <- max(p2NetOut, p2NetIn)

      # total time for phase 2
      p2Time   <- model$time.op + p2Net
      mutate(model, time.op=p2Time)
    }

    model

  } else {
    # How many buckets do we need to sort?
    bktSize  <- m4.bucketSize(machine, nodes, data)
    nBkts    <- max(ceiling(len * REC_SIZE / bktSize), 1)

    # The implementation round-robbins each bucket over the nodes and their
    # disks to achieve maximum parallel read throughput for a ranged read. So a
    # 'layer' (before we repeat the same machines disk) is (nodes * disks)
    # buckets in size.
    layers   <- max(ceiling(nBkts / (machine$disks * nodes)), 1)
    dataSize <- bktSize * layers
    startup  <- m4.startupModel(machine, nodes, data)$time.startup

    # Disk IO for phase 2
    p2DiskR  <- ceiling(dataSize / machine$diskio.r)
    p2DiskW  <- ceiling(dataSize / machine$diskio.w)

    # Network for phase 2
    if (oneC) {
      layerBkts <- nodes * machine$disks
      dataOut   <- len * REC_SIZE
      bktsOut   <- dataOut / bktSize

      # maximum amount of buckets we need to transfer from any one machine
      layersOut <- floor(bktsOut / layerBkts)
      partOut   <- min(bktsOut %% layerBkts, 1 )
      maxOut    <- (layersOut + partOut) * bktSize
      p2NetOut <- maxOut / machine$netio

      # ingress time to single client
      p2NetIn  <- (len * REC_SIZE) / client$netio

      # total network time
      p2Net    <- max(p2NetOut, p2NetIn)

      # total time for phase 2
      p2Time   <- p2DiskR + max(p2DiskW, p2Net)
    } else {
      p2Time   <- p2DiskR + p2DiskW
    }

    time     <- startup + p2Time
    timeM    <- ceiling(time / M)
    timeH    <- round(time / HR, 2)

    costTime <- max(timeM, machine$billing.mintime)
    cost     <- ceiling(costTime / machine$billing.granularity) *
                  machine$cost * nodes

    data.frame(operation="nth", nodes=nodes, one.spot=oneC, start=n,
               length=len, time.total=time, time.min=timeM, time.hr=timeH,
               time.startup=startup, time.op=p2Time,
               cost=cost)
  }
}

m4.cdf <- function(client, machine, nodes, data, oneC, points) {
  if (points > data / REC_SIZE) {
    stop("Asked for more data points in CDF than records exist")
  }

  if (!m4.LAZY) {
    model <- m4.readAll(client, machine, nodes, data, oneC)
    model <- mutate(model, operation="cdf", start=NA, length=points)
    if (oneC) {
      dataOut        <- points * REC_SIZE
      dataOutPerNode <- dataOut / nodes
      p2NetOut       <- dataOutPerNode / machine$netio
      p2NetIn        <- dataOut / client$netio
      p2Time         <- model$time.op + max(p2NetOut, p2NetIn)
      mutate(model, time.op=p2Time)
    }
    model

  } else {
    # Only need to sort buckets that contain needed points
    layerBkts <- nodes * machine$disks
    bktSize   <- m4.bucketSize(machine, nodes, data)
    buckets   <- data / bktSize
    touched   <- min(points, buckets)

    # How many buckets on a single disk do we need to touch?
    tPerDisk <- max(1, touched / layerBkts)
    bktData  <- tPerDisk * bktSize

    startup  <- m4.startupModel(machine, nodes, data)$time.startup

    p2DiskR  <- ceiling(bktData / machine$diskio.r)
    # XXX: There is a design choice here, we could not write back to disk
    p2DiskW  <- ceiling(bktData / machine$diskio.w)

    if (oneC) {
      dataOut        <- points * REC_SIZE
      dataOutPerNode <- dataOut / nodes
      p2NetOut       <- dataOutPerNode / machine$netio
      p2NetIn        <- dataOut / client$netio
      p2Time         <- p2DiskR + max(p2DiskW, max(p2NetOut, p2NetIn))
    } else {
      p2Time   <- p2DiskR + p2DiskW
    }

    time     <- startup + p2Time
    timeM    <- ceiling(time / M)
    timeH    <- round(time / HR, 2)

    costTime <- max(timeM, machine$billing.mintime)
    cost     <- ceiling(costTime / machine$billing.granularity) *
                  machine$cost * nodes

    data.frame(operation="cdf", nodes=nodes, one.spot=oneC, start=NA,
               length=points, time.total=time, time.min=timeM, time.hr=timeH,
               time.startup=startup, time.op=p2Time,
               cost=cost)
  }
}

m4.reservoir <- function(client, machine, nodes, data, oneC, samples) {
  model <- m4.readRange(client, machine, nodes, data, oneC, 0, samples)
  mutate(model, operation="reservoir", start=NA, length=samples)
}
