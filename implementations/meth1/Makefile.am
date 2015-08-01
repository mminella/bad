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

bin-dist:
	rm -Rf dist_root
	mkdir -p dist_root/usr/bin dist_root/etc/init
	cp app/meth1_node dist_root/usr/bin/meth1_node
	cp app/meth1_shell dist_root/usr/bin/meth1_shell
	cp app/meth1_client dist_root/usr/bin/meth1_client
	cp scripts/clear_buffers.sh dist_root/usr/bin/clear_buffers
	cp scripts/setup_fs.sh dist_root/usr/bin/setup_fs
	cp upstart/meth1.conf dist_root/etc/init/
	cp upstart/meth1_node.conf dist_root/etc/init/
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
