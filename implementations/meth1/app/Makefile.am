bin_PROGRAMS = \
	echo_server \
	meth1_client \
	meth1_client_test \
	meth1_cluster_test \
	meth1_node \
	meth1_node_test \
	meth1_shell \
	sort_test \
	sort_test_cfiles \
	sort_simple \
	sort_overlap \
	sort_boost

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

AM_LDFLAGS = -static -static-libstdc++

sort_test_SOURCES = sort_test.cc
sort_test_cfiles_SOURCES = sort_test_cfiles.cc
sort_simple_SOURCES = sort_simple.cc
sort_overlap_SOURCES = sort_overlap.cc
sort_boost_SOURCES = sort_boost.cc

meth1_node_test_SOURCES = meth1_node_test.cc
meth1_client_test_SOURCES = meth1_client_test.cc
meth1_cluster_test_SOURCES = meth1_cluster_test.cc

meth1_node_SOURCES = meth1_node.cc
meth1_client_SOURCES = meth1_client.cc
meth1_shell_SOURCES = meth1_shell.cc

echo_server_SOURCES = echo_server.cc
