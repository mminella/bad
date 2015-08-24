ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libutil libsort libmeth2 app experiments

.PHONY: format check-all bin-dist recs-1gb recs-10gb recs-30gb

format:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
     find $$subdir -iregex '.*\.\(cc\|hh\)$$' -exec clang-format -i {} +; \
	 done

check-all:
	@make clean
	@scan-build ./configure
	@scan-build make

bin-dist:
	rm -Rf dist_root
	cp ../meth1/scripts/clear_buffers.sh dist_root/usr/bin/clear_buffers
	cp ../meth1/scripts/setup_fs.sh dist_root/usr/bin/setup_fs
	cp ../meth1/scripts/setup_all_fs.sh dist_root/usr/bin/setup_all_fs
	tar cvzf bad.tar.gz -C dist_root .

recs-1gb:
	../../gensort/gensort -t8 10485700 test/recs-1gb,buf

recs-10gb:
	../../gensort/gensort -t8 104857000 test/recs-10gb,buf

recs-30gb:
	../../gensort/gensort -t8 314572800 test/recs-30gb,buf
