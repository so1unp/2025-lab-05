CC=gcc
CFLAGS=-Wall -Werror -g -pthread -lncurses

PROGS=ncurses-pong

.PHONY: all
all: $(PROGS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(PROGS)
