bin_PROGRAMS = test meth1

test_SOURCES = test.cc
test_CPPFLAGS = \
	-I$(srcdir)/../lib \
	-I$(srcdir)/../sort
test_LDADD = \
	../lib/libbasicrts.la \
	../sort/libsort.la

meth1_SOURCES = meth1.cc
meth1_CPPFLAGS = \
	-I$(srcdir)/../lib \
	-I$(srcdir)/../sort \
	-I$(srcdir)/../stratergies \
	-I$(srcdir)/../stratergies/meth1
meth1_LDADD = \
	../lib/libbasicrts.la \
	../sort/libsort.la \
	../stratergies/meth1/libmeth1.la \
	-lpthread
