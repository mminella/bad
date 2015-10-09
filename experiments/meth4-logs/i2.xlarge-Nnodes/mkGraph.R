#!/usr/bin/env Rscript
library("plyr")
suppressMessages(library(dplyr))
library("ggplot2")
library("ggthemr")
library("methods")
library("reshape2")
library("scales")

# ===========================================
# Graphing

# for color scheme list -- https://github.com/cttobin/ggthemr
ggthemr('fresh')

mkGraph <- function(d, fout, geom, title, yl, xl) {
  if (is.data.frame(d) & nrow(d) > 0) {
    pdf(fout)
    g <- ggplot(d, aes(clarity, x=xv, y=yv, group=variable, colour=variable))
    g <- g + geom()
    g <- g + ggtitle(title)
    g <- g + xlab(xl)
    g <- g + ylab(yl)
    print(g)
    dev.off()
  } else {
    stop("invalid data to graph")
  }
}

lineGraph <- function(d, fout, title, yl, xl) {
  mkGraph(d, fout, geom_line, title, yl, xl)
}

pointGraph <- function(d, fout, title, yl, xl) {
  mkGraph(d, fout, geom_point, title, yl, xl)
}

barGraph <- function(d, fout, title, yl, xl) {
  mkGraph(d, fout, geom_bar(stat='identity', position='dodge'), title, yl, xl)
}


# ===========================================
# main

USEAGE <- strwrap("Useage: [csv file]")

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 1) {
  stop(USEAGE)
}

df <- read.delim(args[[1]], header=T, sep=',', strip.white=T, comment.char='#')
df <- select(df, variable=type, xv=size, yv=total)
df$variable <- revalue(df$variable, c("o"="observed", "p"="predicted"))

pointGraph(df, "graph.pdf", "ShuffleAll: i2.1x Cluster - ReadAll - 600GB",
           "Total Time (s)", "Cluster Size (nodes)")
