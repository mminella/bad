bin_PROGRAMS = \
	channels \
	echo_server \
	priority_queue \
	rec_memcpy \
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

channels_SOURCES = channels.cc
echo_server_SOURCES = echo_server.cc

priority_queue_SOURCES = priority_queue.cc
rec_memcpy_SOURCES = rec_memcpy.cc

sort_basicrts_SOURCES = sort_basicrts.cc
sort_boost_SOURCES = sort_boost.cc
sort_libc_SOURCES = sort_libc.cc
sort_overlap_channel_SOURCES = sort_overlap_channel.cc
sort_overlap_io_SOURCES = sort_overlap_io.cc
