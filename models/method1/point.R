#!/usr/bin/env Rscript
source('../lib/libmodels.R')
source('./libmethod1.R')

# ===========================================
# Arguments

opt      <- m1.parseArgs()
machines <- loadMachines(opt$machines)
client   <- filter(machines, type==opt$client)
machine  <- filter(machines, type==opt$node)
data     <- opt$data * HD_GB

# ===========================================
# Main

genAllModels <- function(n) {
  rbind(
    m1.readAll(client, machine, n, data),
    m1.readFirst(client, machine, n, data),
    m1.readRange(client, machine, n, data,
                 opt$'range-start', opt$'range-size'),
    m1.cdf(client, machine, n, data, opt$cdf),
    m1.reservoir(client, machine, n, data, opt$reservoir)
  )
}

minNodes <- ceiling(data/(machine$disk.size * machine$disks))
maxNodes <- opt$'min-cluster' + opt$'cluster-points' - 1
if (minNodes <= maxNodes) {
  range <- max(minNodes,opt$'min-cluster'):maxNodes
  options(width=200)
  print(genPoints(range, genAllModels), row.names=FALSE)
}
