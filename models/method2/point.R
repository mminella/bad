#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod2.R')

# ===========================================
# Arguments

opt      <- m2.parseArgs()
machines <- loadMachines(opt$machines)
client   <- filter(machines, type==opt$client)
machine  <- filter(machines, type==opt$node)
data     <- opt$data * HD_GB
oneC     <- opt$'one-client'

# ===========================================
# Main

genAllModels <- function(n) {
  rbind(
    m2.readAll(client, machine, n, data, oneC),
    m2.readFirst(client, machine, n, data, oneC),
    m2.readRange(client, machine, n, data, oneC,
                 opt$'range-start', opt$'range-size'),
    m2.cdf(client, machine, n, data, oneC, opt$cdf),
    m2.reservoir(client, machine, n, data, oneC, opt$reservoir)
  )
}

minNodes <- m2.minNodes(machine, data)
maxNodes <- opt$'min-cluster' + opt$'cluster-points' - 1
if (minNodes <= maxNodes) {
  range <- max(minNodes,opt$'min-cluster'):maxNodes
  options(width=200)
  print(genPoints(range, genAllModels), row.names=FALSE)
}
