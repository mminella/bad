#!/usr/bin/env Rscript
suppressMessages(library(dplyr))
library(ggplot2)
library(ggthemr)
library(reshape2)

# ===========================================
# Graphing

# for color scheme list -- https://github.com/cttobin/ggthemr
ggthemr('fresh')

mkGraph <- function(d, title, y1, file="graph.pdf") {
  if (is.data.frame(d) & nrow(d) > 0) {
    pdf(file)
    g <- ggplot(d, aes(clarity, x=xv, y=yv, group=variable, colour=variable))
    g <- g + geom_line()
    g <- g + ggtitle(title)
    g <- g + xlab("Cluster Size")
    g <- g + ylab(y1)
    print(g)
    dev.off()
  }
}

mkFacetGraph <- function(d, title, file="graph.pdf", aaes=aes(x=xv, y=yv)) {
  if (is.data.frame(d) & nrow(d) > 0) {
    pdf(file)
    d1 <- filter(d, variable=="time")
    d1$panel <- "Time (min)"
    d2 <- filter(d, variable=="cost")
    d2$panel <- "Cost ($)"

    g <- ggplot(data=d, mapping=aaes) +
      facet_grid(panel~., scale="free") +
      layer(data=d1, geom=c("line"), stat="identity") +
      layer(data=d2, geom=c("line"), stat="identity") +
      ggtitle(title) +
      xlab("Cluster Size (# i2.xlarge nodes)") +
      ylab("") +
      theme(legend.position="top", legend.title=element_blank())
    print(g)
    dev.off()
  }
}
