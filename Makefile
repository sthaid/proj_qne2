
SUBDIRS = \
    picoc \
    linux \
    android \
    bin/src

APPS = \
    files/apps/Clock \
    files/apps/Compass \
    files/apps/HexCalc \
    files/apps/Light \
    files/apps/Location \
    files/apps/Morse \
    files/apps/Reversi \
    files/apps/Template \
    files/apps/Test

SVCS = \
    files/svcs/Location \
    files/svcs/Template

.PHONY: build clean

build:
	for d in $(SUBDIRS) ; do echo "\n======== BUILD $$d ========\n"; make -C $$d || exit 1; done
	for d in $(APPS)    ; do echo "\n======== BUILD APP $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done
	for d in $(SVCS)    ; do echo "\n======== BUILD SVC $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done

build_apps:
	for d in $(APPS)    ; do echo "\n======== BUILD APP $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done

build_svcs:
	for d in $(SVCS)    ; do echo "\n======== BUILD SVC $$d ========\n"; cd $$d; eztest build || exit 1; cd ../../..; done

clean:
	for d in $(SUBDIRS) ; do echo "\n======== CLEAN $$d ========\n"; make -C $$d clean || exit 1; done

install:
	cd android; ./do_debug_install

