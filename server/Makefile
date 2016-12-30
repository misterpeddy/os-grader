CC=gcc
CFLAGS=-w
LIBS=-pthread
BIN=bin/
VPATH=src/

all: coordinator judge

coordinator: coordinator.c macros.h server.h server.c
	$(CC) $(CFLAGS) $(LIBS) $? -o $(BIN)$@

judge: judge.c macros.h
	$(CC) $(CFLAGS) $? -o $(BIN)$@

clean:
	rm -rf *swp *gch $(BIN)*

