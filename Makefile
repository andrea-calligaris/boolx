CC	= musl-gcc
CFLAGS	= -std=c99 -Wall -pedantic
LDFLAGS	= -static -s

all: bin/boolx bin/compactorx

bin/boolx: src/interpreter.c
	$(CC) -o bin/boolx src/interpreter.c $(CFLAGS) $(LDFLAGS)

bin/compactorx: src/compactor.c
	$(CC) -o bin/compactorx src/compactor.c $(CFLAGS) $(LDFLAGS)

clean:
	rm bin/boolx bin/compactorx

