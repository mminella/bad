noinst_LTLIBRARIES = libmeth1.la

libmeth1_la_SOURCES = \
	client.hh client.cc \
	client2.hh client2.cc \
	cluster.hh cluster.cc \
	cluster2.hh cluster2.cc \
	meth1_merge.hh \
	node.hh node.cc \
	priority_queue.hh \
	remote_file.hh remote_file.cc

libmeth1_la_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../.. \
	-I$(srcdir)/../../sort \
	-I$(srcdir)/../../lib \
	-I$(srcdir)/../../contrib

libmeth1_la_LIBADD = \
	../../lib/libbasicrts.la \
	../../sort/libsort.la
