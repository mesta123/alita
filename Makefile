
game: main.o ennemi.o
	gcc main.o ennemi.o -o game -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

main.o: main.c ennemi.h
	gcc -c main.c

ennemi.o: ennemi.c ennemi.h
	gcc -c ennemi.c

 
