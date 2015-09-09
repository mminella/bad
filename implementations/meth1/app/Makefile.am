bin_PROGRAMS = \
	meth1_client \
	meth1_client_test \
	meth1_node \
	meth1_node_test_first \
	meth1_node_test_r \
	meth1_node_test_rw \
	meth1_shell

AM_CPPFLAGS = \
	-D_REENTRANT \
	-I$(srcdir)/.. \
	-I$(srcdir)/../libutil \
	-I$(srcdir)/../libsort \
	-I$(srcdir)/../libmeth1 \
	-I$(srcdir)/../contrib

LDADD = \
	../libutil/libbasicrts.la \
	../libsort/libsort.la \
	../libmeth1/libmeth1.la \
	-lpthread \
	$(TBB_LIBS)

# # need `whole-archive nonsense to work around bug with C++11 condition
# # variables and static linking, and need to be careful to us `-Wl,-lpthread`
# # to avoid libtool reordering flags.
# AM_LDFLAGS = -static -static-libstdc++ -all-static \
# 	-Wl,--whole-archive -Wl,-lpthread -Wl,--no-whole-archive

meth1_node_test_first_SOURCES = meth1_node_test_first.cc
meth1_node_test_r_SOURCES = meth1_node_test_r.cc
meth1_node_test_rw_SOURCES = meth1_node_test_rw.cc
meth1_client_test_SOURCES = meth1_client_test.cc

meth1_client_SOURCES = meth1_client.cc
meth1_node_SOURCES = meth1_node.cc
meth1_shell_SOURCES = meth1_shell.cc
