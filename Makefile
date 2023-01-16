CC=musl-gcc
CFLAGS= -std=c99 -Wall -pedantic

all: boolx compactorx

boolx:
	$(CC) -o bin/boolx src/interpreter.c

compactorx:
	$(CC) -o bin/compactorx src/compactor.c

clean:
	rm bin/boolx bin/compactorx

