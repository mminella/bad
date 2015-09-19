#!/usr/bin/Rscript
library(data.table)
suppressMessages(library(dplyr))
library(methods)
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
GB <- MB * 1024
MS <- 1000
HD_GB <- 1000 * 1000 * 1000


# ===========================================
# Load data files

loadFile <- function(f) {
  if (!file.exists(f)) {
    stop("File '", f, "' doesn't exist")
  }

  # extract ID from file-path
  id <- stri_split_fixed(f, '/')[[1]][1]

  # columns
  colNames <- c('op', 'V2', 'V3', 'V4', 'V5', 'V6', 'V7')

  df <- read.delim(f, header=F, sep=',', strip.white=TRUE, comment.char="#",
                   col.names=colNames)
  mutate(df, id=id)
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

readStart <- filter(df, op=="read-start") %>% select(id, op, node=V2, seq=V3, start=V6)
netStart <- filter(df, op=="circular-read-start") %>% select(id, op, node=V2, seq=V3, start=V5)

times <- rbind(readStart, netStart)

n5 <-
  filter(times, node==5) %>%
  arrange(seq)

n5group <- group_by(n5, seq) %>%
  summarise(disk.start=min(start) / MS,
            disk.end=max(start) / MS,
            disk.time=disk.end - disk.start)

netEnd <-
  mutate(readStart, seq=seq - 1) %>%
  filter(seq > 0, node==5)

print(netEnd)

# print(n5group)
