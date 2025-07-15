SRCS   = hello.c
TARGET = hello

CFLAGS = -Wall -g -I. -I../../.. $(shell sdl2-config --cflags)

t1:
	gcc -o $(TARGET) $(CFLAGS) $(SRCS) ../../../sdl.c -lSDL2 -lSDL2_ttf -lm
        

clean:
	rm $(TARGET)

.PHONY: $(TARGET)
