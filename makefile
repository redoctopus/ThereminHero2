CC = gcc

CFLAGS = -I/usr/local/include
LDLIBS = -lSDL2 -lSDL2_ttf
LFLAGS = -L/usr/local/lib

theremin: theremingame.o
	$(CC) -o theremin theremin.c theremingame.o $(LFLAGS) $(LDLIBS)

