CC=gcc
CFLAGS=-Wall -Werror -g -pthread 

PROGS=server clienteD

.PHONY: all
all: $(PROGS)

server: server.c
	$(CC) -o $@ $< $(CFLAGS)

clienteD: clienteD.c
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(PROGS)
