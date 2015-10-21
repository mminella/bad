#!/usr/bin/env Rscript
library("plyr")
suppressMessages(library(dplyr))
library("ggplot2")
library("ggthemr")
library("grid")
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
    g <- ggplot(d, aes(x=xv, y=yv, group=variable, colour=variable)) +
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

    g <- ggplot(d1, aes(x=xv, y=yv)) +
      geom_line(aes(colour=variable)) +
      ggtitle(title) +
      xlab(xl) +
      ylab(yl) +
      geom_point(data=d2, size=3, aes(colour=variable)) +
      guides(colour=guide_legend(override.aes=list(shape=c(16,NA),
                                                   linetype=c(0,1))))
    pdf(fout)
    print(g)
    dev.off()
  } else {
    stop("invalid data to graph")
  }
}

facetGraph <- function(d, fout, title, xl) {
  if (is.data.frame(d) & nrow(d) > 0) {
    cost_p <- filter(d, variable=="cost", type=="Predicted")
    cost_o <- filter(d, variable=="cost", type=="Observed")
    cost_p$panel <- "Cost ($)"
    cost_o$panel <- "Cost ($)"
    time_p <- filter(d, variable=="time", type=="Predicted")
    time_o <- filter(d, variable=="time", type=="Observed")
    time_p$panel <- "Time (min)"
    time_o$panel <- "Time (min)"

    g <- ggplot(data=d, mapping=aes(x=xv, y=yv, color=tt, group=tt)) +
      facet_grid(panel~., scale="free") +
      layer(data=time_p, geom=c("line"), stat="identity") +
      layer(data=time_o, geom=c("point"), stat="identity", size=3) +
      layer(data=cost_p, geom=c("line"), stat="identity") +
      layer(data=cost_o, geom=c("point"), stat="identity", size=3) +
      guides(colour=guide_legend(override.aes=list(shape=c(16,NA),
                                                   linetype=c(0,1)),
                                 nrow=2)) +
      theme(legend.position="top", legend.title=element_blank()) +
      ggtitle(title) +
      xlab(xl) +
      ylab("") +
			scale_x_continuous(expand=c(0,0), limits=c(0,10.5)) +
			expand_limits(x=0, y=0)

    pdf(fout)
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

df <- read.delim(args[[1]], header=T, sep=',', strip.white=T, comment.char='#') %>%
      mutate(time=time/60) %>%
      melt(id.vars=c('size','method', 'type'))

df$type <- revalue(df$type, c("o"="Observed", "p"="Predicted"))
df$method <- revalue(df$method, c("ls"="LinearScan", "sa"="ShuffleAll"))

df <- select(df, method, type, variable, xv=size, yv=value) %>%
      mutate(tt=paste(method, type, sep=' '))

print(head(df))

fout <- "graph.pdf"
facetGraph(df,
           fout,
           "",
           "Cluster Size (# i2.xlarge nodes)")

# lineDotGraph(df,
#              fout,
#              "ShuffleAll: i2.1x Cluster - ReadAll - 600GB",
#              "Total Time (s)",
#              "Cluster Size (# i2.xlarge nodes)",
#              "predicted", "observed")

# system(paste("pdfcrop", fout, fout))
