CC=gcc
CFLAGS=-Wall -Werror -g -pthread -I../directorio

PROGS=server

.PHONY: all
all: $(PROGS)


%: %.c
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(PROGS)
