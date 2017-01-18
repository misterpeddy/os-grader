CC=gcc
CFLAGS=-w
LIBS=-pthread -lsqlite3
DEBUG=-ggdb
BIN=bin/
VPATH=src/

all: coordinator judge

coordinator: coordinator.c macros.h settings.h server.c db.c
	$(CC) $(CFLAGS) $? $(LIBS) -o $(BIN)$@

judge: judge.c macros.h
	$(CC) $(CFLAGS) $? -o $(BIN)$@

debug: coordinator.c macros.h settings.h server.c db.c
	$(CC) $(CFLAGS) $(DEBUG) $? $(LIBS) -o $(BIN)$@

clean:
	rm -rf *swp *gch $(BIN)*

