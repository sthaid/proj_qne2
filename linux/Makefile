
SRCS :=  \
  ../src/main.c \
  ../src/svcs.c \
  ../src/sdlx_misc.c \
  ../src/sdlx_video.c \
  ../src/sdlx_audio.c \
  ../src/sdlx_sensor.c \
  ../src/sdlx_event.c \
  ../src/utils.c \
  ../src/utils_android.cpp \
  ../src/logging.c \
  ../cJSON/cJSON.c \
  ../lodepng/lodepng.c \
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

# xxx add -O2

CFLAGS = -Wall -Werror -Wno-unused-result -g -I../src $(shell pkg-config sdl3 --cflags)
CFLAGS += -I./sdl_build/local/include

build_and_run: build run

build:
	../bin/build_tools//gen_version
	gcc -o ezApp $(CFLAGS) $(SRCS) -L./sdl_build/local/lib -lSDL3 -lSDL3_ttf -lSDL3_mixer -lm

run:
	cd ../files; LD_LIBRARY_PATH=../linux/sdl_build/local/lib ../linux/ezApp

clean:
	rm -f ezApp ../src/version.h

.PHONY: build run build_and_run

