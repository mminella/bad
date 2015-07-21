bin_PROGRAMS = \
	echo_server \
	sort_basicrts \
	sort_boost \
	sort_libc \
	sort_overlap_io

AM_CPPFLAGS = \
	-I$(srcdir)/../lib \
	-I$(srcdir)/../sort

LDADD = \
	../lib/libbasicrts.la \
	../sort/libsort.la \
	$(BOOST_LIBS) \
	-lpthread

AM_LDFLAGS = -static -static-libstdc++

echo_server_SOURCES = echo_server.cc

sort_basicrts_SOURCES = sort_basicrts.cc
sort_boost_SOURCES = sort_boost.cc
sort_libc_SOURCES = sort_libc.cc
sort_overlap_io_SOURCES = sort_overlap_io.cc
