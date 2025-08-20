
SUBDIRS = \
    apps/Primes \
    apps/Reversi \
    apps/Test \
    picoc \
    linux \
    android

.PHONY: build clean

build:
	for d in $(SUBDIRS) ; do echo "\n======== BUILD $$d ========\n"; make -C $$d || exit 1; done

clean:
	for d in $(SUBDIRS) ; do echo "\n======== CLEAN $$d ========\n"; make -C $$d clean || exit 1; done

install:
	cd android; ./do_debug_install

