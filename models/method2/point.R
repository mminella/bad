#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod2.R')

# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 6) {
  stop(strwrap("Usage: [machines file] [client i2 type]
               [i2 type] [nodes] [data size (GB)] [nth record]"))
}

machines <- loadMachines(args[1])
client   <- filter(machines, type==args[2])
machine  <- filter(machines, type==args[3])
nodes    <- as.numeric(args[4])
data     <- as.numeric(args[5]) * HD_GB
nth      <- as.numeric(args[6])
nrecs    <- data / REC_SIZE

# Validate arguments
if (nth >= nrecs) {
  stop("N'th record is outside the data size")
} else if (nrow(client) == 0) {
  stop("Unknown client machine type")
} else if (nrow(machine) == 0) {
  stop("Unknown node machine type")
}

# XXX: This doesn't include init time

# Whole model
rbind(
  m2.allModel(client, machine, nodes, data),
  m2.firstModel(client, machine, nodes, data),
  m2.nthModel(client, machine, nodes, data, nth)
  #m2.cdfModel(client, machine, nodes, data)
)
