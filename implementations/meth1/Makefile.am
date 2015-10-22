ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libutil libsort libmeth1 libmeth4 app experiments

.PHONY: format check-all cppcheck bin-dist recs-1gb recs-10gb recs-30gb

format:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
     find $$subdir -iregex '.*\.\(cc\|hh\)$$' -exec clang-format -i {} +; \
	done

check-all:
	@make clean
	@scan-build ./configure
	@scan-build make

cppcheck:
	@cppcheck --std=c++11 \
		--enable=warning \
		--enable=performance \
		--enable=portability \
		--enable=style \
		. 2> cppcheck.err 1>/dev/null
	@echo "Finished! (check cppcheck.err)..."
	@echo ""
	@head cppcheck.err

bin-dist:
	rm -Rf dist_root
	mkdir -p dist_root/usr/bin dist_root/etc/init
	chmod 755 dist_root/usr
	chmod 755 dist_root/usr/bin
	chmod 755 dist_root/etc
	chmod 755 dist_root/etc/init
	cp app/meth1_node dist_root/usr/bin/meth1_node
	cp app/meth1_shell dist_root/usr/bin/meth1_shell
	cp app/meth1_client dist_root/usr/bin/meth1_client
	cp app/meth1_node_test_first dist_root/usr/bin/meth1_node_test_first
	cp app/meth1_node_test_r dist_root/usr/bin/meth1_node_test_r
	cp app/meth1_node_test_rw dist_root/usr/bin/meth1_node_test_rw
	cp libmeth4/meth4_client dist_root/usr/bin/meth4_client
	cp libmeth4/meth4_node dist_root/usr/bin/meth4_node
	cp scripts/clear_buffers.sh dist_root/usr/bin/clear_buffers
	cp scripts/setup_fs.sh dist_root/usr/bin/setup_fs
	cp scripts/setup_all_fs.sh dist_root/usr/bin/setup_all_fs
	cp scripts/setup_gfs.sh dist_root/usr/bin/setup_gfs
	cp scripts/gensort_all.sh dist_root/usr/bin/gensort_all
	cp upstart/meth1.conf dist_root/etc/init/
	cp upstart/meth1_node.conf dist_root/etc/init/
	chmod 755 dist_root/usr/bin/*
	chmod 644 dist_root/etc/init/*
	git show HEAD | head -1 > dist_root/etc/bad-version
	chmod 644 dist_root/etc/bad-version
	tar cvzf bad.tar.gz -C dist_root .

recs-1gb:
	../../gensort/gensort -t8 10485700 test/recs-1gb,buf

recs-5gb:
	../../gensort/gensort -t8 52428800 test/recs-5gb,buf

recs-10gb:
	../../gensort/gensort -t8 104857000 test/recs-10gb,buf

recs-30gb:
	../../gensort/gensort -t8 314572800 test/recs-30gb,buf

TESTS = \
	test/channels.test \
	test/meth1_node.test \
	test/meth1_node_multi.test \
	test/sort_libc.test \
	test/sort_basicrts.test \
	test/sort_boost.test \
	test/sort_node.test \
	test/sort_node_first.test \
	test/sort_node_multi.test \
	test/sort_node_r.test \
	test/sort_overlap_channel.test \
	test/sort_overlap_io.test \
	test/meth4_node.test \
	test/meth4_range.test
