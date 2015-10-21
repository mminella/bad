#!/usr/bin/env Rscript
library(methods)
source('lib/libmodels.R')
source('lib/libgraph.R')
source('method1/libmethod1.R')
source('method2/libmethod2.R')
source('method4/libmethod4.R')

# ===========================================
# Arguments

opt       <- m4.parseArgs(graph=T)
machines <- loadMachines(opt$machines)
client   <- filter(machines, type==opt$client)
machine  <- filter(machines, type==opt$node)
data     <- opt$data * HD_GB
oneC     <- opt$'one-client'
maxNodes <- opt$'min-cluster' + opt$'cluster-points' - 1

# cluster ranges
m1.range <- m1.minNodes(machine,data):maxNodes
m2.range <- m2.minNodes(machine,data):maxNodes
m4.range <- m4.minNodes(machine,data):maxNodes
if ( m1.range[[1]] > maxNodes || m2.range[[1]] > maxNodes || m4.range[[1]] > maxNodes ) {
  stop("Not a valid cluster range size, increase cluster-points")
}

# ===========================================
# Main

if (opt$operation == "all") {
  operation <- "ReadAll"
  m1.preds <- genPoints(m1.range,
                        function(x) m1.readAll(client, machine, x, data))
  m2.preds <- genPoints(m2.range,
                        function(x) m2.readAll(client, machine, x, data, oneC))
  m4.preds <- genPoints(m4.range,
                        function(x) m4.readAll(client, machine, x, data, oneC))
} else if (opt$operation == "first") {
  operation <- "ReadFirst"
  m1.preds <- genPoints(m1.range,
                        function(x) m1.readFirst(client, machine, x, data))
  m2.preds <- genPoints(m2.range,
                        function(x) m2.readFirst(client, machine, x, data,
                                                 oneC))
  m4.preds <- genPoints(m4.range,
                        function(x) m4.readFirst(client, machine, x, data,
                                                 oneC))
} else if (opt$operation == "nth") {
  operation <- "ReadRange"
  m1.preds <- genPoints(m1.range,
                        function(x) m1.readRange(client, machine, x, data,
                                                 opt$'range-start',
                                                 opt$'range-size'))
  m2.preds <- genPoints(m2.range,
                        function(x) m2.readRange(client, machine, x, data, oneC,
                                                 opt$'range-start',
                                                 opt$'range-size'))
  m4.preds <- genPoints(m4.range,
                        function(x) m4.readRange(client, machine, x, data, oneC,
                                                 opt$'range-start',
                                                 opt$'range-size'))
} else if (opt$operation == "cdf") {
  operation <- "CDF"
  m1.preds <- genPoints(m1.range,
                        function(x) m1.cdf(client, machine, x, data, opt$cdf))
  m2.preds <- genPoints(m2.range,
                        function(x) m2.cdf(client, machine, x, data, oneC,
                                           opt$cdf))
  m4.preds <- genPoints(m4.range,
                        function(x) m4.cdf(client, machine, x, data, oneC,
                                           opt$cdf))
} else if (opt$operation == "reservoir") {
  operation <- "Reservoir Sampling"
  m1.preds <- genPoints(m1.range,
                        function(x) m1.reservoir(client, machine, x, data,
                                                 opt$reservoir))
  m2.preds <- genPoints(m2.range,
                        function(x) m2.reservoir(client, machine, x, data, oneC,
                                                 opt$reservoir))
  m4.preds <- genPoints(m4.range,
                        function(x) m4.reservoir(client, machine, x, data, oneC,
                                                 opt$reservoir))
}

preparePoints <- function(preds, name) {
  select(preds, nodes, time=time.total, cost) %>%
  mutate(time=time / M, strategy=name) %>%
  melt(id.vars=c("nodes", "strategy"),
       variable.name="variable", value.name="yv") %>%
  select(xv=nodes, strategy, variable, yv)
}

m1.points <- preparePoints(m1.preds, "Linear Scan")
m2.points <- preparePoints(m2.preds, "Local Index")
m4.points <- preparePoints(m4.preds, "Shuffle All")
points    <- rbind(m1.points, m2.points, m4.points)
title     <- paste(client$type, "Client <->", machine$type,
                  "Cluster:", operation, "-", data / HD_GB, "GB")
mkFacetGraph(points, title, file=opt$file,
             aaes=aes(x=xv, y=yv, group=strategy, color=strategy))
