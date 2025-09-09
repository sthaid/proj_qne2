TARGET = t1

SRCS   = t1.c

SRCS_COMMON = \
       ../../src/sdl.c \
       ../../src/sdl_audio.c \
       ../../src/sdl_sensor.c \
       ../../src/utils.c \
       ../../src/logging.c

FONT_FILE=FreeMonoBold.ttf

CFLAGS = -Wall -Werror -Wno-unused-result -g -I. -I../../src $(shell pkg-config sdl3--cflags)

build:
	gcc -o $(TARGET) $(CFLAGS) $(SRCS) $(SRCS_COMMON) -lSDL3 -lSDL3_ttf -lm

run_linux: build
	ln -sf ../../android/assets/$(FONT_FILE) .
	./$(TARGET)
	rm -f $(FONT_FILE) $(TARGET)

run_picoc:
	ln -sf ../../android/assets/$(FONT_FILE) .
	../../picoc/picoc $(SRCS) - apps/t1 123456789abcd
	rm -f $(FONT_FILE)

clean:
	rm -f $(TARGET) FreeMonoBold.ttf params
	rm -f recording.raw

.PHONY: build clean
