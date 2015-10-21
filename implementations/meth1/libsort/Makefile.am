noinst_LTLIBRARIES = libsort.la

libsort_la_SOURCES = \
	alloc.hh \
	record.hh \
	record_common.hh \
	record_loc.hh record_loc.cc \
	record_ptr.hh \
	record_string.hh record_string.cc \
	record_t.hh record_t.cc \
	record_t_shallow.hh record_t_shallow.cc

libsort_la_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../libutil

libsort_la_LIBADD = \
	../libutil/libutil.la
