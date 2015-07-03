ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib sort stratergies app

format:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
     find $$subdir -iregex '.*\.\(cc\|hh\)$$' -exec clang-format -i {} +; \
	 done

check-all:
	@make clean
	@scan-build ./configure
	@scan-build make

test: app/sort_test app/meth1_node_test
	@mkdir -p .test-tmp
	@./app/sort_test test/in.s0000.e1000.recs .test-tmp/out.1000.recs > /dev/null
	@diff test/out.1000.recs .test-tmp/out.1000.recs && echo "PASSED 1" || echo "FAILED 1"
	@rm .test-tmp/out.1000.recs
	@./app/meth1_node_test test/in.s0000.e1000.recs .test-tmp/out.1000.recs > /dev/null
	@diff test/out.1000.recs .test-tmp/out.1000.recs && echo "PASSED 2" || echo "FAILED 2"

bin-dist: app/meth1_node app/meth1_shell app/meth1_client
	rm -Rf dist_root
	mkdir -p dist_root/usr/bin
	cp app/meth1_node dist_root/usr/bin/meth1_node
	cp app/meth1_shell dist_root/usr/bin/meth1_shell
	cp app/meth1_client dist_root/usr/bin/meth1_client
	cp scripts/clear_buffers.sh dist_root/usr/bin/clear_buffers
	tar cvzf bad.tar.gz -C dist_root .

