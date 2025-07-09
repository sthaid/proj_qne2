SRCS = reversi.c human.c cpu.c
TARGET = reversi

CFLAGS = -Wall -g -I. -I../../.. $(shell sdl2-config --cflags)

$(TARGET):
	gcc -o $@ $(CFLAGS) $(SRCS) ../../../sdl.c ../../../utils.c -lSDL2 -lSDL2_ttf

clean:
	rm $(TARGET)

.PHONY: $(TARGET)
