#!/usr/bin/Rscript
suppressMessages(library(dplyr))
# library(ggplot2)
# library(ggthemr)
# library(reshape2)

# ASSUMPTIONS:
# - Uniform key distribution.

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

REC_SIZE      <- 100

dataAtNode <- function(machine, nodes, data) {
  perNode <- ceiling(data / nodes)
  if (perNode > (machine$disk.size * machine$disks)) {
    stop("Not enough disk space for data set")
  }
  perNode
}

allModel <- function(machine, nodes, data) {
  nodeData <- dataAtNode(machine, nodes, data)

  p1Net    <- ceiling(((nodes - 1) * nodeData) / (nodes * machine$netio))
  p1DiskR  <- ceiling(nodeData / (machine$diskio.r * machine$disks))
  p1DiskW  <- ceiling(nodeData / (machine$diskio.w * machine$disks))
  p1Time   <- max(p1Net, p1DiskR, p1DiskW)
  p2Time   <- p1DiskR + p1DiskW
  time     <- p1Time + p2Time

  timeM    <- ceiling(time / M)
  timeH    <- round(time / HR, 2)
  cost     <- ceiling(time / HR) * machine$cost * nodes

  data.frame(operation="all", nodes=nodes,
             time.total=time, time.min=timeM, time.hr=timeH,
             time.p1=p1Time, time.p2=p2Time,
             cost=cost)
}

# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 4) {
  stop(strwrap("Usage: [machines file] [i2 type] [nodes] [data size (GB)]"))
}

machines  <- loadMachines(args[1])
machine   <- filter(machines, type==args[2])
nodes     <- as.numeric(args[3])
data      <- as.numeric(args[4]) * HD_GB
nrecs     <- data / REC_SIZE

# Validate arguments
if (nrow(machine) == 0) {
  stop("Unknown node machine type")
}

# Whole model
rbind(
  allModel(machine, nodes, data)
)

