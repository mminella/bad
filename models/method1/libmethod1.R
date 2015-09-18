#!/usr/bin/env Rscript
suppressMessages(library(dplyr))

# LIMITATIONS:
# - We assume reads at backends will perfectly overlap. While not too
# unreasonable with a uniform key distribution, at larger cluster sizes (both
# due to more machines, and since the network buffer per backend node at a
# client is by neccessity smaller, this assumption fails).

# ASSUMPTIONS:
# - Uniform key distribution.


# ===========================================
# Model

REC_ALIGNMENT <- 8
REC_PTR_SIZE  <- 18

MEM_RESERVE <- 500 * MB
NET_BUFFER  <- 500 * MB
DISK_BUFFER <- 4000 * MB

SORT_RATIO  <- 14

# We copy across the dynamic chunk sizing from the meth1 code.
m1.chunkSize <- function(mem, disks) {
  mem <- mem -  MEM_RESERVE
  mem <- mem -  NET_BUFFER * 2
  mem <- mem -  DISK_BUFFER * disks

  valMem <- ceiling(VAL_SIZE / REC_ALIGNMENT) * REC_ALIGNMENT
  div1 <- 2 * REC_PTR_SIZE + valMem
  div2 <- floor((REC_PTR_SIZE + valMem) / SORT_RATIO)

  floor( mem /  (div1 + div2) ) * REC_SIZE
}

m1.allModel <- function(client, machine, nodes, data) {
  chunk    <- m1.chunkSize(machine$mem, machine$disks)
  nodeData <- dataAtNode(machine, nodes, data, 1)
  scans    <- ceiling(nodeData / chunk)

  netio    <- min(client$netio, machine$netio * nodes)
  timeNet  <- round(data / netio)
  timeDisk <- round(nodeData / (machine$diskio.r * machine$disks) * scans)
  time     <- timeNet + timeDisk

  timeM    <- ceiling(time / M)
  timeH    <- round(time / HR, 2)

  costTime <- max(timeM, machine$billing.mintime)
  cost     <- ceiling(costTime / machine$billing.granularity) *
                machine$cost * nodes

  data.frame(operation="all", nodes=nodes, start=0, length=data/REC_SIZE,
             time.total=time, time.min=timeM, time.hr=timeH,
             time.disk=timeDisk, time.net=timeNet,
             cost=cost)
}

m1.firstModel <- function(client, machine, nodes, data) {
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

m1.nthModel <- function(client, machine, nodes, data, n) {
  chunk    <- m1.chunkSize(machine$mem, machine$disks)
  nodeData <- dataAtNode(machine, nodes, data, 1)
  scans    <- max(ceiling(n * REC_SIZE / chunk / nodes), 1)

  netio    <- min(client$netio, machine$netio * nodes)
  timeNet  <- n*REC_SIZE / netio
  timeDisk <- round(nodeData / (machine$diskio.r * machine$disks) * scans)
  time     <- timeNet + timeDisk

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

m1.cdfModel <- function(client, machine, nodes, data) {
  model <- m1.allModel(client, machine, nodes, data)
  mutate(model, operation="cdf", start=NA, length=100)
}
