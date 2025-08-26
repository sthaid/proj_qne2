TARGET = reversi

SRCS   = reversi.c cpu.c

SRCS_COMMON = \
       ../../src/sdl.c \
       ../../src/sdl_audio.c \
       ../../src/sdl_sensor.c \
       ../../src/utils.c \
       ../../src/logging.c

FONT_FILE=../../android/assets/FreeMonoBold.ttf

CFLAGS = -Wall -Werror -Wno-unused-result -g -I. -I../../src $(shell pkg-config sdl3--cflags)

build:
	gcc -o $(TARGET) $(CFLAGS) $(SRCS) $(SRCS_COMMON) -lSDL3 -lSDL3_ttf -lm

run_linux: build
	ln -sf $(FONT_FILE) .
	./$(TARGET)

run_picoc:
	ln -sf $(FONT_FILE) .
	../../picoc/picoc $(SRCS)

clean:
	rm -f $(TARGET) FreeMonoBold.ttf params

.PHONY: build clean
