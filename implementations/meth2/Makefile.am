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
	mkdir -p dist_root/usr/bin dist_root/etc/init
	chmod 755 dist_root/usr
	chmod 755 dist_root/usr/bin
	chmod 755 dist_root/etc
	chmod 755 dist_root/etc/init
	cp app/meth2_node dist_root/usr/bin/meth1_node
	cp app/meth2_shell dist_root/usr/bin/meth1_shell
	cp app/meth2_client dist_root/usr/bin/meth1_client
	cp app/meth2_client_test dist_root/usr/bin/meth1_client_test
	cp app/meth2_node_test_r dist_root/usr/bin/meth1_node_test_r
	cp app/meth2_node_test_rw dist_root/usr/bin/meth1_node_test_rw
	cp scripts/clear_buffers.sh dist_root/usr/bin/clear_buffers
	cp scripts/setup_fs.sh dist_root/usr/bin/setup_fs
	cp scripts/setup_all_fs.sh dist_root/usr/bin/setup_all_fs
	cp scripts/gensort_all.sh dist_root/usr/bin/gensort_all
	cp experiments/sortperf dist_root/usr/bin/sortperf
	cp ../measure/random_read_meth2 dist_root/usr/bin/random_read_meth2
	cp upstart/meth2.conf dist_root/etc/init/
	cp upstart/meth2_node.conf dist_root/etc/init/
	chmod 755 dist_root/usr/bin/*
	chmod 644 dist_root/etc/init/*
	git show HEAD | head -1 > dist_root/etc/bad-version
	chmod 644 dist_root/etc/bad-version
	tar cvzf bad.tar.gz -C dist_root .

recs-1gb:
	../../gensort/gensort -t8 10485700 test/recs-1gb,buf

recs-10gb:
	../../gensort/gensort -t8 104857000 test/recs-10gb,buf

recs-30gb:
	../../gensort/gensort -t8 314572800 test/recs-30gb,buf
