SRCS   = hello.c utils.c
TARGET = hello

CFLAGS = -Wall -g -I. -I../../android/src $(shell sdl2-config --cflags)

$(TARGET):
	gcc -o $@ $(CFLAGS) $(SRCS) ../../android/src/sdl.c -lSDL2 -lSDL2_ttf -lm

clean:
	rm $(TARGET)

.PHONY: $(TARGET)
