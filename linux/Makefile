
SRCS :=  \
  main.c sdl.c utils.c \
  picoc/clibrary.c \
  picoc/debug.c \
  picoc/expression.c \
  picoc/heap.c \
  picoc/include.c \
  picoc/lex.c \
  picoc/parse.c \
  picoc/picoc_android.c \
  picoc/platform.c \
  picoc/table.c \
  picoc/type.c \
  picoc/variable.c \
  picoc/cstdlib/ctype.c \
  picoc/cstdlib/errno.c \
  picoc/cstdlib/math.c \
  picoc/cstdlib/stdbool.c \
  picoc/cstdlib/string.c \
  picoc/cstdlib/stdio.c \
  picoc/cstdlib/stdlib.c \
  picoc/cstdlib/time.c \
  picoc/cstdlib/unistd.c \
  picoc/platform/library_unix.c \
  picoc/platform/platform_unix.c

TARGET = app

CFLAGS = -Wall -g -I. $(shell sdl2-config --cflags)

$(TARGET):
	gcc -o $@ $(CFLAGS) $(SRCS) -lSDL2 -lSDL2_ttf -lm

clean:
	rm $(TARGET)

.PHONY: $(TARGET)

