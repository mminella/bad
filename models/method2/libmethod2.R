#!/usr/bin/env Rscript
suppressMessages(library(dplyr))
library('methods')
library('argparser')

# ASSUMPTIONS:
# - Uniform key distribution.

# 8 byte point + 2 bytes disk / host ID
INDEX_SIZE <- KEY_SIZE + 22

# ===========================================
# Model

m2.startup <- function(client, machine, nodes, data) {
  nodeData <- dataAtNode(machine, nodes, data)
  loadTime <- sequentialRead(machine, nodeData)
  # XXX: Old sort method was slow and nearly linear, Need to plugin new numbers
  # for sort
  sortTime <- machine$psort * (nodeData / (REC_SIZE*1000*1000))
  inSequence(loadTime, sortTime)
}

m2.readAll <- function(client, machine, nodes, data, oneC, start=T) {
  clients   <- ifelse(oneC, 1, nodes)
  nodeData  <- dataAtNode(machine, nodes, data)
  timeIndex <- ifelse(start, m2.startup(client, machine, nodes, data), 0)
  timeDisk  <- randomRead(machine, nodeData / REC_SIZE, nodeData)
  timeNet   <- networkSend(machine, nodes, client, clients, data)
  time      <- inSequence(timeIndex, inParallel(timeNet, timeDisk))
  data.frame(operation="all", nodes=nodes, start=0, length=data/REC_SIZE,
             time.total=time, time.min=toMin(time), time.hr=toHr(time),
             time.index=timeIndex, time.disk=timeDisk, time.net=timeNet,
             cost=toCost(machine, nodes, time))
}

m2.readFirst <- function(client, machine, nodes, data, oneC, start=T) {
  timeIndex <- ifelse(start, m2.startup(client, machine, nodes, data), 0)
  timeDisk  <- randomRead(machine, 1, REC_SIZE)
  timeNet   <- networkSend(machine, nodes, client, 1, REC_SIZE * nodes)
  time      <- inSequence(timeIndex, timeNet, timeDisk)
  data.frame(operation="first", nodes=nodes, start=0, length=1,
             time.total=timeDisk, time.min=toMin(time), time.hr=toHr(time),
             time.index=timeIndex, time.disk=timeDisk, time.net=timeNet,
             cost=toCost(machine, nodes, time))
}

m2.readRange <- function(client, machine, nodes, data, oneC, n, len, start=T) {
  clients   <- ifelse(oneC, 1, nodes)
  nodeRange <- max(1, len / nodes)
  nodeData  <- dataAtNode(machine, nodes, data)
  srchQs    <- log2(nodeData / REC_SIZE)
  timeIndex <- ifelse(start, m2.startup(client, machine, nodes, data), 0)
  # Search orchestrated by client requires approximately log requests
  srchTime  <- (srchQs * client$rtt) / 1000
  timeDisk  <- randomRead(machine, nodeRange, nodeRange * REC_SIZE)
  timeNet   <- networkSend(machine, nodes, client, clients, len * REC_SIZE)
  time      <- inSequence(timeIndex, srchTime, inParallel(timeNet, timeDisk))
  data.frame(operation="nth", nodes=nodes, start=n, length=len,
             time.total=time, time.min=toMin(time), time.hr=toHr(time),
             time.index=timeIndex, time.disk=timeDisk, time.net=timeNet,
             cost=toCost(machine, nodes, time))
}

m2.cdf <- function(client, machine, nodes, data, oneC, points) {
  timeIndex <- m2.startup(client, machine, nodes, data)
  reads <- genPoints(1:points,
                     function(x) {
                       n <- round(x / points * data / REC_SIZE)
                       m2.readRange(client, machine, nodes, data, oneC,
                                    n, 1, start=F)
                     })
  timeOps <- sum(reads$time.total)
  time    <- inSequence(timeIndex, timeOps)
  data.frame(operation="cdf", nodes=nodes, start=NA, length=points,
             time.total=time, time.min=toMin(time), time.hr=toHr(time),
             time.index=timeIndex, time.disk=NA, time.net=NA,
             cost=toCost(machine, nodes, time))
}

m2.reservoir <- function(client, machine, nodes, data, oneC, samples) {
  model <- m2.readRange(client, machine, nodes, data, oneC, 0, samples)
  mutate(model, operation="reservoir", start=NA, length=samples)
}

m2.minNodes <- function(machine, data) {
  minNodesDisk <- ceiling(data/(machine$disk.size * machine$disks))
  indexSize    <- data / REC_SIZE * INDEX_SIZE
  minNodesMem  <- ceiling(indexSize/machine$mem)
  max(minNodesDisk, minNodesMem)
}

m2.parseArgs <- function(graph=F) {
  p <- arg_parser("Local Index B.A.D Model")

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
                    flag=T)
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
