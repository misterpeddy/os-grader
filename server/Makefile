CC=gcc
CFLAGS=-w
LIBS=-pthread
DEBUG=-ggdb
BIN=bin/
VPATH=src/

all: coordinator judge

coordinator: coordinator.c macros.h settings.h server.h server.c
	$(CC) $(CFLAGS) $(LIBS) $? -o $(BIN)$@

judge: judge.c macros.h
	$(CC) $(CFLAGS) $? -o $(BIN)$@

debug: coordinator.c macros.h settings.h server.h server.c
	$(CC) $(CFLAGS) $(DEBUG) $(LIBS) $? -o $(BIN)$@

clean:
	rm -rf *swp *gch $(BIN)*

