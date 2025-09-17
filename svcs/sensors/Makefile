TARGET  = sensors

SRCS    = sensors.c

SRCS_COMMON = \
       ../../src/sdl_misc.c \
       ../../src/sdl_video.c \
       ../../src/sdl_audio.c \
       ../../src/sdl_sensor.c \
       ../../src/sdl_event.c \
       ../../src/utils.c \
       ../../src/logging.c

CFLAGS = -Wall -Werror -Wno-unused-result -g -I. -I../../src -fcommon $(shell pkg-config sdl3--cflags)

build: setup
	gcc -o $(TARGET) $(CFLAGS) $(SRCS) $(SRCS_COMMON) -lSDL3 -lSDL3_ttf -lm

run_linux: setup
	gcc -o $(TARGET) $(CFLAGS) $(SRCS) $(SRCS_COMMON) -lSDL3 -lSDL3_ttf -lm
	$(TARGET) data_dir 0

run_picoc: setup
	../../picoc/picoc -n $(TARGET) $(SRCS) - data_dir 0

setup:
	mkdir -p data_dir

clean:
	rm -rf $(TARGET) data_dir

.PHONY: build setup clean
