CC=gcc
CFLAGS=-w

all: client

client: client.c client.h
	$(CC) $(CFLAGS) $? -o $@

clean:
	rm -rf *swp *gch client
