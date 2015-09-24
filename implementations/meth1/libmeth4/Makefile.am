bin_PROGRAMS = \
	meth4_node

AM_CPPFLAGS = \
	-D_REENTRANT \
	-I$(srcdir)/.. \
	-I$(srcdir)/../libutil \
	-I$(srcdir)/../libsort

LDADD = \
	../libutil/libutil.la \
	../libsort/libsort.la \
	-lpthread \
	$(TBB_LIBS)

meth4_node_SOURCES = \
	meth4_node.cc \
	meth4_knobs.hh \
	send.hh send.cc \
	recv.hh recv.cc \
	config_file.hh config_file.cc \
	cluster_map.hh cluster_map.cc \
	disk_writer.hh disk_writer.cc \
	sort.hh sort.cc
