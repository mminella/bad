noinst_LTLIBRARIES = libmeth1.la

libmeth1_la_SOURCES = \
	node.hh node.cc \
	client.hh client.cc

libmeth1_la_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../../sort \
	-I$(srcdir)/../../lib

libmeth1_la_LIBADD = \
	../../lib/libbasicrts.la \
	../../sort/libsort.la
