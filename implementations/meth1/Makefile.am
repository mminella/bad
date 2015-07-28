ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib sort stratergies app experiments

.PHONY: format check-all bin-dist recs-1gb recs-10gb

format:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
     find $$subdir -iregex '.*\.\(cc\|hh\)$$' -exec clang-format -i {} +; \
	 done

check-all:
	@make clean
	@scan-build ./configure
	@scan-build make

bin-dist: app/meth1_node app/meth1_shell app/meth1_client
	rm -Rf dist_root
	mkdir -p dist_root/usr/bin
	cp app/meth1_node dist_root/usr/bin/meth1_node
	cp app/meth1_shell dist_root/usr/bin/meth1_shell
	cp app/meth1_client dist_root/usr/bin/meth1_client
	cp app/sort_test dist_root/usr/bin/sort_test
	cp app/sort_test_cfiles dist_root/usr/bin/sort_test_cfiles
	cp app/meth1_node_test dist_root/usr/bin/meth1_node_test
	cp scripts/clear_buffers.sh dist_root/usr/bin/clear_buffers
	cp scripts/setup_fs.sh dist_root/usr/bin/setup_fs
	tar cvzf bad.tar.gz -C dist_root .

recs-1gb:
	../../gensort/gensort 10485700 test/recs-1gb

recs-10gb:
	../../gensort/gensort 104857000 test/recs-1gb

TESTS = \
	test/channels.test \
	test/sort_libc.test \
	test/sort_basicrts.test \
	test/sort_boost.test \
	test/sort_node.test \
	test/sort_overlap_channel.test \
	test/sort_overlap_io.test
