#!/usr/bin/env Rscript
source('lib/libmodels.R')
source('method1/libmethod1.R')
source('method4/libmethod4.R')
suppressMessages(library(dplyr))
library(ggplot2)
library(ggthemr)
library(reshape2)

# ===========================================
# Graphing

# for color scheme list -- https://github.com/cttobin/ggthemr
ggthemr('fresh')

genPoints <- function(range, f) {
  do.call("rbind", lapply(range, f))
}

mkFacetGraph <- function(d, title) {
  if (is.data.frame(d) & nrow(d) > 0) {
    pdf("graph.pdf")
    d1 <- filter(d, variable=="cost")
    d1$panel <- "Cost ($)"
    d2 <- filter(d, variable=="time")
    d2$panel <- "Time (min)"

    g <- ggplot(data=d, mapping=aes(x=xv, y=yv, group=strategy, color=strategy))
    g <- g + facet_grid(panel~., scale="free")
    g <- g + layer(data=d1, geom=c("line"), stat="identity")
    g <- g + layer(data=d2, geom=c("line"), stat="identity")
    g <- g + ggtitle(title)
    g <- g + xlab("Cluster Size")
    g <- g + ylab("")
    print(g)
    dev.off()
  }
}

# ===========================================
# Main

USEAGE <- strwrap("Useage: [machines file] [operation] [client i2 type]
                   [i2 type] [data size (GB)] [data points]
                   (<nth record> | <cdf points> | <reservoir k>)")
# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 6 && length(args) != 7) {
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

m1.start <- ceiling(data / (machine$disk.size * machine$disks))
m1.range <- m1.start:(m1.start+points)

m4.start <- ceiling((data * m4.DATA_MULT) / (machine$disk.size * machine$disks))
m4.range <- m4.start:(m4.start+points)

if (operation == "readall") {
  operation <- "ReadAll"
  m1.preds <- genPoints(m1.range,
                        function(x) m1.readAll(client, machine, x, data))
  m4.preds <- genPoints(m4.range,
                        function(x) m4.readAll(machine, x, data))
} else if (operation == "first") {
  operation <- "FirstRecord"
  m1.preds <- genPoints(m1.range,
                        function(x) m1.firstRec(client, machine, x, data))
  m4.preds <- genPoints(m4.range,
                        function(x) m4.firstRec(machine, x, data))
} else if (operation == "nth") {
  operation <- "NthRecord"
  nrecs <- data / REC_SIZE
  nth   <- as.numeric(args[7])
  if (nth >= nrecs) {
    stop("N'th record is outside the data size")
  }
  m1.preds <- genPoints(m1.range,
                        function(x) m1.readRange(client, machine, x, data, nth))
  m4.preds <- genPoints(m4.range,
                        function(x) m4.readRange(machine, x, data, nth))
} else if (operation == "cdf") {
  operation <- "CDF"
  m1.preds <- genPoints(m1.range,
                        function(x) m1.cdf(client, machine, x, data))
  m4.preds <- genPoints(m4.range,
                        function(x) m4.cdf(machine, x, data))
} else if (operation == "reservoir") {
  kSamples <- as.numeric(args[7])
  operation <- "Reservoir Sampling"
  m1.preds <- genPoints(m1.range,
                        function(x) m1.reservoir(client, machine, x, data, kSamples))
  m4.preds <- genPoints(m4.range,
                        function(x) m4.reservoir(client, machine, x, data, T, kSamples))
}

preparePoints <- function(preds, name) {
  select(preds, nodes, time=time.total, cost) %>%
  mutate(time=time / M, strategy=name) %>%
  melt(id.vars=c("nodes", "strategy"), variable.name="variable", value.name="yv") %>%
  select(xv=nodes, strategy, variable, yv)
}

m1.points <- preparePoints(m1.preds, "Linear Scan")
m4.points <- preparePoints(m4.preds, "Shuffle All")
points    <- rbind(m1.points, m4.points)
title     <- paste(client$type, "Client <->", machine$type,
                  "Cluster:", operation, "-", data / HD_GB, "GB")
mkFacetGraph(points, title)
