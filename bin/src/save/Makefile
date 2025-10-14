JPEG_TO_PIXELS_SRCS = jpeg_to_pixels.c \
                      ../../src/sdlx_misc.c \
                      ../../src/sdlx_video.c \
                      ../../src/sdlx_audio.c \
                      ../../src/sdlx_sensor.c \
                      ../../src/sdlx_event.c \
                      ../../src/logging.c 

all: ../ezsh

../jpeg_to_pixels: $(JPEG_TO_PIXELS_SRCS)
	gcc -Wall -o $@ -I../../src $(shell pkg-config sdl3 --cflags) $^ -lm -ljpeg -lpng -lSDL3 -lSDL3_ttf
	#../jpeg_to_pixels

../ezsh: ezsh.c
	gcc -Wall -o ../ezsh -Wall ezsh.c

clean:
	rm -f ../ezsh ../jpeg_to_pixels
