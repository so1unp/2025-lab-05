CC=gcc
CFLAGS=-Wall -Werror -g -pthread -lncurses -std=c99 -D_XOPEN_SOURCE=700 -lrt -lc

PROGS=main

.PHONY: all clean

all: $(PROGS)

main: main.c seleccion-mapa.c seleccion-rol.c base.c jugador.c gameOver.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(PROGS)
