
CC = cc
# CFLAGS = -std=c99 -pedantic -Wall -O3
CFLAGS = -std=c11 -mpopcnt -pedantic -Wall -O3
#CFLAGS =  -Wall -O3
#CFLAGS = clang -O3  -funroll-loops


all: lcstest

clean:
	rm -rf *.o lcstest

rebuild: clean all
 

lcstest.o:  Makefile
	$(CC) $(CFLAGS) -c -o lcstest.o lcstest.c 

lcstest: lcstest.o Makefile
	$(CC) $(CFLAGS) -o lcstest lcstest.o
