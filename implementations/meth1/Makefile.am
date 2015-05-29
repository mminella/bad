SUBDIRS = lib sort stratergies app

format:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
     find $$subdir -iregex '.*\.\(cc\|hh\)$$' -exec clang-format -i {} +; \
	 done

check-all:
	@make clean
	@scan-build ./configure
	@scan-build make

test: app/test
	@mkdir -p .test-tmp
	@./app/test test/in.100.recs .test-tmp/out.100.recs > /dev/null
	@diff test/out.100.recs .test-tmp/out.100.recs && echo "PASSED"

