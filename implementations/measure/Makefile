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
	cp $(BINARIES) dist_root/usr/bin
	cp test_async.py dist_root/usr/bin
	cp test_sync.py dist_root/usr/bin
	cp drop_cache.sh dist_root/usr/bin/drop_cache
	cp setup_fs.sh dist_root/usr/bin/setup_fs
	tar cvzf measure.tar.gz -C dist_root .
