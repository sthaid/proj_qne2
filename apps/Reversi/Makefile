TARGET  = Reversi

SRCS    = reversi.c cpu.c

SRCS_COMMON = \
       ../../src/sdl.c \
       ../../src/sdl_audio.c \
       ../../src/sdl_sensor.c \
       ../../src/utils.c \
       ../../src/logging.c

FONT_FILE = FreeMonoBold.ttf

CFLAGS = -Wall -Werror -Wno-unused-result -g -I. -I../../src -fcommon $(shell pkg-config sdl3--cflags)

run_linux: setup
	gcc -o $(TARGET) $(CFLAGS) $(SRCS) $(SRCS_COMMON) -lSDL3 -lSDL3_ttf -lm
	$(TARGET) data_dir

run_picoc: setup
	../../picoc/picoc -n $(TARGET) $(SRCS) - data_dir

setup:
	mkdir -p data_dir
	mkdir -p apps/Reversi
	ln -sf ../../common.h apps/Reversi
	ln -sf ../../android/assets/$(FONT_FILE) .

clean:
	rm -rf $(TARGET) $(FONT_FILE) data_dir apps

.PHONY: setup clean
