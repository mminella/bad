#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod1.R')
source('../lib/libgraph.R')

# ===========================================
# Arguments

opt      <- m1.parseArgs(graph=T)
machines <- loadMachines(opt$machines)
client   <- filter(machines, type==opt$client)
machine  <- filter(machines, type==opt$node)
data     <- opt$data * HD_GB

# ===========================================
# Main

minNodes <- m1.minNodes(machine, data)
maxNodes <- opt$'min-cluster' + opt$'cluster-points' - 1
range    <- minNodes:maxNodes

if (opt$operation == "all") {
  operation <- "ReadAll"
  preds <- genPoints(range,
                     function(x) m1.readAll(client, machine, x, data))
} else if (opt$operation == "first") {
  operation <- "ReadFirst"
  preds <- genPoints(range,
                     function(x) m1.readFirst(client, machine, x, data))
} else if (opt$operation == "nth") {
  operation <- "ReadRange"
  preds <- genPoints(range,
                     function(x) m1.readRange(client, machine, x, data,
                                              opt$'range-start',
                                              opt$'range-size'))
} else if (opt$operation == "cdf") {
  operation <- "CDF"
  preds  <- genPoints(range,
                      function(x) m1.cdf(client, machine, x, data, opt$cdf))
} else if (opt$operation == "reservoir") {
  operation <- "Reservoir Sampling"
  preds  <- genPoints(range,
                      function(x) m1.reservoir(client, machine, x, data,
                                               opt$reservoir))
}

# read all vs cost
title <- paste("Linear Scan:", client$type, "<->", machine$type, "Cluster:",
               operation, "-", data / HD_GB, "GB")
points <-
  select(preds, nodes, time=time.total, cost) %>%
  mutate(time=time / M) %>%
  melt(id.vars=c("nodes"), variable.name="variable", value.name="yv") %>%
  select(xv=nodes, variable, yv)

mkFacetGraph(points, title, file=opt$file)
