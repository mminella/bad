bin_PROGRAMS = \
	meth2_client \
	meth2_client_test \
	meth2_node \
	meth2_node_test_r \
	meth2_node_test_rw \
	meth2_shell

AM_CPPFLAGS = \
	-D_REENTRANT \
	-I$(srcdir)/.. \
	-I$(srcdir)/../libutil \
	-I$(srcdir)/../libsort \
	-I$(srcdir)/../libmeth2 \
	-I$(srcdir)/../contrib

LDADD = \
	../libutil/libutil.la \
	../libsort/libsort.la \
	../libmeth2/libmeth2.la \
	-lpthread

# # need `whole-archive nonsense to work around bug with C++11 condition
# # variables and static linking, and need to be careful to us `-Wl,-lpthread`
# # to avoid libtool reordering flags.
# AM_LDFLAGS = -static -static-libstdc++ -all-static \
# 	-Wl,--whole-archive -Wl,-lpthread -Wl,--no-whole-archive

meth2_node_test_r_SOURCES = meth2_node_test_r.cc
meth2_node_test_rw_SOURCES = meth2_node_test_rw.cc
meth2_client_test_SOURCES = meth2_client_test.cc

meth2_client_SOURCES = meth2_client.cc
meth2_node_SOURCES = meth2_node.cc
meth2_shell_SOURCES = meth2_shell.cc
