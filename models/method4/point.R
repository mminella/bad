#!/usr/bin/Rscript
source('./libmethod4.R')

# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 5) {
  stop(strwrap("Usage: [machines file] [i2 type]
               [nodes] [data size (GB)] [nth record]"))
}

machines <- loadMachines(args[1])
machine  <- filter(machines, type==args[2])
nodes    <- as.numeric(args[3])
data     <- as.numeric(args[4]) * HD_GB
nth      <- as.numeric(args[5])
nrecs    <- data / REC_SIZE

# Validate arguments
if (nth >= nrecs) {
  stop("N'th record is outside the data size")
} else if (nrow(machine) == 0) {
  stop("Unknown machine type")
}

# Whole model
rbind(
  m4.allModel(machine, nodes, data),
  m4.firstModel(machine, nodes, data),
  m4.nthModel(machine, nodes, data, nth),
  m4.cdfModel(machine, nodes, data)
)

