#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod4.R')
source('../lib/libgraph.R')

# ===========================================
# Main

USEAGE <- strwrap("Useage: [machines file] [operation] [i2 type]
                  [data size (GB)] [data points] <nth record> <subset size>")
# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 5 && length(args) != 7) {
  stop(USEAGE)
} else if (length(args) == 7 && args[2] != "nth") {
  stop(USEAGE)
}

machines  <- loadMachines(args[1])
operation <- args[2]
machine   <- filter(machines, type==args[3])
data      <- as.numeric(args[4]) * HD_GB
points    <- as.numeric(args[5])

# Validate arguments
if (nrow(machine) == 0) {
  stop("Unknown machine type")
}

start <- ceiling((data * m4.DATA_MULT) / (machine$disk.size * machine$disks))
range <- start:(start+points)

if (operation == "readall") {
  operation <- "ReadAll"
  preds <- genPoints(range, function(x) m4.allModel(machine, x, data))
} else if (operation == "first") {
  operation <- "FirstRecord"
  preds <- genPoints(range, function(x) m4.firstModel(machine, x, data))
} else if (operation == "nth") {
  operation <- "NthRecord"
  nrecs   <- data / REC_SIZE
  nth     <- as.numeric(args[6])
  nthSize <- as.numeric(args[7])
  if (nth >= nrecs | nth < 0) {
    stop("N'th record is outside the data size")
  } else if ((nth + nthSize) > nrecs) {
    stop("Subset range is outside the data size")
  } else if (nthSize < 1) {
    stop("Subset size must be greater than zero")
  }
  preds <- genPoints(range, function(x) m4.nthModel(machine, x, data,
                                                    nth, nthSize))
} else if (operation == "cdf") {
  operation <- "CDF"
  preds <- genPoints(range, function(x) m4.cdfModel(machine, x, data))
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
