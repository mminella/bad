#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod1.R')
source('../lib/libgraph.R')

# ===========================================
# Main

USEAGE <- strwrap("Useage: [machines file] [operation] [client i2 type]
                   [i2 type] [data size (GB)] [data points] <nth record>
                   <subset size>")
# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 6 && length(args) != 8) {
  stop(USEAGE)
} else if (length(args) == 8 && args[2] != "nth") {
  stop(USEAGE)
}

machines  <- loadMachines(args[1])
operation <- args[2]
client    <- filter(machines, type==args[3])
machine   <- filter(machines, type==args[4])
data      <- as.numeric(args[5]) * HD_GB
points    <- as.numeric(args[6])

# Validate arguments
if (nrow(client) == 0) {
  stop("Unknown client machine type")
} else if (nrow(machine) == 0) {
  stop("Unknown node machine type")
}

start <- ceiling(data / (machine$disk.size * machine$disks))
range <- start:(start+points)

if (operation == "readall") {
  operation <- "ReadAll"
  preds <- genPoints(range, function(x) m1.allModel(client, machine, x, data))
} else if (operation == "first") {
  operation <- "FirstRecord"
  preds <- genPoints(range, function(x) m1.firstModel(client, machine, x, data))
} else if (operation == "nth") {
  operation <- "NthRecord"
  nrecs <- data / REC_SIZE
  nth   <- as.numeric(args[7])
  nthSize <- as.numeric(args[8])
  if (nth >= nrecs | nth < 0) {
    stop("N'th record is outside the data size")
  } else if ((nth + nthSize) > nrecs) {
    stop("Subset range is outside the data size")
  } else if (nthSize < 1) {
    stop("Subset size must be greater than zero")
  }
  preds <- genPoints(range, function(x) m1.nthModel(client, machine, x, data,
                                                    nth, nthSize))
} else if (operation == "cdf") {
  operation <- "CDF"
  preds <- genPoints(range, function(x) m1.cdfModel(client, machine, x, data))
}

# disk vs network
title <- paste("LinearScan:", client$type, "Client <->", machine$type,
               "Cluster:", operation, "-", data / HD_GB, "GB")
points <-
  select(preds, nodes, time.disk, time.net) %>%
  mutate(time.disk=time.disk/M, time.net=time.net/M) %>%
  melt(id.vars=c("nodes"), variable.name="variable", value.name="yv") %>%
  select(xv=nodes, variable, yv)

mkGraph(points, title, "Time (min)")
