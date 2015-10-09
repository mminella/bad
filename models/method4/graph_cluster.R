#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod4.R')
source('../lib/libgraph.R')

# ===========================================
# Main

USEAGE <- strwrap("Useage: [machines file] [operation] [client i2 type]
                  [i2 type] [data size (GB)] [one client?] [data points]
                  (<cdf points> | <reservoir k> | <nth record> <subset size>)")
# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 7 && length(args) != 8 && length(args) != 9) {
  stop(USEAGE)
} else if (length(args) == 9 && args[2] != "nth") {
  stop(USEAGE)
} else if (length(args) == 8 && args[2] != "cdf" && args[2] != "reservoir") {
  stop(USEAGE)
}

machines  <- loadMachines(args[1])
operation <- args[2]
client    <- filter(machines, type==args[3])
machine   <- filter(machines, type==args[4])
data      <- as.numeric(args[5]) * HD_GB
oneC      <- as.logical(args[6])
points    <- as.numeric(args[7])

# Validate arguments
if (nrow(client) == 0) {
  stop("Unknown client machine type")
} else if (nrow(machine) == 0) {
  stop("Unknown machine type")
}

start <- ceiling((data * m4.DATA_MULT) / (machine$disk.size * machine$disks))
range <- start:(start+points)

if (operation == "readall") {
  operation <- "ReadAll"
  preds <- genPoints(range, function(x) m4.readAll(client, machine, x,
                                                   data, oneC))
} else if (operation == "first") {
  operation <- "FirstRecord"
  preds <- genPoints(range, function(x) m4.firstRec(client, machine, x,
                                                    data, oneC))
} else if (operation == "nth") {
  operation <- "NthRecord"
  nrecs   <- data / REC_SIZE
  nth     <- as.numeric(args[8])
  nthSize <- as.numeric(args[9])
  if (nth >= nrecs | nth < 0) {
    stop("N'th record is outside the data size")
  } else if ((nth + nthSize) > nrecs) {
    stop("Subset range is outside the data size")
  } else if (nthSize < 1) {
    stop("Subset size must be greater than zero")
  }
  preds <- genPoints(range, function(x) m4.readRange(client, machine, x,
                                                     data, oneC, nth, nthSize))
} else if (operation == "cdf") {
  operation <- "CDF"
  points <- as.numeric(args[8])
  preds  <- genPoints(range, function(x) m4.cdf(client, machine, x,
                                                data, oneC, points))
} else if (operation == "reservoir") {
  operation <- "Reservoir Sampling"
  points <- as.numeric(args[8])
  preds  <- genPoints(range, function(x) m4.reservoir(client, machine, x,
                                                      data, oneC, points))
}

# read all vs cost
title <- paste("ShuffleAll:", machine$type, "Cluster:",
               operation, "-", data / HD_GB, "GB")
points <-
  select(preds, nodes, time=time.total, cost) %>%
  mutate(time=time / M) %>%
  melt(id.vars=c("nodes"), variable.name="variable", value.name="yv") %>%
  select(xv=nodes, variable, yv)

# mkGraph(points, title, "Cost ($) / Time (min)")
mkFacetGraph(points, title)
