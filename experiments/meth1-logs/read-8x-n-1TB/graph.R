#!/usr/bin/Rscript
library(data.table)
library(dplyr)
library(ggplot2)
library(ggthemr)
library(reshape2)
library(stringi)

# ===========================================
# Constants

KEYSIZE <- 10
VALSIZE <- 90
RECSIZE <- KEYSIZE + VALSIZE
MB <- 1024 * 1024
GB <- MB * 1000
MS <- 1000


# ===========================================
# Helpers

fromRecs <- function(x) {
  x * RECSIZE
}

toRecs <- function(x) {
  x / RECSIZE
}

toGB <- function(x) {
  x / GB
}

fromGB <- function(x) {
  x * GB
}

twoDec <- function(x) {
  format(round(x, 2), nsmall=2)
}

noDec <- function(x) {
  format(round(x), nsmall=0)
}

extractOp <- function(df, opName, ...) {
  filter(df, op==opName) %>% select(id, ...)
}

# ===========================================
# Load data files

loadFile <- function(f) {
  if (!file.exists(f)) {
    stop("File '", f, "' doesn't exist")
  }

  # extract ID from file-path
  id <- stri_split_fixed(f, '/')[[1]][1]

  df <- read.delim(f, header=F, sep=',', strip.white=TRUE, comment.char="#")
  df <- mutate(df, id=id)
  rename(df, op=V1)
}


# ===========================================
# Model

DISKIO <- 450 * MB
NETIO <- c("1"=82, "2"=125, "4"=250, "8"=1200)

diskNETIO <- function(x) {
  NETIO[toString(x)] * MB
}


# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) < 1) {
  stop("Usage: <data file> [<data file>...] ")
}

# grab all files
df <- bind_rows(lapply(args, loadFile))

# extract components
cmdRead <- extractOp(df, "cmd-read", total=V2)
disks <- extractOp(df, "disks", disks=V2)
chunks <- extractOp(df, "chunk-size", size.chunk=V2) %>%
  mutate(size.chunk=fromRecs(size.chunk))
sizes <-
  extractOp(df, "size", node=V2, size=V4) %>%
  group_by(id) %>%
  summarise(nodes=n(),
            size.total=sum(fromRecs(size)),
            size.node=min(fromRecs(size)))

# summarise all clusters and predict running time
clusters <-
  inner_join(sizes, chunks, by="id") %>%
  inner_join(disks, by="id") %>%
  mutate(
    scans   = ceiling(size.node / size.chunk),
    netio   = as.numeric(lapply(disks, diskNETIO)),
    p.read  = size.node / (DISKIO * disks) * scans,
    p.net   = size.total / netio,
    p.total = p.read + p.net
  )

# ===========================================
# Graphing

# for color scheme list -- https://github.com/cttobin/ggthemr
ggthemr('fresh')

mkGraph <- function(file, data, title, xl, yl) {
  if (is.data.frame(data) & nrow(data) > 0) {
    pdf(file)
    g <- ggplot(data, aes(clarity, x=xv, y=yv, group=type, colour=type))
    g <- g + geom_line()
    g <- g + ggtitle(title)
    g <- g + xlab(xl)
    g <- g + ylab(yl)
    print(g)
    dev.off()
  }
}

# graph observed vs predicted total running times
totalP <-
  select(clusters, id, total=p.total) %>%
  mutate(type="predicted")
totalO <- mutate(cmdRead, total=total/MS, type="observed")

totalAll <- rbind(totalP, totalO) %>%
  select(xv=id, yv=total, type)

mkGraph("total.pdf", totalAll, "Linear Scan: Read all Records",
        "I2.8xlarge Cluster Size (# nodes)", "Time (s)")
