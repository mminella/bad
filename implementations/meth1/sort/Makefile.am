noinst_LTLIBRARIES = libsort.la

libsort_la_SOURCES = \
	alloc.hh alloc.cc \
	record.hh \
	record_loc.hh record_loc.cc \
	record_t.hh record_t.cc \
	record_t_shallow.hh record_t_shallow.cc \
	record_ptr.hh \
	record_common.hh

libsort_la_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../lib \
	-I$(srcdir)/../contrib

libsort_la_LIBADD = \
	../lib/libbasicrts.la
