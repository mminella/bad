CXX=c++
CXXFLAGS=-O3 -lrt -std=c++11 -ltbb
BINARIES= \
	read \
	random_read \
	write \
	random_write \
	async_random_read \
	async_read \
	memalloc_speed \
	memcpy_speed \
	memcmp_speed \
	network_recv \
	network_send

all: $(BINARIES)

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
	cp async_read dist_root/usr/bin/measure_async_read
	cp async_random_read dist_root/usr/bin/measure_async_random_read
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
