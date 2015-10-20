CXX=c++
CXXFLAGS=-O3 -std=c++11
LDFLAGS=-pthread -lpthread
BINARIES= \
	read \
	random_read \
	random_read_meth2 \
	write \
	random_write \
	memalloc_speed \
	memcpy_speed \
	memcmp_speed \
	network_recv \
	network_send

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
BINARIES += \
	async_read \
	async_random_read
ASYNC := 1
endif

ifeq ($(UNAME_S),FreeBSD)
CXXFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
endif

all: $(BINARIES)

memalloc_speed: memalloc_speed.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< -ltbb

memcpy_speed: memcpy_speed.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< -ltbb

memcmp_speed: memcmp_speed.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< -ltbb

.PHONY: clean dist bin-dist

clean:
	rm -f *.o
	rm -rf dist_root
	rm -f measure.tar.gz
	rm -f $(BINARIES)

dist: $(BINARIES)
	rm -Rf dist_root
	mkdir -p dist_root/usr/bin
	cp memalloc_speed dist_root/usr/bin/memalloc_speed
	cp memcmp_speed dist_root/usr/bin/memcmp_speed
	cp memcpy_speed  dist_root/usr/bin/memcpy_speed
	cp read dist_root/usr/bin/measure_read
	cp random_read dist_root/usr/bin/measure_random_read
	cp write dist_root/usr/bin/measure_write
	cp random_write dist_root/usr/bin/measure_random_write
	if [ ${ASYNC} -eq 1 ]; then \
		cp async_read dist_root/usr/bin/measure_async_read; \
		cp async_random_read dist_root/usr/bin/measure_async_random_read; \
	fi
	cp network_recv dist_root/usr/bin/measure_network_recv
	cp network_send dist_root/usr/bin/measure_network_send
	cp test_async.py dist_root/usr/bin
	cp test_sync.py dist_root/usr/bin
	cp test_dd.py dist_root/usr/bin
	cp drop_cache.sh dist_root/usr/bin/drop_cache
	cp setup_fs.sh dist_root/usr/bin/setup_fs
	cp run_experiment.sh dist_root/usr/bin/run_experiment
	tar cvzf measure.tar.gz -C dist_root .

bin-dist: dist
