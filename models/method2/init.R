#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod2.R')

# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 5) {
  stop(strwrap("Usage: [machines file] [client i2 type]
	       [i2 type] [nodes] [data size (GB)]"))
}

machines <- loadMachines(args[1])
client   <- filter(machines, type==args[2])
machine  <- filter(machines, type==args[3])
nodes    <- as.numeric(args[4])
data     <- as.numeric(args[5]) * HD_GB
nrecs    <- data / REC_SIZE

# Validate arguments
if (nrow(client) == 0) {
  stop("Unknown client machine type")
} else if (nrow(machine) == 0) {
  stop("Unknown node machine type")
}

# Whole model
rbind(
  m2.init(client, machine, nodes, data)
)
