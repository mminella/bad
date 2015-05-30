bin_PROGRAMS = test meth1_test meth1_client meth1_node

AM_CPPFLAGS = \
	-I$(srcdir)/../lib \
	-I$(srcdir)/../sort \
	-I$(srcdir)/../stratergies \
	-I$(srcdir)/../stratergies/meth1
LDADD = \
	../lib/libbasicrts.la \
	../sort/libsort.la \
	../stratergies/meth1/libmeth1.la \
	-lpthread

test_SOURCES = test.cc
meth1_test_SOURCES = meth1_test.cc
meth1_client_SOURCES = meth1_client.cc
meth1_node_SOURCES = meth1_node.cc

