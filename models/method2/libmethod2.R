#!/usr/bin/env Rscript
suppressMessages(library(dplyr))

# ASSUMPTIONS:
# - Uniform key distribution.

# ===========================================
# Model

REC_ALIGNMENT <- 8
REC_PTR_SIZE  <- 18

m2.init <- function(client, machine, nodes, data) {
  nodeData <- dataAtNode(machine, nodes, data, 1)
  loadTime <- round(nodeData / (machine$diskio.r * machine$disks))

  # Old sort method was slow and nearly linear
  # Need to plugin new numbers for sort
  sortTime <- loadTime

  loadTime + sortTime
}

m2.randIO <- function(machine, ios) {
  randtime  <- ios / machine$iops.r
  seqtime   <- ios * REC_SIZE / machine$diskio.r
  max(randtime, seqtime)
}

m2.allModel <- function(client, machine, nodes, data) {
  nodeData <- dataAtNode(machine, nodes, data, 1)

  netio    <- min(client$netio, machine$netio * nodes)
  timeNet  <- round(data / netio)

  ios      <- nodeData / REC_SIZE
  timeDisk <- round(m2.randIO(machine, ios))

  time     <- max(timeNet, timeDisk)

  timeM    <- ceiling(time / M)
  timeH    <- round(time / HR, 2)

  costTime <- max(timeM, machine$billing.mintime)
  cost     <- ceiling(costTime / machine$billing.granularity) *
                machine$cost * nodes

  data.frame(operation="all", nodes=nodes, start=0, length=data/REC_SIZE,
             time.total=time, time.min=timeM, time.hr=timeH,
	     time.disk=0, time.net=0,
             cost=cost)
}

m2.firstModel <- function(client, machine, nodes, data) {
  nodeData <- dataAtNode(machine, nodes, data, 1)
  timeDisk <- round(nodeData / (machine$diskio.r * machine$disks))
  time     <- timeDisk

  timeM    <- ceiling(time / M)
  timeH    <- round(time / HR, 2)

  costTime <- max(timeM, machine$billing.mintime)
  cost     <- ceiling(costTime / machine$billing.granularity) *
                machine$cost * nodes

  data.frame(operation="first", nodes=nodes, start=0, length=1,
             time.total=timeDisk, time.min=timeM, time.hr=timeH,
             time.disk=timeDisk, time.net=0,
             cost=cost)
}

m2.nthModel <- function(client, machine, nodes, data, n) {
  nodeData <- dataAtNode(machine, nodes, data, 1)

  srchQs   <- log2(nodeData / REC_SIZE)
  srchTime <- m2.randIO(machine, srchQs) #+ (srchQs * client$rtt)

  netio    <- min(client$netio, machine$netio * nodes)
  timeNet  <- n*REC_SIZE / netio

  timeDisk <- m2.randIO(machine, nodeData / REC_SIZE)
  time     <- max(timeNet, timeDisk) + srchTime

  timeM    <- ceiling(time / M)
  timeH    <- round(time / HR, 2)

  costTime <- max(timeM, machine$billing.mintime)
  cost     <- ceiling(costTime / machine$billing.granularity) *
                machine$cost * nodes

  data.frame(operation="nth", nodes=nodes, start=n, length=1,
             time.total=time, time.min=timeM, time.hr=timeH,
	     time.disk=timeDisk, time.net=timeNet,
             cost=cost)
}

m2.cdfModel <- function(client, machine, nodes, data) {
  # TODO
  #model <- m2.allModel(client, machine, nodes, data)
  #mutate(model, operation="cdf", start=NA, length=100)
}
