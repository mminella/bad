#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod4.R')

# ===========================================
# Arguments

opt      <- m4.parseArgs()
machines <- loadMachines(opt$machines)
client   <- filter(machines, type==opt$client)
machine  <- filter(machines, type==opt$node)
data     <- opt$data * HD_GB
oneC     <- opt$'one-client'

# ===========================================
# Main

genAllModels <- function(n) {
  rbind(
    m4.readAll(client, machine, n, data, oneC),
    m4.readFirst(client, machine, n, data, oneC),
    m4.readRange(client, machine, n, data, oneC,
                 opt$'range-start', opt$'range-size'),
    m4.cdf(client, machine, n, data, oneC, opt$cdf),
    m4.reservoir(client, machine, n, data, oneC, opt$reservoir)
  )
}

minNodes <- m4.minNodes(machine, data)
maxNodes <- opt$'min-cluster' + opt$'cluster-points' - 1
if (minNodes <= maxNodes) {
  range <- max(minNodes,opt$'min-cluster'):maxNodes
  options(width=200)
  print(genPoints(range, genAllModels), row.names=FALSE)
}
