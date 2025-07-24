SRCS = reversi.c cpu.c
TARGET = reversi

CFLAGS = -Wall -g -I. -I../../src $(shell sdl2-config --cflags)

$(TARGET):
	gcc -o $@ $(CFLAGS) $(SRCS) ../../src/sdl.c ../../src/utils.c ../../src/logging.c -lSDL2 -lSDL2_ttf -lm

clean:
	rm $(TARGET)

.PHONY: $(TARGET)
