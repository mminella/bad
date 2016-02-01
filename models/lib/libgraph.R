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
    # Construct graph
    g <- ggplot(d, aes(clarity, x=xv, y=yv, group=variable, colour=variable)) +
      geom_line() +
      ggtitle(title) +
      xlab("Cluster Size") +
      ylab(y1)

    # Output PDF
    pdf(file)
    print(g)
    dev.off()
  }
}

mkFacetGraph <- function(d, title, file="graph.pdf", aaes=aes(x=xv, y=yv)) {
  if (is.data.frame(d) & nrow(d) > 0) {
    # Set panel ordering and title
    d$variable <- factor(d$variable, levels=c("time", "cost"))
    d$variable <- revalue(d$variable, c("time"="Time (min)", "cost"="Cost ($)"))

    # Construct graph
    g <- ggplot(d, aaes) +
      geom_line() +
      facet_grid(variable ~ ., scale="free", switch="y") +
      ggtitle(title) +
      xlab("Cluster Size (# i2.xlarge nodes)") +
      ylab("") +
      theme(legend.position="top", legend.title=element_blank())
      # theme(plot.margin=unit(c(0,0.5,0,0), units="lines"))

    # Output PDF
    pdf(file)
    print(g)
    dev.off()
  }
}
