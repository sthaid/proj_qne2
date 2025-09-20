
SUBDIRS = \
    picoc \
    linux \
    android \
    bin/src

APPS = \
    files/apps/Env \
    files/apps/Reversi \
    files/apps/Test \

SVCS = \
    files/svcs/Sensors

.PHONY: build clean

build:
	for d in $(SUBDIRS) ; do echo "\n======== BUILD $$d ========\n"; make -C $$d || exit 1; done
	for d in $(APPS)    ; do echo "\n======== BUILD APP $$d ========\n"; cd $$d; test_app build || exit 1; cd ../../..; done
	for d in $(SVCS)    ; do echo "\n======== BUILD SVC $$d ========\n"; cd $$d; test_app build || exit 1; cd ../../..; done

clean:
	for d in $(SUBDIRS) ; do echo "\n======== CLEAN $$d ========\n"; make -C $$d clean || exit 1; done

install:
	cd android; ./do_debug_install

