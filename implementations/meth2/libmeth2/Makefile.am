noinst_LTLIBRARIES = libmeth2.la

libmeth2_la_SOURCES = \
	client.hh client.cc \
	cluster.hh cluster.cc \
	node.hh node.cc \
	priority_queue.hh \
	remote_file.hh remote_file.cc \
	circular_aio.cc circular_aio.hh

libmeth2_la_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../libsort \
	-I$(srcdir)/../libutil \
	-I$(srcdir)/../contrib

libmeth2_la_LIBADD = \
	../libutil/libutil.la \
	../libsort/libsort.la
