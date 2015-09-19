#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod1.R')

# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 8) {
  stop(strwrap("Usage: [machines file] [client i2 type] [i2 type] [start nodes]
               [node points] [data size (GB)] [nth record] [subset size]"))
}

machines <- loadMachines(args[1])
client   <- filter(machines, type==args[2])
machine  <- filter(machines, type==args[3])
nodes    <- as.numeric(args[4])
points   <- as.numeric(args[5])
data     <- as.numeric(args[6]) * HD_GB
nth      <- as.numeric(args[7])
nthSize  <- as.numeric(args[8])
nrecs    <- data / REC_SIZE

# Validate arguments
if (nth >= nrecs | nth < 0) {
  stop("N'th record is outside the data size")
} else if ((nth + nthSize) > nrecs) {
  stop("Subset range is outside the data size")
} else if (nthSize < 1) {
  stop("Subset size must be greater than zero")
} else if (nrow(client) == 0) {
  stop("Unknown client machine type")
} else if (nrow(machine) == 0) {
  stop("Unknown node machine type")
} else if ( points < 1 ) {
  stop("Node points must be greater than zero")
}

genAllModels <- function(n) {
  rbind(
    m1.allModel(client, machine, n, data),
    m1.firstModel(client, machine, n, data),
    m1.nthModel(client, machine, n, data, nth, nthSize),
    m1.cdfModel(client, machine, n, data)
  )
}

minNodes <- 
  ceiling(data / (machine$disk.size * machine$disks))

if (minNodes <= nodes + points - 1) {
  start <- max(minNodes, nodes)
  range <- start:(start + points - 1)

  options( width=200 )
  print(genPoints(rang, genAllModels))
}
