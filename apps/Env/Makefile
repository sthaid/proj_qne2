TARGET  = Env

SRCS    = apps/Env/env.c

SRCS_COMMON = \
       src/sdl_misc.c \
       src/sdl_video.c \
       src/sdl_audio.c \
       src/sdl_sensor.c \
       src/sdl_event.c \
       src/utils.c \
       src/logging.c

FONT_FILE = FreeMonoBold.ttf

CFLAGS = -Wall -Werror -Wno-unused-result -g -Isrc -I. -fcommon $(shell pkg-config sdl3--cflags)

build: 
	cd ../..; gcc -o $(TARGET) $(CFLAGS) $(SRCS) $(SRCS_COMMON) -lSDL3 -lSDL3_ttf -lm

run_linux: 
	cd ../..; gcc -o $(TARGET) $(CFLAGS) $(SRCS) $(SRCS_COMMON) -lSDL3 -lSDL3_ttf -lm
	apps/Env/$(TARGET) apps_data/Env

run_picoc: setup
	../../picoc/picoc -n $(TARGET) $(SRCS) - data_dir

setup:
	mkdir -p data_dir
	mkdir -p apps/Env
	ln -sf ../../common.h apps/Env
	ln -sf ../../android/assets/$(FONT_FILE) .

clean:
	rm -rf $(TARGET) $(FONT_FILE) data_dir apps

.PHONY: build setup clean
