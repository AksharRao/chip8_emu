CFLAGS=-std=c17 -Wall -Wextra -Werror
all:
	clang chip8.c -o chip8 $(CFLAGS) `sdl2-config --cflags --libs`
	
debug:
	clang chip8.c -o chip8 $(CFLAGS) `sdl2-config --cflags --libs` -DDEBUG
