bin_PROGRAMS = \
	sort_test \
	meth1_node_test \
	meth1_client_test \
	meth1_client \
	meth1_node

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

sort_test_SOURCES = sort_test.cc
meth1_node_test_SOURCES = meth1_node_test.cc
meth1_client_test_SOURCES = meth1_client_test.cc

meth1_node_SOURCES = meth1_node.cc
meth1_client_SOURCES = meth1_client.cc
