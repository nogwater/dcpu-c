CFLAGS=-Wall -g

all: tests

tests: dcpu.o

clean:
	rm -f *.o tests
	rm -r tests.dSYM
