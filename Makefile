game.o:
	gcc -c -o game.o game.c

connection.o:
	gcc -c -o connection.o connection.c

snakesserver: game.o connection.o
	gcc -o snakesserver snakesserver.c game.o connection.o

snakesclient: game.o connection.o
	gcc -o snakesclient snakesclient.c game.o connection.o -lncurses
