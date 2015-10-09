#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod4.R')

# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 11) {
  stop(strwrap("Usage: [machines file] [client i2 type] [i2 type] [start nodes]
               [node points] [data size (GB)] [one client?] [nth record]
               [subset size] [cdf points] [reservoir k]"))
}

machines  <- loadMachines(args[1])
client    <- filter(machines, type==args[2])
machine   <- filter(machines, type==args[3])
nodes     <- as.numeric(args[4])
points    <- as.numeric(args[5])
data      <- as.numeric(args[6]) * HD_GB
oneC      <- as.logical(args[7])
nth       <- as.numeric(args[8])
nthSize   <- as.numeric(args[9])
cdfPoints <- as.numeric(args[10])
kSamples  <- as.numeric(args[11])
nrecs     <- data / REC_SIZE

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
  stop("Unknown machine type")
} else if ( points < 1 ) {
  stop("Node points must be greater than zero")
}

genAllModels <- function(n) {
  rbind(
    m4.readAll(client, machine, n, data, oneC),
    m4.firstRec(client, machine, n, data, oneC),
    m4.readRange(client, machine, n, data, oneC, nth, nthSize),
    m4.cdf(client, machine, n, data, oneC, cdfPoints),
    m4.reservoir(client, machine, n data, oneC, kSamples)
  )
}

minNodes <- 
  ceiling((data * m4.DATA_MULT) / (machine$disk.size * machine$disks))

if (minNodes <= nodes + points - 1) {
  start <- max(minNodes, nodes)
  range <- start:(nodes + points - 1)

  options( width=200 )
  print(genPoints(range, genAllModels), row.names=FALSE)
}
