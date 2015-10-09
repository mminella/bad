#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod4.R')
source('../lib/libgraph.R')

# ===========================================
# Arguments

opt      <- m4.parseArgs(graph=T)
machines <- loadMachines(opt$machines)
client   <- filter(machines, type==opt$client)
machine  <- filter(machines, type==opt$node)
data     <- opt$data * HD_GB
oneC     <- opt$'one-client'

# ===========================================
# Main

minNodes <- ceiling((data * m4.DATA_MULT)/(machine$disk.size * machine$disks))
maxNodes <- opt$'min-cluster' + opt$'cluster-points' - 1
range    <- minNodes:maxNodes

if (opt$operation == "all") {
  operation <- "ReadAll"
  preds <- genPoints(range,
                     function(x) m4.readAll(client, machine, x, data, oneC))
} else if (opt$operation == "first") {
  operation <- "ReadFirst"
  preds <- genPoints(range,
                     function(x) m4.readFirst(client, machine, x, data, oneC))
} else if (opt$operation == "nth") {
  operation <- "ReadRange"
  preds <- genPoints(range,
                     function(x) m4.readRange(client, machine, x, data, oneC,
                                              opt$'range-start',
                                              opt$'range-size'))
} else if (opt$operation == "cdf") {
  operation <- "CDF"
  preds  <- genPoints(range,
                      function(x) m4.cdf(client, machine, x, data, oneC,
                                         opt$cdf))
} else if (opt$operation == "reservoir") {
  operation <- "Reservoir Sampling"
  preds  <- genPoints(range,
                      function(x) m4.reservoir(client, machine, x, data, oneC,
                                               opt$reservoir))
}

# read all vs cost
title <- paste("ShuffleAll:", machine$type, "Cluster:",
               operation, "-", data / HD_GB, "GB")
points <-
  select(preds, nodes, startup=time.startup, operations=time.op) %>%
  mutate(startup=startup/M, operations=operations/M) %>%
  melt(id.vars=c("nodes"), variable.name="variable", value.name="yv") %>%
  select(xv=nodes, variable, yv)

mkGraph(points, title, "Time (min)", file=opt$file)
