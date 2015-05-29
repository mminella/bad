#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([bad1], [1.0.0], [dterei@cs.stanford.edu])

# Set automake options.
# `-Wall -Werror` here is for automake warning, nothing to do with src
# compilation.
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Set sanity check for a source file.
AC_CONFIG_SRCDIR([lib/address.hh])

# Set configure output header.
AC_CONFIG_HEADERS([config.h])

# Set default CXXFLAGS
WARN_FLAGS="-pedantic -Wall -Wextra -Weffc++"
DEF_CXXFLAGS="-std=c++11 -g -O2 $WARN_FLAGS"
AC_SUBST([AM_CXXFLAGS], ["$DEF_CXXFLAGS"])

# Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB
AM_PROG_AR

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT16_T

# Checks for library functions.

# Set output files.
AC_CONFIG_FILES([
  Makefile
  lib/Makefile
  sort/Makefile
  stratergies/Makefile
  stratergies/meth1/Makefile
  app/Makefile
])
AC_OUTPUT
