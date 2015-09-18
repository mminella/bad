#!/usr/bin/env Rscript
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

dataAtNode <- function(machine, nodes, data, mult) {
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
