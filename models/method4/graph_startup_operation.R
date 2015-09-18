#!/usr/bin/Rscript
source('./libmethod4.R')
library(ggplot2)
library(ggthemr)
library(reshape2)

# ===========================================
# Main

USEAGE <- strwrap("Useage: [machines file] [operation]
                   [i2 type] [data size (GB)] [data points] <nth record>")
# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 5 && length(args) != 6) {
  stop(USEAGE)
} else if (length(args) == 6 && args[2] != "nth") {
  stop(USEAGE)
}

machines  <- loadMachines(args[1])
operation <- args[2]
machine   <- filter(machines, type==args[3])
data      <- as.numeric(args[4]) * HD_GB
points    <- as.numeric(args[5])

# Validate arguments
if (nrow(machine) == 0) {
  stop("Unknown machine type")
}

# ===========================================
# Graphing

# for color scheme list -- https://github.com/cttobin/ggthemr
ggthemr('fresh')

mkGraph <- function(file, data, title, xl, yl) {
  if (is.data.frame(data) & nrow(data) > 0) {
    pdf(file)
    g <- ggplot(data, aes(clarity, x=xv, y=yv, group=variable, colour=variable))
    g <- g + geom_line()
    g <- g + ggtitle(title)
    g <- g + xlab(xl)
    g <- g + ylab(yl)
    print(g)
    dev.off()
  }
}

start <- ceiling(data / (machine$disk.size * machine$disks))
range <- start:(start+points)

if (operation == "readall") {
  operation <- "ReadAll"
  preds <- do.call("rbind",
                   lapply(range,
                          function(x) allModel(machine, x, data)))
} else if (operation == "first") {
  operation <- "FirstRecord"
  preds <- do.call("rbind",
                   lapply(range,
                          function(x) firstModel(machine, x, data)))
} else if (operation == "nth") {
  operation <- "NthRecord"
  nrecs <- data / REC_SIZE
  nth   <- as.numeric(args[7])
  if (nth >= nrecs) {
    stop("N'th record is outside the data size")
  }
  preds <- do.call("rbind",
                   lapply(range,
                          function(x) nthModel(machine, x, data, nth)))
} else if (operation == "cdf") {
  operation <- "CDF"
  preds <- do.call("rbind",
                   lapply(range,
                          function(x) cdfModel(machine, x, data)))
}

# read all vs cost
title <- paste("ShuffleAll:", machine$type, "Cluster:",
               operation, "-", data / HD_GB, "GB")
points <-
  select(preds, nodes, startup=time.startup, operations=time.op) %>%
  mutate(startup=startup/HR, operations=operations/HR) %>%
  melt(id.vars=c("nodes"), variable.name="variable", value.name="yv") %>%
  select(xv=nodes, variable, yv)
mkGraph("graph.pdf", points, title, "Cluster Size", "Time (hr)")
