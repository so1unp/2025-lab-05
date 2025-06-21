CC=gcc
CFLAGS=-Wall -Werror -g -pthread -lncurses

PROGS=main

.PHONY: all clean

all: $(PROGS)

main: main.c seleccion-mapa.c seleccion-rol.c base.c gameover.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(PROGS)
