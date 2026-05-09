enigme: main.o enigme.o
	gcc main.o enigme.o -o enigme -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer 
main.o: main.c enigme.h 
	gcc -c main.c 
enigme.o: enigme.c enigme.h 
	gcc -c enigme.c

