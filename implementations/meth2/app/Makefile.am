bin_PROGRAMS = 

AM_CPPFLAGS = \
	-D_REENTRANT \
	-I$(srcdir)/../lib \
	-I$(srcdir)/../sort \
	-I$(srcdir)/../stratergies \
	-I$(srcdir)/../stratergies/meth1

LDADD = \
	../lib/libbasicrts.la \
	../sort/libsort.la \
	../stratergies/meth1/libmeth1.la \
	$(BOOST_LIBS)

# need `whole-archive nonsense to work around bug with C++11 condition
# variables and static linking, and need to be careful to us `-Wl,-lpthread`
# to avoid libtool reordering flags.
AM_LDFLAGS = -static -static-libstdc++ -all-static \
	-Wl,--whole-archive -Wl,-lpthread -Wl,--no-whole-archive

