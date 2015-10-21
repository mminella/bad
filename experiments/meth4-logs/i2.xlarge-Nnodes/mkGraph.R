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
    g <- ggplot(d, aes(clarity, x=xv, y=yv, group=variable, colour=variable)) +
      geom() +
      ggtitle(title) +
      xlab(xl) +
      ylab(yl)
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
  g <- function() { geom_bar(stat='identity', position='dodge') }
  mkGraph(d, fout, g, title, yl, xl)
}

lineDotGraph <- function(d, fout, title, yl, xl, lineVar, dotVar) {
  if (is.data.frame(d) & nrow(d) > 0) {
    d1 <- filter(d, variable==lineVar)
    d2 <- filter(d, variable==dotVar)
    pdf(fout)
    g <- ggplot(d1, aes(clarity, x=xv, y=yv)) +
      geom_line(aes(colour=variable)) +
      ggtitle(title) +
      xlab(xl) +
      ylab(yl) +
      geom_point(data=d2, size=4, aes(colour=variable)) +
      guides(colour=guide_legend(override.aes=list(shape=c(16,NA),
                                                   linetype=c(0,1))))
    print(g)
    dev.off()
  } else {
    stop("invalid data to graph")
  }
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

fout <- "graph.pdf"
lineDotGraph(df, fout, "ShuffleAll: i2.1x Cluster - ReadAll - 600GB",
                 "Total Time (s)", "Cluster Size (nodes)",
                 "predicted", "observed")

system(paste("pdfcrop", fout, fout))
