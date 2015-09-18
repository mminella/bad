#!/usr/bin/Rscript
source('../lib/libmodels.R')
source('./libmethod1.R')
source('../lib/libgraph.R')

# ===========================================
# Main

USEAGE <- strwrap("Useage: [machines file] [operation] [client i2 type]
                   [i2 type] [data size (GB)] [data points] <nth record>")
# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 6 && length(args) != 7) {
  stop(USEAGE)
} else if (length(args) == 7 && args[2] != "nth") {
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
  if (nth >= nrecs) {
    stop("N'th record is outside the data size")
  }
  preds <- genPoints(range, function(x) m1.nthModel(client, machine, x, data, nth))
} else if (operation == "cdf") {
  operation <- "CDF"
  preds <- genPoints(range, function(x) m1.cdfModel(client, machine, x, data))
}

# read all vs cost
title <- paste("LinearScan:", client$type, "Client <->", machine$type,
               "Cluster:", operation, "-", data / HD_GB, "GB")
points <-
  select(preds, nodes, time=time.total, cost) %>%
  mutate(time=time / M) %>%
  melt(id.vars=c("nodes"), variable.name="variable", value.name="yv") %>%
  select(xv=nodes, variable, yv)

# mkGraph(points, title, "Cost ($) / Time (min)")
mkFacetGraph(points, title)
