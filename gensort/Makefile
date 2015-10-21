# gensort and valsort Makefile
#
# Define USE_SUMP_PUMP as "1" to use the SUMP Pump library to make
# multithreaded versions of gensort and valsort.  Use any other value
# to make single-threaded versions.  SUMP Pump requires the pthread library.
#
USE_SUMP_PUMP=1
# use BITS=-m32 to force 32-bit build on a 64-bit platform
BITS=
CFLAGS=$(BITS) -O3 -Wall

ifeq ($(CC),gcc)
LIBRT=-lrt
else
LIBRT=
endif

ifeq ($(UNAME_S),Linux)
LIBRT += -ldl
endif
ifeq ($(UNAME_S),Darwin)
LIBRT += -ldl
endif

all: gensort valsort

ifeq ($(USE_SUMP_PUMP),1)
# {
gensort: gensort.c rand16.o rand16.h sump.o
	$(CC) -g $(CFLAGS) -o gensort -DSUMP_PUMP gensort.c rand16.o sump.o -lz -lpthread $(LIBRT)

valsort: valsort.c rand16.o rand16.h sump.o
	$(CC) -g $(CFLAGS) -o valsort -DSUMP_PUMP valsort.c rand16.o sump.o -lz -lpthread $(LIBRT)

sump.o: sump.c sump.h
	$(CC) -g $(BITS) -Wall -Wno-char-subscripts -c -DSUMP_PUMP_NO_SORT sump.c
# }
else
# {
gensort: gensort.c rand16.o rand16.h
	$(CC) -g $(CFLAGS) -o gensort gensort.c rand16.o -lz

valsort: valsort.c rand16.o rand16.h
	$(CC) -g $(CFLAGS) -o valsort valsort.c rand16.o -lz
# }
endif

rand16.o: rand16.c rand16.h
	$(CC) -g $(CFLAGS) -c rand16.c
