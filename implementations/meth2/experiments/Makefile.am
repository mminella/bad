bin_PROGRAMS = index1 index2 index3 index4

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
AM_LDFLAGS = -static -static-libstdc++ -all-static
#	-Wl,--whole-archive -Wl,-lpthread -Wl,--no-whole-archive

index1_SOURCES = index1.cc
index2_SOURCES = index2.cc
index3_SOURCES = index3.cc
index4_SOURCES = index4.cc

