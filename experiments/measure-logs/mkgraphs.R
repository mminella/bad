#!/usr/bin/Rscript
library(ggplot2)
library(dplyr)
library(ggthemr)
library(reshape2)

# for color scheme list -- https://github.com/cttobin/ggthemr
ggthemr('fresh')

# We specify number of data lines, not starting offset as that can vary.
DATA_LINES <- 96
OUT_FOLDER <- "graphs"

loadFile <- function(f) {
  if (!file.exists(f)) {
    stop("File '", f, "' doesn't exist")
  }

  # we dynamically detect file length and columns to handle different versions
  # of our measurement tests.
  offset <- length(readLines(f)) - DATA_LINES
  row    <- tail(readLines(f), 1)
  ncols  <- length(strsplit(row, ',')[[1]])

  if ( ncols == 7 ) {
    cns <- c('rw', 'iomode', 'access', 'block', 'queue', 'odirect', 'mbs')
    read.delim(f, header=F, sep=',', skip=offset, col.names=cns,
               strip.white=TRUE)
  } else {
    cns <- c('rw', 'iomode', 'access', 'block', 'queue', 'mbs')
    df <- read.delim(f, header=F, sep=',', skip=offset, col.names=cns,
               strip.white=TRUE)
    odir <- data.frame( odirect=c(rep(0,16), rep(1,16), rep(0,32), rep(1,32)))
    bind_cols(df, odir)
  }
}

mkGraph <- function(file, data, title) {
  dir.create(OUT_FOLDER, showWarnings=F)
  pdf(paste(OUT_FOLDER, file, sep='/'))
  g <- ggplot(data)
  g <- g + geom_histogram(aes(x=mbs), binwidth=5)
  g <- g + geom_vline(aes(xintercept=mean(mbs)), color="red", linetype="dashed", size=1)
  g <- g + ggtitle(title)
  print(g)
  dev.off()
}

# ===========================================
# Main

# check args
args <- commandArgs(trailingOnly = T)
if (length(args) < 1) {
  stop("Usage: <data file> [<data file>...] ")
}

# process all files
df <- bind_rows(lapply(args, loadFile))

# ===========================================
# Graphs -- read, blocking

# read-sequential-blocking
seq_r <- filter( df, rw == 'r', iomode == 's', access == 's', block == 4096, odirect == 0 )
mkGraph('read-seq-block-4kb.pdf', seq_r, 'Read: sequential, blocking, 4KB, !O_DIRECT')
seq_r <- filter( df, rw == 'r', iomode == 's', access == 's', block == 1048576, odirect == 0 )
mkGraph('read-seq-block-1mb.pdf', seq_r, 'Read: sequential, blocking, 1MB, !O_DIRECT')

# read-sequential-blocking + O_DIRECT
seq_r <- filter( df, rw == 'r', iomode == 's', access == 's', block == 4096, odirect == 1 )
mkGraph('read-seq-block-4kb-o.pdf', seq_r, 'Read: sequential, blocking, 4KB, O_DIRECT')
seq_r <- filter( df, rw == 'r', iomode == 's', access == 's', block == 1048576, odirect == 1 )
mkGraph('read-seq-block-1mb-o.pdf', seq_r, 'Read: sequential, blocking, 1MB, O_DIRECT')

# read-random-blocking
seq_r <- filter( df, rw == 'r', iomode == 's', access == 'r', block == 4096, odirect == 0 )
mkGraph('read-ran-block-4kb.pdf', seq_r, 'Read: random, blocking, 4KB, !O_DIRECT')
seq_r <- filter( df, rw == 'r', iomode == 's', access == 'r', block == 1048576, odirect == 0 )
mkGraph('read-ran-block-1mb.pdf', seq_r, 'Read: random, blocking, 1MB, !O_DIRECT')

# read-random-blocking + O_DIRECT
seq_r <- filter( df, rw == 'r', iomode == 's', access == 'r', block == 4096, odirect == 1 )
mkGraph('read-ran-block-4kb-o.pdf', seq_r, 'Read: random, blocking, 4KB, O_DIRECT')
seq_r <- filter( df, rw == 'r', iomode == 's', access == 'r', block == 1048576, odirect == 1 )
mkGraph('read-ran-block-1mb-o.pdf', seq_r, 'Read: random, blocking, 1MB, O_DIRECT')


# ===========================================
# Graphs -- write, blocking

# write-sequential-blocking
seq_r <- filter( df, rw == 'w', iomode == 's', access == 's', block == 4096, odirect == 0 )
mkGraph('write-seq-block-4kb.pdf', seq_r, 'Write: sequential, blocking, 4KB, !O_DIRECT')
seq_r <- filter( df, rw == 'w', iomode == 's', access == 's', block == 1048576, odirect == 0 )
mkGraph('write-seq-block-1mb.pdf', seq_r, 'Write: sequential, blocking, 1MB, !O_DIRECT')

# write-sequential-blocking + O_DIRECT
seq_r <- filter( df, rw == 'w', iomode == 's', access == 's', block == 4096, odirect == 1 )
mkGraph('write-seq-block-4kb-o.pdf', seq_r, 'Write: sequential, blocking, 4KB, O_DIRECT')
seq_r <- filter( df, rw == 'w', iomode == 's', access == 's', block == 1048576, odirect == 1 )
mkGraph('write-seq-block-1mb-o.pdf', seq_r, 'Write: sequential, blocking, 1MB, O_DIRECT')

