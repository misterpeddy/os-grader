CC=gcc
CFLAGS=-w

all: proto.o
	$(CC) proto.o -o main
proto.o: proto.c
	$(CC) -c proto.c
executor: executor.c
	$(CC) $(CFLAGS) executor.c -o executor
clean:
	rm -rf *o *gch

