CC=musl-gcc
CFLAGS= -std=c99 -Wall -pedantic
LDFLAGS  = -static -s

all: boolx compactorx

boolx:
	$(CC) -o bin/boolx src/interpreter.c $(CFLAGS) $(LDFLAGS)

compactorx:
	$(CC) -o bin/compactorx src/compactor.c $(CFLAGS) $(LDFLAGS)

clean:
	rm bin/boolx bin/compactorx

