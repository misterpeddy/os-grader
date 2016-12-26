CC=gcc
CFLAGS=-w
LIBS=-pthread

all: receiver judge

receiver: receiver.c macros.h
	$(CC) $(CFLAGS) $(LIBS) receiver.c macros.h -o receiver
judge: judge.c macros.h
	$(CC) $(CFLAGS) judge.c macros.h -o judge
clean:
	rm -rf *o *gch

