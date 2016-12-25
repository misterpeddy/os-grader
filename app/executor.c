#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "macros.h"

char log_file[MAX_FILENAME_LEN];
char err_file[MAX_FILENAME_LEN];

void init_sandbox(char *user) 
{	
	if (DEBUG) printf("%sExecutor Started%s\n", FILLER, FILLER);
	
	// Make empty directory for sandbox
	char command[MAX_COMMAND_LEN];
	memset(&command, 0, MAX_COMMAND_LEN);
	sprintf(&command, "rm -rf %s && mkdir %s", user, user);
	system(command);

	// Log stdout
	sprintf(&log_file, "%s/%s_%s", user, user, LOGFILE_SUFFIX);
	freopen(log_file, "w", stdout);

	// Log stderr
	sprintf(&err_file, "%s/%s_%s", user, user, ERRORFILE_SUFFIX);
	freopen(err_file, "w", stderr);

	if (DEBUG) 
	{
		printf("%sRedirecting stdout output%s\n", FILLER, FILLER);
		printf("$(%s)\n", command);
	}
}

/* 
** Compiles C file with name filename for user
** Returns 0 if compiled with no errors otherwise positive number 
*/
int compile_source(char *filename, char *user) 
{	
	if (DEBUG) printf("\n%sCompiling %s for %s%s\n", FILLER, filename, user, FILLER);

	// Compile source
	char command[MAX_COMMAND_LEN];
	memset(&command, 0, MAX_COMMAND_LEN);
	sprintf(&command, "gcc -w %s -o %s/%s", filename, user, BIN);
	if (DEBUG) printf("$(%s)\n", command);
	system(command);
	
	if (DEBUG) printf("%sFinished compiling %s for %s%s\n\n", FILLER, filename, user, FILLER);

	// Read error log file stats
	struct stat log_stat;
	stat(err_file, &log_stat);
	
	return log_stat.st_size;
}

void run_program(char *user, char *input_file) 
{
	if (DEBUG) printf("\n%sRunning %s/%s%s\n", FILLER, user, BIN, FILLER);

	// Redirect output to OUT
	char logfile[MAX_FILENAME_LEN];
	sprintf(&logfile, "%s/%s_%s", user, user, OUTFILE_SUFFIX);
	freopen(logfile, "w", stdout);

	// Run the binary
	char command[MAX_COMMAND_LEN];
	memset(&command, 0, MAX_COMMAND_LEN);
	if (input_file)
	{
		sprintf(&command, "./%s/%s < %s", user, BIN, input_file);
	} else 
	{
		sprintf(&command, "./%s/%s", user, BIN);
	}
	system(command);	

	// Redirect output to logfile
	memset(&logfile, 0, MAX_FILENAME_LEN);
	sprintf(&logfile, "%s/%s_%s", user, user, LOGFILE_SUFFIX);
	freopen(logfile, "a", stdout);
	
	if (DEBUG) printf("$(%s)\n", command);
	if (DEBUG) printf("%sFinished running %s/%s%s\n\n", FILLER, user, BIN, FILLER);
}

void clean_up() 
{
	if (DEBUG) printf("%sFinishing stdout redirect%s\n\n", FILLER, FILLER);
	fflush(stdout);
	fflush(stderr);
	freopen(TTY, "a", stderr);
	freopen(TTY, "a", stdout);
	if (DEBUG) printf("%sExecutor Finished%s\n", FILLER, FILLER);
}

/********************* Helpers ***********************/
/*
** Writes acknowledgement (<user><delimiter><filename><delimiter><ack_code>\0)
** onto the pipe.
*/
void send_ack(int pipe_fd, const char *ack_str, char *user, char *filename) {
	// Allocate space for the message
	char *buf = (char *) malloc(strlen(ack_str) + strlen(user) + strlen(filename) + 3);
	
	// Concat the user, filename and acknowledgement string
	strcpy(buf, user);
	strcpy(buf + strlen(user) + 1, filename);
	strcpy(buf + strlen(user) + strlen(filename) + 2, ack_str);

	// Change the null terminators to delimiters
	buf[strlen(user)] = DELIM;
	buf[strlen(user) + 1 + strlen(filename)] = DELIM;

	// Write the message to the pipe
	write(pipe_fd, buf, strlen(buf));

	// Deallocate message
	free(buf);
}

int main(int argc, char **argv) 
{

	// Check arguments
	if (argc < 4 || argc > 5) {
		printf("Usage: %s <source file> <username> <pipe fd> [<input file>]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// Capture arguments
	char *source_file = argv[1];
	char *user = argv[2];
	int pipe_fd = atoi(argv[3]);
	char *input_file = (argc == 5) ? argv[4] : NULL;

	// Create sandbox
	init_sandbox(user);

	// Log arguments
	if (DEBUG)
	{
		printf("%sArgs: ", FILLER);
		for (int i=0; i<argc; i++) 
			printf("<%s>", argv[i]);
		printf("%s\n", FILLER);
	}
	// Compile submitted source code 
	int comp_result = compile_source(source_file, user);

	// Write compilation result code to pipe, exit if errored
	if (comp_result)
	{
		send_ack(pipe_fd, COMP_ERR, user, source_file);
		clean_up();
		exit(1);
	}
	else
	{
		send_ack(pipe_fd, COMP_AOK, user, source_file);
	}

	// Run executable
	run_program(user, "input1.txt");

	// Run executable
	run_program(user, "input2.txt");

	// Restore stdout and stderr 
	clean_up();

}
