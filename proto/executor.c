#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG 1 

#define SANDBOX "sandbox"
#define LOGFILE "log.txt"
#define MAX_BUF 512
#define TTY "/dev/tty"

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	char command[MAX_BUF];

	if (DEBUG) printf("****Executor Started****\n");

	// Capture output
	freopen(LOGFILE, "w", stdout);

	// Create sandbox
	memset(&command, 0, MAX_BUF);
	sprintf(&command, "rm -rf %s && mkdir %s", SANDBOX, SANDBOX);
	if (DEBUG) printf("$(%s)\n", command);
	system(command);

	// Compile submittedsource code
	memset(&command, 0, MAX_BUF);
	sprintf(&command, "gcc %s -o %s/out", argv[1], SANDBOX);
	if (DEBUG) printf("$(%s)\n", command);
	system(command);

	// Run executable
	memset(&command, 0, MAX_BUF);
	sprintf(&command, "./%s/out", SANDBOX);
	if (DEBUG) printf("$(%s)\n", command);
	system(command);
	
	// Restore stdout
	fflush(stdout);
	freopen(TTY, "a", stdout);

	if (DEBUG) printf("****Executor Finished****\n");
}
