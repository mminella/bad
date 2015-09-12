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


# ===========================================
# Load data files

loadFile <- function(f) {
  if (!file.exists(f)) {
    stop("File '", f, "' doesn't exist")
  }

  # extract ID from file-path
  id <- stri_split_fixed(f, '/')[[1]][1]

  df <- read.delim(f, header=F, sep=',', strip.white=TRUE, comment.char="#")
  df <- mutate(df, i2=id)
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

# arrange data
data <-
  filter(df, op=="read"
           | op=="network"
           | op=="chunk-size"
           | op=="disks"
           | op=="size"
           | op=="cmd-read") %>%
    group_by(i2, op) %>%
    summarise(
      disks      = sum(V2, na.rm=TRUE),
      size       = sum(fromRecs(V4), na.rm=TRUE),
      chunk.size = sum(fromRecs(V2), na.rm=TRUE),
      network    = sum(V3, na.rm=TRUE) / MS,
      read       = sum(V6, na.rm=TRUE) / MS,
      total      = sum(V2, na.rm=TRUE) / MS
    )

# summarise
sum <- data.table(
    distinct(data["i2"]),
    filter(data, op=="disks")["disks"],
    filter(data, op=="size")["size"],
    filter(data, op=="chunk-size")["chunk.size"],
    filter(data, op=="read")["read"],
    filter(data, op=="network")["network"],
    filter(data, op=="cmd-read")["total"]
  )

# add in predictions
sum <- mutate( sum,
         scans  = ceiling(size / chunk.size),
         netio  = as.numeric(lapply(disks, diskNETIO)),
         diskio = disks * DISKIO
       )

sum <- mutate(sum,
         p.read          = size / diskio * scans,
         p.network       = size / netio,
         p.total         = p.read + p.network,
         readio          = size * scans / read,
         readio.per.disk = readio / disks
       )

# ===========================================
# Graphing

# for color scheme list -- https://github.com/cttobin/ggthemr
ggthemr('fresh')

mkGraph <- function(file, data, title, xl, yl) {
  if (is.data.frame(data) & nrow(data) > 0) {
    pdf(file)
    g <- ggplot(data, aes(clarity, x=xv, y=yv, group=type, fill=type))
    g <- g + geom_bar(stat = "identity", position="dodge")
    g <- g + ggtitle(title)
    g <- g + xlab(xl)
    g <- g + ylab(yl)
    print(g)
    dev.off()
  }
}

# Total Time
total <-
  select(sum, i2, total, p.total) %>%
  rename(observed=total, predicted=p.total) %>%
  melt(id.vars=c("i2"), variable.name="type", value.name="yv") %>%
  rename(xv=i2)

mkGraph("total.pdf", total, "Time to Read all Records",
        "I2 Instance Type", "Time (s)")

# i2.xN
i2xN <- function(xn, file, title) {
  i2x <- filter(sum, i2==xn) %>%
    select(total, read, network, p.total, p.read, p.network)
  i2x <-
    melt(i2x, measure.vars=colnames(i2x),
         variable.name="operation", value.name="time") %>%
    mutate(type=ifelse(stri_startswith_fixed(operation, "p."),
                       "predicted", "observed")) %>%
    rename(xv=operation, yv=time)
  levels(i2x$xv) <-
    list(total=c("total", "p.total"),
         read=c("read", "p.read"),
         network=c("network", "p.network"))
  mkGraph(file, i2x, paste(title, "Time for all Records"),
          "Operation", "Time (s)")
}

i2xN(1, "i2.xlarge.pdf",  "I2.xlarge")
i2xN(2, "i2.2xlarge.pdf", "I2.2xlarge")
i2xN(4, "i2.4xlarge.pdf", "I2.4xlarge")
i2xN(8, "i2.8xlarge.pdf", "I2.8xlarge")

