CC ?= cc
BIN = myers
PREFIX ?= /usr/local

all:
	cc main.c -l curses -o $(BIN)
clean:
	rm -f $(BIN)
install:
	mkdir -p $(PREFIX)/bin
	cp -f $(BIN) $(PREFIX)/bin

.PHONY: all clean
