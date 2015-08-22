#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([bad1],[1.0.0],[dterei@cs.stanford.edu])

# Set automake options.
# `-Wall -Werror` here is for automake warning, nothing to do with src
# compilation.
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Set sanity check for a source file.
AC_CONFIG_SRCDIR([lib/address.hh])

# Set m4 output directory (for libtool).
AC_CONFIG_MACRO_DIR([m4])

# Set configure output header.
AC_CONFIG_HEADERS([config.h])

# Set some default CXXFLAGS
WARN_FLAGS="-pedantic -Wall -Wextra -Weffc++"
AC_SUBST([AM_CXXFLAGS], ["$WARN_FLAGS"])

# Checks for programs.
AC_PROG_CXX
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11([noext])
AM_PROG_AR
LT_INIT([disable-shared])

# Checks for header files.
AC_CHECK_HEADERS([boost/sort/spreadsort/string_sort.hpp])
AC_CHECK_HEADERS([boost/heap/d_ary_heap.hpp])
AC_CHECK_HEADERS([tbb/parallel_invoke.h], [TBB_LIBS="-ltbb"])
AC_CHECK_HEADERS([tbb/parallel_sort.h])
AC_SUBST(TBB_LIBS)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.

# Set output files.
AC_CONFIG_FILES([
  Makefile
  app/Makefile
  experiments/Makefile
  lib/Makefile
  sort/Makefile
  stratergies/Makefile
  stratergies/meth1/Makefile
])
AC_OUTPUT
