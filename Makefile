
CC = cc
CFLAGS = -std=c99 -pedantic -Wall -O3

all: lcstest

clean:
	rm -rf *.o lcstest

rebuild: clean all
 

lcstest.o:  Makefile
	$(CC) $(CFLAGS) -c -o lcstest.o lcstest.c 

lcstest: lcstest.o Makefile
	$(CC) $(CFLAGS) -o lcstest lcstest.o
