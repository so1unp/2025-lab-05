CC=gcc
CFLAGS=-Wall -Werror -g -pthread -I../directorio -lrt

PROGS=server

.PHONY: all
all: $(PROGS)

server: server.c utils.c solicitudes.c config.c
	$(CC) -o $@ $^ $(CFLAGS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(PROGS)
