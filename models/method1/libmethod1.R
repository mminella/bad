#!/usr/bin/env Rscript
suppressMessages(library(dplyr))
library('methods')
library('argparser')

# LIMITATIONS:
# - We assume reads at backends will perfectly overlap. While not too
# unreasonable with a uniform key distribution, at larger cluster sizes (both
# due to more machines, and since the network buffer per backend node at a
# client is by neccessity smaller, this assumption fails).

# ASSUMPTIONS:
# - Uniform key distribution.


# ===========================================
# Model

REC_VAL_ALIGNED <- 112
REC_PTR_SIZE  <- 18

MEM_RESERVE <- 0 * MB
NET_BUFFER  <- 500 * MB
DISK_BUFFER <- 4000 * MB

SORT_RATIO  <- 14

# We copy across the dynamic chunk sizing from the meth1 code.
m1.chunkSize <- function(mem, disks) {
  mem <- mem -  MEM_RESERVE
  mem <- mem -  NET_BUFFER * 2
  mem <- mem -  DISK_BUFFER * disks

  div1 <- 2 * REC_PTR_SIZE + REC_VAL_ALIGNED
  div2 <- floor((REC_PTR_SIZE + REC_VAL_ALIGNED) / SORT_RATIO)

  floor( mem /  (div1 + div2) ) * REC_SIZE
}

m1.readAll <- function(client, machine, nodes, data) {
  chunk    <- m1.chunkSize(machine$mem, machine$disks)
  nodeData <- dataAtNode(machine, nodes, data)
  scans    <- ceiling(nodeData / chunk)

  timeNet  <- networkSend(machine, nodes, client, 1, data)
  timeDisk <- sequentialRead(machine, nodeData) * scans
  time     <- timeNet + timeDisk

  data.frame(operation="all", nodes=nodes, start=0, length=data/REC_SIZE,
             time.total=time, time.min=toMin(time), time.hr=toHr(time),
             time.disk=timeDisk, time.net=timeNet,
             cost=toCost(machine, nodes, time))
}

m1.readFirst <- function(client, machine, nodes, data) {
  nodeData <- dataAtNode(machine, nodes, data)
  timeDisk <- sequentialRead(machine, nodeData)
  time     <- timeDisk

  data.frame(operation="first", nodes=nodes, start=0, length=1,
             time.total=timeDisk, time.min=toMin(time), time.hr=toHr(time),
             time.disk=timeDisk, time.net=0,
             cost=toCost(machine, nodes, time))
}

m1.readRange <- function(client, machine, nodes, data, n, len) {
  # XXX: Since assuming normal distribution we're assuming for a range read of
  # length `len`, we retrieve `len / nodes` from each node.
  dataN    <- (n + len) * REC_SIZE

  chunk    <- m1.chunkSize(machine$mem, machine$disks)
  nodeData <- dataAtNode(machine, nodes, data)
  scans    <- max(ceiling(dataN / chunk / nodes), 1)

  timeNet  <- networkSend(machine, nodes, client, 1, dataN)
  timeDisk <- sequentialRead(machine, nodeData) * scans
  time     <- timeNet + timeDisk

  data.frame(operation="nth", nodes=nodes, start=n, length=len,
             time.total=time, time.min=toMin(time), time.hr=toHr(time),
             time.disk=timeDisk, time.net=timeNet,
             cost=toCost(machine, nodes, time))
}

m1.cdf <- function(client, machine, nodes, data, points) {
  model <- m1.readAll(client, machine, nodes, data)
  mutate(model, operation="cdf", start=NA, length=points)
}

m1.reservoir <- function(client, machine, nodes, data, samples) {
  model <- m1.readRange(client, machine, nodes, data, 0, samples)
  mutate(model, operation="reservoir", start=NA, length=samples)
}

m1.minNodes <- function(machine, data) {
  ceiling(data/(machine$disk.size * machine$disks))
}

m1.parseArgs <- function(graph=F) {
  p <- arg_parser("Linear Scan B.A.D Model")

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
