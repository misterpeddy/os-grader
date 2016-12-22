C=gcc

all: proto.o
	$(CC) proto.o -o main
proto.o: proto.c
	$(CC) -c proto.c
executor.o: executor.c
	$(CC) executor.c -o executor
clean:
	rm -rf *o *gch

