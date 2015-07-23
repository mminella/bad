bin_PROGRAMS = \
	echo_server \
	sort_basicrts \
	sort_boost \
	sort_libc \
	sort_overlap_channel \
	sort_overlap_io

AM_CPPFLAGS = \
	-D_REENTRANT \
	-I$(srcdir)/.. \
	-I$(srcdir)/../lib \
	-I$(srcdir)/../sort

LDADD = \
	../lib/libbasicrts.la \
	../sort/libsort.la \
	$(BOOST_LIBS)

# need `whole-archive nonsense to work around bug with C++11 condition
# variables and static linking, and need to be careful to us `-Wl,-lpthread`
# to avoid libtool reordering flags.
AM_LDFLAGS = -static -static-libstdc++ -all-static \
	-Wl,--whole-archive -Wl,-lpthread -Wl,--no-whole-archive

echo_server_SOURCES = echo_server.cc

sort_basicrts_SOURCES = sort_basicrts.cc
sort_boost_SOURCES = sort_boost.cc
sort_libc_SOURCES = sort_libc.cc
sort_overlap_channel_SOURCES = sort_overlap_channel.cc
sort_overlap_io_SOURCES = sort_overlap_io.cc
