#!/usr/bin/env Rscript
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

mkGraph <- function(d, title, y1) {
  if (is.data.frame(d) & nrow(d) > 0) {
    pdf("graph.pdf")
    g <- ggplot(d, aes(clarity, x=xv, y=yv, group=variable, colour=variable))
    g <- g + geom_line()
    g <- g + ggtitle(title)
    g <- g + xlab("Cluster Size")
    g <- g + ylab(y1)
    print(g)
    dev.off()
  }
}

mkFacetGraph <- function(d, title) {
  if (is.data.frame(d) & nrow(d) > 0) {
    pdf("graph.pdf")
    d1 <- filter(d, variable=="cost")
    d1$panel <- "Cost ($)"
    d2 <- filter(d, variable=="time")
    d2$panel <- "Time (min)"

    g <- ggplot(data=d, mapping=aes(x=xv, y=yv))
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
