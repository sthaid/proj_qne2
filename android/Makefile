build_debug: gen_version gen_assets
	cd SDL/build/org.sthaid.ezApp; \
        ./gradlew assembleDebug

install_debug: build_debug
	cd SDL/build/org.sthaid.ezApp; \
        adb install -r ./app/build/outputs/apk/debug/app-debug.apk

uninstall:
	adb uninstall  org.sthaid.ezApp

clean:
	cd SDL/build/org.sthaid.ezApp; \
        ./gradlew clean

clobber:
	rm -f SDL
	rm -rf 	SDL3-*
	rm -rf 	setup_files/orig
	rm -rf assets/*

# -----------------------------

gen_version:
	../bin/build_tools/gen_version

gen_assets:
	cd ../files; \
	git ls-files > /tmp/files.txt; \
	tar -cf ../android/assets/files.tar --files-from /tmp/files.txt; \
	rm -f /tmp/files.txt

# -----------------------------

.PHONY: build_debug install_debug
.PHONY: clean clobber
.PHONY: gen_version gen_assets

