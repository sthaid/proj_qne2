TARGET = Env

SRCS   = apps/$(TARGET)/env.c

# the following should not need to be changed

SRCS_COMMON = \
       ../src/sdl_misc.c \
       ../src/sdl_video.c \
       ../src/sdl_audio.c \
       ../src/sdl_sensor.c \
       ../src/sdl_event.c \
       ../src/utils.c \
       ../src/logging.c

CFLAGS = -Wall -Werror -Wno-unused-result -g -I. -I../src -fcommon $(shell pkg-config sdl3--cflags)

DATA_DIR = apps_data/$(TARGET)

build: 
	cd ../..; gcc -o /tmp/$(TARGET) $(CFLAGS) $(SRCS) $(SRCS_COMMON) -lSDL3 -lSDL3_ttf -lm

run_linux: build
	cd ../..; mkdir -p $(DATA_DIR)
	cd ../..; /tmp/$(TARGET) $(DATA_DIR) 0

run_picoc: 
	cd ../..; mkdir -p $(DATA_DIR)
	cd ../..; ../picoc/picoc -n $(TARGET) $(SRCS) - $(DATA_DIR) 0

clean:
	rm -f /tmp/$(TARGET)

.PHONY: build clean run_linux run_picoc
