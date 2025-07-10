SRCS = hello.c
TARGET = hello

CFLAGS = -Wall -g -I. -I../../.. $(shell sdl2-config --cflags)

$(TARGET):
	gcc -o $@ $(CFLAGS) $(SRCS) ../../../sdl.c ../../../utils.c -lSDL2 -lSDL2_ttf -lm

#xxx why utils?

clean:
	rm $(TARGET)

.PHONY: $(TARGET)
