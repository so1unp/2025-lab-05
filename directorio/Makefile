CC=gcc
CFLAGS=-std=c99

PROGS=server clienteD

.PHONY: all
all: $(PROGS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(PROGS)