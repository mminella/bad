#!/usr/bin/env Rscript

# Install R packages needed for models.

MIRRORS <- c('http://cran.cnr.berkeley.edu/')

if ( !require('dplyr') ) {
  install.packages('dplyr', repos=MIRRORS)
}

if ( !require('ggplot2') ) {
  install.packages('ggplot2', repos=MIRRORS)
}

if ( !require('reshape2') ) {
  install.packages('reshape2', repos=MIRRORS)
}

if ( !require('devtools') ) {
  install.packages('devtools', repos=MIRRORS)
}

if ( !require('ggthemr') ) {
  library('devtools')
  devtools::install_github('cttobin/ggthemr')
}