# write-random-blocking
seq_r <- filter( df, rw == 'w', iomode == 's', access == 'r', block == 4096, odirect == 0 )
mkGraph('write-ran-block-4kb.pdf', seq_r, 'Write: random, blocking, 4KB, !O_DIRECT')
seq_r <- filter( df, rw == 'w', iomode == 's', access == 'r', block == 1048576, odirect == 0 )
mkGraph('write-ran-block-1mb.pdf', seq_r, 'Write: random, blocking, 1MB, !O_DIRECT')

# write-random-blocking + O_DIRECT
seq_r <- filter( df, rw == 'w', iomode == 's', access == 'r', block == 4096, odirect == 1 )
mkGraph('write-ran-block-4kb-o.pdf', seq_r, 'Write: random, blocking, 4KB, O_DIRECT')
seq_r <- filter( df, rw == 'w', iomode == 's', access == 'r', block == 1048576, odirect == 1 )
mkGraph('write-ran-block-1mb-o.pdf', seq_r, 'Write: random, blocking, 1MB, O_DIRECT')

# ===========================================
# Graphs -- read, async

asyncGraph <- function(file, data, title) {
  qdGraph <- function(qd) {
    file_ <- sprintf(file, qd)
    title_ <- sprintf(title, qd)
    data_ <- filter( data, queue == qd)
    mkGraph(file_, data_, title_)
  }
  lapply(c(1,15,31,62), qdGraph)
}

# read-sequential-async
seq_r <- filter( df, rw == 'r', iomode == 'a', access == 's', block == 4096, odirect == 0 )
asyncGraph('read-seq-async-4kb-q%d.pdf', seq_r, 'Read: sequential, async, 4KB, Q%d, !O_DIRECT')
seq_r <- filter( df, rw == 'r', iomode == 'a', access == 's', block == 1048576, odirect == 0 )
asyncGraph('read-seq-async-1mb-q%d.pdf', seq_r, 'Read: sequential, async, 1MB, Q%d, !O_DIRECT')

# read-random-async
seq_r <- filter( df, rw == 'r', iomode == 'a', access == 'r', block == 4096, odirect == 0 )
asyncGraph('read-ran-async-4kb-q%d.pdf', seq_r, 'Read: random, async, 4KB, Q%d, !O_DIRECT')
seq_r <- filter( df, rw == 'r', iomode == 'a', access == 'r', block == 1048576, odirect == 0 )
asyncGraph('read-ran-async-1mb-q%d.pdf', seq_r, 'Read: random, async, 1MB, Q%d, !O_DIRECT')

# read-sequential-async + O_DIRECT
seq_r <- filter( df, rw == 'r', iomode == 'a', access == 's', block == 4096, odirect == 1 )
asyncGraph('read-seq-async-4kb-q%d.pdf', seq_r, 'Read: sequential, async, 4KB, Q%d, O_DIRECT')
seq_r <- filter( df, rw == 'r', iomode == 'a', access == 's', block == 1048576, odirect == 1 )
asyncGraph('read-seq-async-1mb-q%d.pdf', seq_r, 'Read: sequential, async, 1MB, Q%d, O_DIRECT')

# read-random-async + O_DIRECT
seq_r <- filter( df, rw == 'r', iomode == 'a', access == 'r', block == 4096, odirect == 1 )
asyncGraph('read-ran-async-4kb-q%d-o.pdf', seq_r, 'Read: random, async, 4KB, Q%d, O_DIRECT')
seq_r <- filter( df, rw == 'r', iomode == 'a', access == 'r', block == 1048576, odirect == 1 )
asyncGraph('read-ran-async-1mb-q%d-o.pdf', seq_r, 'Read: random, async, 1MB, Q%d, O_DIRECT')


# ===========================================
# Graphs -- box-blot compares

boxplot <- function(file, data, title, inc_q) {
  # transform column values to human readable
  rw_t=c(r='read', w='write')
  io_t=c(s='block', a='async')
  a_t=c(s='seq', r='rand')
  b_t=c('4096'='4KB', '1048576'='1MB')
  o_t=c('0'='BUF', '1'='OD')
  q_t=c('1'='Q01', '15'='Q15', '31'='Q31', '62'='Q62', ordered=T)
  data <- mutate(data, rw=rw_t[as.character(rw)],
                  iomode=io_t[as.character(iomode)],
                  access=a_t[as.character(access)],
                  block=b_t[as.character(block)],
                  odirect=o_t[as.character(odirect)],
                  queue=q_t[as.character(queue)])

  if (inc_q) {
    data <- transmute(data,
                      id=paste(rw,iomode,access,block,queue,odirect,sep='-'),
                      mbs=mbs)
    print(data)
  } else {
    data <- transmute(data,
                      id=paste(rw,iomode,access,block,odirect,sep='-'),
                      mbs=mbs)
  }

  pdf(paste(OUT_FOLDER, file, sep='/'))
  g <- ggplot(data)
  g <- g + geom_boxplot(aes(x=id,y=mbs))
  g <- g + coord_flip()
  g <- g + ggtitle(title)
  print(g)
  dev.off()
}

# read, blocking
data <- filter( df, rw == 'r', iomode == 's' )
boxplot('bp-read-block.pdf', data, 'Read: blocking', F)

# write, blocking
data <- filter( df, rw == 'w', iomode == 's' )
boxplot('bp-write-block.pdf', data, 'Write: blocking', F)

# read, async
data <- filter( df, rw == 'r', iomode == 'a', odirect == 0 )
boxplot('bp-read-async.pdf', data, 'Read: async, !O_DIRECT', T)

# read, async + odirect
data <- filter( df, rw == 'r', iomode == 'a', odirect == 1 )
boxplot('bp-read-async-o.pdf', data, 'Read: async, O_DIRECT', T)

