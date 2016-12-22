#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define DEBUG 1 

#define SANDBOX "sandbox"
#define LOGFILE_SUFFIX "log.txt"
#define MAX_COMMAND_LEN 512
#define MAX_FILENAME_LEN 128
#define TTY "/dev/tty"

void init_sandbox(char *user) 
{	
	char command[MAX_COMMAND_LEN];
	memset(&command, 0, MAX_COMMAND_LEN);
	sprintf(&command, "rm -rf %s && mkdir %s", user, user);
	system(command);

	// Log stdout
	char logfile[MAX_FILENAME_LEN];
	sprintf(&logfile, "%s/%s_%s", user, user, LOGFILE_SUFFIX);
	freopen(logfile, "w", stdout);
	if (DEBUG) printf("Redirecting stdout output\n$(%s)\n", command);
}

void compile_source(char *filename, char *user) 
{	
	char command[MAX_COMMAND_LEN];
	memset(&command, 0, MAX_COMMAND_LEN);
	sprintf(&command, "gcc %s -o %s/out", filename, user);
	if (DEBUG) printf("$(%s)\n", command);
	system(command);
}

void run_program(char *user) 
{
	char command[MAX_COMMAND_LEN];
	memset(&command, 0, MAX_COMMAND_LEN);
	sprintf(&command, "./%s/out", user);
	if (DEBUG) printf("$(%s)\n", command);
	system(command);	
}

void clean_up() {
	if (DEBUG) printf("Finishing stdout redirect\n");
	fflush(stdout);
	freopen(TTY, "a", stdout);
}

int main(int argc, char **argv) 
{

	// Check arguments
	if (argc != 3) {
		printf("Usage: %s <filename> <username>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// Capture arguments
	char *filename = argv[1];
	char *user = argv[2];

	if (DEBUG) printf("****Executor Started****\n");

	// Create sandbox
	init_sandbox(user);

	// Compile submitted source code
	compile_source(filename, user);

	// Run executable
	run_program(user);

	// Restore stdout
	clean_up();

	if (DEBUG) printf("****Executor Finished****\n");
}
