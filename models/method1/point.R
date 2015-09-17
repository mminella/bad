#!/usr/bin/Rscript
source('./libmethod1.R')

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

# Whole model
rbind(
  allModel(client, machine, nodes, data),
  firstModel(client, machine, nodes, data),
  nthModel(client, machine, nodes, data, nth)
)

