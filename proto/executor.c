#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEBUG 1 

#define SANDBOX "sandbox"
#define LOGFILE_SUFFIX "log.txt"
#define ERRORFILE_SUFFIX "error.txt"
#define OUTFILE_SUFFIX "out.txt"
#define BIN "bin"
#define MAX_COMMAND_LEN 512
#define MAX_FILENAME_LEN 128
#define TTY "/dev/tty"
#define STAR "*****"
#define DELIM 0x2A

const char COMP_AOK[] = "COMP_AOK\0";
const char COMP_ERR[] = "COMP_ERR\0";
const char RUN_AOK[] = "RUN_AOK\0";
const char RUN_ERR[] = "RUN_ERR\0";

char log_file[MAX_FILENAME_LEN];
char err_file[MAX_FILENAME_LEN];

void init_sandbox(char *user) 
{	
	if (DEBUG) printf("%sExecutor Started%s\n", STAR, STAR);
	
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
		printf("%sRedirecting stdout output%s\n", STAR, STAR);
		printf("$(%s)\n", command);
	}
}

/* 
** Compiles C file with name filename for user
** Returns 0 if compiled with no errors otherwise positive number 
*/
int compile_source(char *filename, char *user) 
{	
	if (DEBUG) printf("\n%sCompiling %s for %s%s\n", STAR, filename, user, STAR);

	// Compile source
	char command[MAX_COMMAND_LEN];
	memset(&command, 0, MAX_COMMAND_LEN);
	sprintf(&command, "gcc -w %s -o %s/%s", filename, user, BIN);
	if (DEBUG) printf("$(%s)\n", command);
	system(command);
	
	// Read error log file stats
	struct stat log_stat;
	stat(err_file, &log_stat);
	
	if (DEBUG) printf("%sFinished compiling %s for %s%s\n\n", STAR, filename, user, STAR);

	return log_stat.st_size;
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
	if (DEBUG) printf("%sExecutor Finished%s\n", STAR, STAR);
}

/********************* Helpers ***********************/

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
	if (argc != 4) {
		printf("Usage: %s <filename> <username> <pipe fd>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// Capture arguments
	char *filename = argv[1];
	char *user = argv[2];
	int pipe_fd = atoi(argv[3]);

	// Create sandbox
	init_sandbox(user);

	// Compile submitted source code 
	int comp_result = compile_source(filename, user);

	// Write compilation result code to pipe, exit if errored
	if (comp_result)
	{
		send_ack(pipe_fd, COMP_ERR, user, filename);
		clean_up();
		exit(1);
	}
	else
	{
		send_ack(pipe_fd, COMP_AOK, user, filename);
	}

	// Run executable
	run_program(user);

	// Restore stdout and stderr 
	clean_up();

}
