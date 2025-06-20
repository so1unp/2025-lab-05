CC=gcc
CFLAGS=-std=c99
LDFLAGS=-pthread

PROGS=server clienteD

.PHONY: all
all: $(PROGS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(PROGS)