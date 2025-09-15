
SRCS :=  \
  ../src/main.c \
  ../src/sdl_misc.c \
  ../src/sdl_video.c \
  ../src/sdl_audio.c \
  ../src/sdl_sensor.c \
  ../src/sdl_event.c \
  ../src/test.cpp \
  ../src/utils.c \
  ../src/logging.c \
  ../picoc/picoc_ezapp.c \
  ../picoc/clibrary.c \
  ../picoc/debug.c \
  ../picoc/expression.c \
  ../picoc/heap.c \
  ../picoc/include.c \
  ../picoc/lex.c \
  ../picoc/parse.c \
  ../picoc/platform.c \
  ../picoc/table.c \
  ../picoc/type.c \
  ../picoc/variable.c \
  ../picoc/cstdlib/ctype.c \
  ../picoc/cstdlib/errno.c \
  ../picoc/cstdlib/math.c \
  ../picoc/cstdlib/stdbool.c \
  ../picoc/cstdlib/string.c \
  ../picoc/cstdlib/stdio.c \
  ../picoc/cstdlib/stdlib.c \
  ../picoc/cstdlib/time.c \
  ../picoc/cstdlib/unistd.c \
  ../picoc/platform/library_unix.c \
  ../picoc/platform/platform_unix.c

TARGET = ezApp

# xxx add -O2

CFLAGS = -Wall -Werror -Wno-unused-result -g -I../src $(shell pkg-config sdl3--cflags)

$(TARGET):
	../tools/gen_version
	gcc -o $@ $(CFLAGS) $(SRCS) -lSDL3 -lSDL3_ttf -lm
	rm -rf files assets; mkdir files assets;
	../tools/gen_tar linux/assets/apps_and_svcs.tar
	ln -s ../../android/assets/FreeMonoBold.ttf assets
	ln -s ../../android/assets/copyright assets

run: $(TARGET)
	ezApp

clean:
	rm -rf $(TARGET) files assets ../src/version.h

.PHONY: $(TARGET)

