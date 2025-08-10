
SUBDIRS = android picoc linux apps/unit_test apps/reversi apps/primes

.PHONY: build clean

build:
	for d in $(SUBDIRS) ; do echo; make -C $$d || exit 1; done

clean:
	for d in $(SUBDIRS) ; do echo; make -C $$d clean || exit 1; done

