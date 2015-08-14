bin_PROGRAMS = \
	meth1_client \
	meth1_client_test \
	meth1_node \
	meth1_node_test \
	meth1_node_test2 \
	meth1_shell

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
	-lpthread \
	$(BOOST_LIBS)

# need `whole-archive nonsense to work around bug with C++11 condition
# variables and static linking, and need to be careful to us `-Wl,-lpthread`
# to avoid libtool reordering flags.
AM_LDFLAGS = -static -static-libstdc++ -all-static \
	-Wl,--whole-archive -Wl,-lpthread -Wl,--no-whole-archive

meth1_node_test_SOURCES = meth1_node_test.cc
meth1_node_test2_SOURCES = meth1_node_test2.cc
meth1_client_test_SOURCES = meth1_client_test.cc

meth1_client_SOURCES = meth1_client.cc
meth1_node_SOURCES = meth1_node.cc
meth1_shell_SOURCES = meth1_shell.cc
