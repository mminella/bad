#!/usr/bin/env Rscript

# Install R packages needed for models.

MIRRORS <- c('http://cran.cnr.berkeley.edu/')

installPackage <- function(pkg) {
  if (!require(pkg, character.only=T)) {
    install.packages(pkg, repos=MIRRORS)
  }
}

installPackage('plyr')
installPackage('dplyr')
installPackage('ggplot2')
installPackage('reshape2')
installPackage('devtools')
installPackage('argparser')

if ( !require('ggthemr') ) {
  library('devtools')
  devtools::install_github('cttobin/ggthemr')
}

