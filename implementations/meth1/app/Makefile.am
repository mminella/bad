AM_CPPFLAGS = -I$(srcdir)/../lib \
							-I$(srcdir)/../sort \
							-I$(srcdir)/../stratergies \
							-I$(srcdir)/../stratergies/meth1
LDADD = ../lib/libbasicrts.a \
				../sort/libsort.a \
				../stratergies/meth1/libmeth1.a \
				-lpthread

bin_PROGRAMS = test meth1

test_SOURCES = test.cc

meth1_SOURCES = meth1.cc

