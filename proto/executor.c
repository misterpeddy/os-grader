#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define DEBUG 1 

#define SANDBOX "sandbox"
#define LOGFILE_SUFFIX "log.txt"
#define ERRORFILE_SUFFIX "error.txt"
#define BIN "bin"
#define OUTFILE_SUFFIX "out.txt"
#define MAX_COMMAND_LEN 512
#define MAX_FILENAME_LEN 128
#define TTY "/dev/tty"
#define STAR "*****"

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

	// Log stderr
	memset(&logfile, 0, MAX_FILENAME_LEN);
	sprintf(&logfile, "%s/%s_%s", user, user, ERRORFILE_SUFFIX);
	freopen(logfile, "w", stderr);

	if (DEBUG) 
	{
		printf("%sRedirecting stdout output%s\n", STAR, STAR);
		printf("$(%s)\n", command);
	}
}

void compile_source(char *filename, char *user) 
{	
	if (DEBUG) printf("\n%sCompiling %s for %s%s\n", STAR, filename, user, STAR);

	char command[MAX_COMMAND_LEN];
	memset(&command, 0, MAX_COMMAND_LEN);
	sprintf(&command, "gcc %s -o %s/%s", filename, user, BIN);
	if (DEBUG) printf("$(%s)\n", command);
	system(command);

	if (DEBUG) printf("%sFinished compiling %s for %s%s\n\n", STAR, filename, user, STAR);
}

void run_program(char *user) 
{
	if (DEBUG) printf("\n%sRunning %s/%s%s\n", STAR, user, BIN, STAR);

	// Redirect output to OUT
	char logfile[MAX_FILENAME_LEN];
	sprintf(&logfile, "%s/%s_%s", user, user, OUTFILE_SUFFIX);
	freopen(logfile, "w", stdout);

	// Run the binary
	char command[MAX_COMMAND_LEN];
	memset(&command, 0, MAX_COMMAND_LEN);
	sprintf(&command, "./%s/%s", user, BIN);
	system(command);	

	// Redirect output to logfile
	memset(&logfile, 0, MAX_FILENAME_LEN);
	sprintf(&logfile, "%s/%s_%s", user, user, LOGFILE_SUFFIX);
	freopen(logfile, "a", stdout);
	
	if (DEBUG) printf("$(%s)\n", command);
	if (DEBUG) printf("%sFinished running %s/%s%s\n\n", STAR, user, BIN, STAR);
}

void clean_up() 
{
	if (DEBUG) printf("%sFinishing stdout redirect%s\n\n", STAR, STAR);
	fflush(stdout);
	fflush(stderr);
	freopen(TTY, "a", stderr);
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
