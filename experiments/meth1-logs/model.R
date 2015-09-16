#!/usr/bin/Rscript
library(dplyr)
library(ggplot2)
library(ggthemr)
library(reshape2)

# LIMITATIONS:
# - We assume reads at backends will perfectly overlap. While not too
# unreasonable with a uniform key distribution, at larger cluster sizes (both
# due to more machines, and since the network buffer per backend node at a
# client is by neccessity smaller, this assumption fails).

# ASSUMPTIONS:
# - Uniform key distribution.

# ===========================================
# Units

B  <- 1
KB <- 1024 * B
MB <- 1024 * KB
GB <- 1024 * MB

# Annoy 1024 vs 1000 business
HD_GB <- 1000 * 1000 * 1000

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
    disk.size = disk.size * HD_GB,
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

nthModel <- function(client, machine, nodes, data, n) {
  chunk       <- chunkSize(machine$mem, machine$disks)
  dataPerNode <- dataAtNode(machine, nodes, data)
  scans       <- max(ceiling(n*REC_SIZE/chunk/nodes), 1)

  netio      <- min(client$netio, machine$netio * nodes)
  timeNet    <- n*REC_SIZE / netio
  timeDisk   <- round(dataPerNode / (machine$diskio * machine$disks) * scans)
  timeTotal  <- timeNet + timeDisk
  timeTotalH <- round(timeTotal / HR, 2)
  cost       <- ceiling(timeTotal / HR) * machine$cost * nodes
  clientCost <- ceiling(timeTotal / HR) * client$cost
  costTotal  <- cost + clientCost

  data.frame(operation="nth", nodes=nodes, start=n, length=1,
             time.total=timeTotal, time.disk=timeDisk,
             time.net=timeNet, cost=cost, time.hours=timeTotalH)
}

allModel <- function(client, machine, nodes, data) {
  chunk       <- chunkSize(machine$mem, machine$disks)
  dataPerNode <- dataAtNode(machine, nodes, data)
  scans       <- ceiling(dataPerNode/chunk)

  netio      <- min(client$netio, machine$netio * nodes)
  timeNet    <- round(data / netio)
  timeDisk   <- round(dataPerNode / (machine$diskio * machine$disks) * scans)
  timeTotal  <- timeNet + timeDisk
  timeTotalH <- round(timeTotal / HR, 2)
  cost       <- ceiling(timeTotal / HR) * machine$cost * nodes
  clientCost <- ceiling(timeTotal / HR) * client$cost
  costTotal  <- cost + clientCost

  data.frame(operation="all", nodes=nodes, start=0, length=data/REC_SIZE,
             time.total=timeTotal, time.disk=timeDisk,
             time.net=timeNet, cost=costTotal, time.hours=timeTotalH)
}

firstModel <- function(client, machine, nodes, data) {
  dataPerNode <- dataAtNode(machine, nodes, data)
  timeDisk    <- round(dataPerNode / (machine$diskio * machine$disks))
  timeTotalH  <- round(timeDisk / HR, 2)
  cost        <- ceiling(timeDisk / HR) * machine$cost * nodes

  data.frame(operation="first", nodes=nodes, start=0, length=1,
             time.total=timeDisk, time.disk=timeDisk,
             time.net=0, cost=cost, time.hours=timeTotalH)
}

# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 6) {
  stop(strwrap("Usage: [machines file] [client i2 type]
               [i2 type] [nodes] [data size (GB)] [nth record]"))
}

machines  <- loadMachines(args[1])
client    <- filter(machines, type==args[2])
machine   <- filter(machines, type==args[3])
nodes     <- as.numeric(args[4])
data      <- as.numeric(args[5]) * HD_GB
nrecs     <- data / REC_SIZE
nth       <- as.numeric(args[6])
if ( nth >= nrecs ) {
  stop("N'th record is outside the data size")
}

# Whole model
rbind(
  allModel(client, machine, nodes, data),
  firstModel(client, machine, nodes, data),
  nthModel(client, machine, nodes, data, nth)
)

# # ===========================================
# # Graphing
#
# # for color scheme list -- https://github.com/cttobin/ggthemr
# ggthemr('fresh')
#
# mkGraph <- function(file, data, title, xl, yl) {
#   if (is.data.frame(data) & nrow(data) > 0) {
#     pdf(file)
#     g <- ggplot(data, aes(clarity, x=xv, y=yv, group=variable, colour=variable))
#     g <- g + geom_line()
#     g <- g + ggtitle(title)
#     g <- g + xlab(xl)
#     g <- g + ylab(yl)
#     print(g)
#     dev.off()
#   }
# }
#
# start <- ceiling(data / (machine$disk.size * machine$disks))
#
# range <- start:(start+45)
# preds <- do.call("rbind",
#                  lapply(range, function(x) allModel(client, machine, x, data)))
#
# # read all vs cost
# title <- paste(client$type, "Client <->", machine$type, "Cluster: ReadAll -",
#                data / HD_GB, "GB")
# points <-
#   select(preds, nodes, time=time.total, cost) %>%
#   mutate(time=time / HR) %>%
#   melt(id.vars=c("nodes"), variable.name="variable", value.name="yv") %>%
#   select(xv=nodes, variable, yv)
# mkGraph("total.pdf", points, title, "Machines", "Time (hr) / Cost ($)")
#
# # disk vs network
# points <-
#   select(preds, nodes, time.disk, time.net) %>%
#   melt(id.vars=c("nodes"), variable.name="variable", value.name="yv") %>%
#   select(xv=nodes, variable, yv)
# mkGraph("total.pdf", points, title, "Machines", "Time (hr) / Cost ($)")
