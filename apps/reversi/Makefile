TARGET = reversi

SRCS   = reversi.c cpu.c

SRCS_COMMON = \
       ../../src/sdl.c \
       ../../src/sdl_audio.c \
       ../../src/utils.c \
       ../../src/logging.c

CFLAGS = -Wall -Werror -Wno-unused-result -g -I. -I../..//src $(shell pkg-config sdl3--cflags)

build:
	gcc -o $(TARGET) $(CFLAGS) $(SRCS) $(SRCS_COMMON) -lSDL3 -lSDL3_ttf -lm

run_linux: build
	./$(TARGET)

run_picoc:
	../../picoc/picoc $(SRCS)

clean:
	rm -f $(TARGET)

.PHONY: build clean
