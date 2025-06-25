CC=gcc
CFLAGS=-Wall -Werror -g -pthread -lncurses -std=c99 -D_XOPEN_SOURCE=700

PROGS=main

.PHONY: all clean

all: $(PROGS)

main: main.c seleccion-mapa.c seleccion-rol.c base.c gameover.c jugador.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(PROGS)
