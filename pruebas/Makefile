CC=gcc
CFLAGS=-Wall -Werror -g -pthread -lncurses

PROGS=raider guardian

.PHONY: all
all: $(PROGS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(PROGS)
