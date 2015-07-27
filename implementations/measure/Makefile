CXX=c++
CXXFLAGS=-O3 -lrt
BINARIES= \
	read \
	random_read \
	write \
	random_write \
	async_random_read \
	async_read

all: $(BINARIES)

.PHONY: clean dist

clean:
	rm -f *.o
	rm -rf dist_root
	rm -f measure.tar.gz
	rm -f $(BINARIES)

dist: $(BINARIES)
	rm -Rf dist_root
	mkdir -p dist_root/usr/bin
	cp read dist_root/usr/bin/measure_read
	cp random_read dist_root/usr/bin/measure_random_read
	cp write dist_root/usr/bin/measure_write
	cp random_write dist_root/usr/bin/measure_random_write
	cp async_read dist_root/usr/bin/measure_async_read
	cp async_random_read dist_root/usr/bin/measure_async_random_read
	cp test_async.py dist_root/usr/bin
	cp test_sync.py dist_root/usr/bin
	cp drop_cache.sh dist_root/usr/bin/drop_cache
	cp setup_fs.sh dist_root/usr/bin/setup_fs
	cp run_experiment.sh dist_root/usr/bin/run_experiment
	tar cvzf measure.tar.gz -C dist_root .
