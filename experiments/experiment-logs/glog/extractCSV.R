#!/usr/bin/env Rscript
suppressMessages(library(dplyr))
library("methods")
library("ggplot2")
library("reshape2")
library("scales")

stopIfFileNotExists <- function(f) {
  if (!file.exists(f)) {
    stop("File '", f, "' doesn't exist")
  }
}

loadAllFiles <- function(args, loader, nameFun) {
  loadM <- function(id) {
    loader(args[[id]], id, nameFun)
  }
  bind_rows(lapply(seq.int(1,length(args)), loadM))
}

loadMachines <- function(f, id, nameFun) {
  name     <- nameFun(f, id)
  allLines <- readLines(f)
  lengths  <- sapply(allLines, function(x) length(unlist(strsplit(x, ","))))
  maxLen   <- max(lengths)
  columns  <- sapply(c(1:maxLen), function(x) paste('V', x, sep=''))
  df <- read.delim(f, header=F, sep=',', strip.white=TRUE, comment.char="#",
             col.names=columns, stringsAsFactors=FALSE)
  mutate(df, id=name)
}

# ===========================================
# Main

USEAGE <- strwrap("Useage: [observed log files...]")

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) < 1) {
  stop(USEAGE)
}

# name according to folder
nameData <- function(path, id) {
  unlist(strsplit(path, '/'))[1]
}

# load all data files
check <- lapply(args, stopIfFileNotExists)
df <- loadAllFiles(args, loadMachines, nameData)

df <- filter(df, V1=='cmd-read') %>%
      select(id, V2) %>%
      mutate(time=as.numeric(V2)/1000/60,type='o') %>%
      select(id, type, time)

write.csv(df, file='out.csv', row.names=F)

