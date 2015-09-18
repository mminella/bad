#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod4.R')

# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 7) {
  stop(strwrap("Usage: [machines file] [i2 type] [start nodes] [node points]
               [data size (GB)] [nth record] [subset size]"))
}

machines <- loadMachines(args[1])
machine  <- filter(machines, type==args[2])
nodes    <- as.numeric(args[3])
points   <- as.numeric(args[4])
data     <- as.numeric(args[5]) * HD_GB
nth      <- as.numeric(args[6])
nthSize  <- as.numeric(args[7])
nrecs    <- data / REC_SIZE

# Validate arguments
if (nth >= nrecs | nth < 0) {
  stop("N'th record is outside the data size")
} else if ((nth + nthSize) > nrecs) {
  stop("Subset range is outside the data size")
} else if (nthSize < 1) {
  stop("Subset size must be greater than zero")
} else if (nrow(machine) == 0) {
  stop("Unknown machine type")
} else if ( points < 1 ) {
  stop("Node points must be greater than zero")
}

genAllModels <- function(n) {
  rbind(
    m4.allModel(machine, n, data),
    m4.firstModel(machine, n, data),
    m4.nthModel(machine, n, data, nth, nthSize),
    m4.cdfModel(machine, n, data)
  )
}

options( width=200 )
print(
  genPoints(nodes:(nodes + points - 1), genAllModels),
  row.names=FALSE)
