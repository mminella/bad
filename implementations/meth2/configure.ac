#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([bad-meth2],[1.0.0],[mashti@cs.stanford.edu])

# Set automake options.
# `-Wall -Werror` here is for automake warning, nothing to do with src
# compilation.
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Set sanity check for a source file.
AC_CONFIG_SRCDIR([libutil/address.hh])

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
AC_CHECK_HEADERS([tbb/task_group.h])
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
  libutil/Makefile
  libsort/Makefile
  libmeth2/Makefile
])
AC_OUTPUT
