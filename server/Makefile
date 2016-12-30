CC=gcc
CFLAGS=-w
LIBS=-pthread
BIN=bin/
VPATH=src/

all: coordinator judge server

coordinator: coordinator.c macros.h
	$(CC) $(CFLAGS) $(LIBS) $? -o $(BIN)$@

judge: judge.c macros.h
	$(CC) $(CFLAGS) $? -o $(BIN)$@

server: server.c macros.h
	$(CC) $(CFLAGS) $? -o $(BIN)$@

clean:
	rm -rf *swp *gch $(BIN)*

