CC=gcc
CFLAGS=-w

all: receiver judge

receiver: receiver.c macros.h
	$(CC) -w -pthread receiver.c macros.h -o receiver
judge: judge.c macros.h
	$(CC) -w $(CFLAGS) judge.c macros.h -o judge
clean:
	rm -rf *o *gch

