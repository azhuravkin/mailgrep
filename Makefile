CC = gcc -Wall -O2 -g -ggdb
LIBS = -lpcre
INSTALL_DIR = /usr/local/bin

all: mailgrep

mailgrep: Makefile mailgrep.o
	$(CC) mailgrep.o $(LIBS) -o mailgrep

mailgrep.o: Makefile mailgrep.c
	$(CC) -c mailgrep.c -o mailgrep.o

install: mailgrep
	install -o root -s mailgrep $(INSTALL_DIR)

clean:
	rm -f mailgrep.o mailgrep
