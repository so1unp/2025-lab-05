CC=gcc
CFLAGS=-Wall -Werror -g -pthread -lncurses -std=c99 -D_XOPEN_SOURCE=700 -lrt -lc

PROGS=main

.PHONY: all clean

all: $(PROGS)

main: main.c seleccion-rol.c seleccion-mapa.c base.c jugador.c pantalla-final.c colores.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(PROGS)