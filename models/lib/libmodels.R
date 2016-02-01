#!/usr/bin/env Rscript
library(plyr)
suppressMessages(library(dplyr))


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

dataAtNode <- function(machine, nodes, data, mult=1) {
  perNode <- data / nodes
  if (perNode * mult > (machine$disk.size * machine$disks)) {
    stop("Not enough disk space for data set")
  }
  ceiling(perNode)
}


# ===========================================
# Record Sizes

REC_SIZE <- 100
KEY_SIZE <- 10
VAL_SIZE <- 90

# ===========================================
# Helpers

genPoints <- function(range, f) {
  do.call("rbind", lapply(range, f))
}

toMin <- function(t) {
  ceiling(t/M)
}

toHr <- function(t) {
  round(t/HR,2)
}

toCost <- function(machine, nodes, t) {
  billedTime <- max(toMin(t), machine$billing.mintime)
  ceiling(billedTime / machine$billing.granularity) * machine$cost * nodes
}


# ===========================================
# Components

inParallel <- function(...) {
  max(...)
}

inSequence <- function(...) {
  sum(...)
}

sequentialRead <- function(machine, data, disks=machine$disks) {
  round(data / (machine$diskio.r * disks))
}

sequentialWrite <- function(machine, data, disks=machine$disks) {
  round(data / (machine$diskio.w * disks))
}

randomRead <- function(machine, ios, data) {
  randtime <- ios / (machine$iops.r * machine$disks)
  seqtime  <- data / (machine$diskio.r * machine$disks)
  round(max(randtime, seqtime))
}

networkOut <- function(machine, data) {
  round(data/machine$netio)
}

networkIn <- function(machine, data) {
  round(data/machine$netio)
}

networkSend <- function(machineOut, outN, machineIn, inN, data) {
  outBytes <- machineOut$netio * outN
  inBytes  <- machineIn$netio * inN
  netio    <- min(outBytes, inBytes)
  round(data/netio)
}

shuffleAll <- function(machine, nodes, nodeData) {
  p1DiskR  <- round(nodeData / (machine$diskio.r * machine$disks))
  p1Net    <- round(((nodes - 1) * nodeData) / (nodes * machine$netio))
  p1DiskW  <- round(nodeData / (machine$diskio.w * machine$disks))
  inParallel(p1Net, p1DiskR, p1DiskW)
}
