CC=gcc
CFLAGS=-w

all: macros.h judge.o
	$(CC) judge.o -o main
judge.o: judge.c macros.h
	$(CC) -c macros.h judge.c
executor: executor.c macros.h
	$(CC) $(CFLAGS) executor.c macros.h -o executor
clean:
	rm -rf *o *gch

