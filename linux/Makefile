
SRCS :=  \
  ../src/main.c \
  ../src/sdlx_misc.c \
  ../src/sdlx_video.c \
  ../src/sdlx_audio.c \
  ../src/sdlx_sensor.c \
  ../src/sdlx_event.c \
  ../src/utils.c \
  ../src/utils_location.cpp \
  ../src/logging.c \
  ../cJSON/cJSON.c \
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

CFLAGS = -Wall -Werror -Wno-unused-result -g -I../src $(shell pkg-config sdl3 --cflags)

$(TARGET):
	../bin/gen_version
	gcc -o $@ $(CFLAGS) $(SRCS) -lSDL3 -lSDL3_ttf -lm

run: $(TARGET)
	cd ../files; ../linux/$(TARGET)

clean:
	rm -f $(TARGET) ../src/version.h

.PHONY: $(TARGET)

