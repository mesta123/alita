all:
	gcc main.c background.c -o game `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf
