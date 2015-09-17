#!/usr/bin/Rscript
source('./libmethod1.R')

# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 5) {
  stop(strwrap("Usage: [machines file] [client i2 type]
               [i2 type] [nodes] [data size (GB)]"))
}

machines <- loadMachines(args[1])
client   <- filter(machines, type==args[2])
machine  <- filter(machines, type==args[3])
nodes    <- as.numeric(args[4])
data     <- as.numeric(args[5]) * HD_GB

# Validate arguments
if (nrow(client) == 0) {
  stop("Unknown client machine type")
} else if (nrow(machine) == 0) {
  stop("Unknown node machine type")
}

# CDF cost
cdfCost <- allModel(client, machine, nodes, data)

mutate(cdfCost, operation="cdf") %>%
  select(operation, nodes,
         time.total, time.min, time.hr, time.disk, time.net,
         cost)

