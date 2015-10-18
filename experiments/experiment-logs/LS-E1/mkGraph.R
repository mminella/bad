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
      geom_point(data=d2, size=3, aes(colour=variable)) +
      guides(colour=guide_legend(override.aes=list(shape=c(16,NA),
                                                   linetype=c(0,1))))
    print(g)
    dev.off()
  } else {
    stop("invalid data to graph")
  }
}

facetGraph <- function(d, fout, title, y1, xl) {
  if (is.data.frame(d) & nrow(d) > 0) {
    d1o <- filter(d, variable=="cost", type=="predicted")
    d1$panel <- "Cost ($)"
    d2 <- filter(d, variable=="time", type=="predicted")
    d2$panel <- "Time (min)"

    pdf(file)
    g <- ggplot(data=d, mapping=aaes)
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
# main

USEAGE <- strwrap("Useage: [csv file]")

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) != 1) {
  stop(USEAGE)
}

df <- read.delim(args[[1]], header=T, sep=',', strip.white=T, comment.char='#')
df <- melt(df, id.vars=c('size','type'))
df$type <- revalue(df$type, c("o"="observed", "p"="predicted"))

print(head(df))

# lineDotGraph(df, "graph.pdf", "ShuffleAll: i2.1x Cluster - ReadAll - 600GB",
#                  "Total Time (s)", "Cluster Size (nodes)",
#                  "predicted", "observed")
