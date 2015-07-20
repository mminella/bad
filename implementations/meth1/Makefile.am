ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib sort stratergies app experiments

.PHONY: format check-all test bin-dist recs-1gb

format:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
     find $$subdir -iregex '.*\.\(cc\|hh\)$$' -exec clang-format -i {} +; \
	 done

check-all:
	@make clean
	@scan-build ./configure
	@scan-build make

test: experiments/sort_libc experiments/sort_basicrts app/meth1_node_test
	@mkdir -p .test-tmp
	
	@./experiments/sort_libc test/in.s0000.e1000.recs .test-tmp/out.1000.recs > /dev/null
	@diff test/out.1000.recs .test-tmp/out.1000.recs > /dev/null && \
		echo "sort libc test     : PASSED" || \
		echo "sort libc test     : FAILED"
	@rm .test-tmp/out.1000.recs
	
	@./experiments/sort_basicrts test/in.s0000.e1000.recs .test-tmp/out.1000.recs > /dev/null
	@diff test/out.1000.recs .test-tmp/out.1000.recs > /dev/null && \
		echo "sort basicrts test : PASSED" || \
		echo "sort basicrts test : FAILED"
	@rm .test-tmp/out.1000.recs
	
	@./app/meth1_node_test test/in.s0000.e1000.recs .test-tmp/out.1000.recs > /dev/null
	@diff test/out.1000.recs .test-tmp/out.1000.recs > /dev/null && \
		echo "node test          : PASSED" || \
		echo "node test          : FAILED"
	

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
	tar cvzf bad.tar.gz -C dist_root .

recs-1gb:
	../../gensort/gensort 10485700 test/recs-1gb

