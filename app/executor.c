#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "macros.h"

char log_file[MAX_FILENAME_LEN];
char err_file[MAX_FILENAME_LEN];
char out_file[MAX_FILENAME_LEN];
char diff_file[MAX_FILENAME_LEN];

void init_sandbox(char *user, char *ass_num) {	
	if (DEBUG) printf("%sExecutor Started%s\n", FILLER, FILLER);
	
	// Make sure user directory exists
	char command1[MAX_COMMAND_LEN];
	sprintf(&command1, "mkdir %s", user);
	if (access(user, F_OK) == -1) system(command1);
	
	// Remove assignment directory
	char command2[MAX_COMMAND_LEN];
	sprintf(&command2, "rm -rf %s/%s", user, ass_num);
	system(command2);

	// Create assignment directory
	char command3[MAX_COMMAND_LEN];
	sprintf(&command3, "mkdir %s/%s", user, ass_num);
	system(command3);

	// Log stdout
	sprintf(&log_file, "%s/%s/%s_%s_%s", user, ass_num, user, ass_num, LOGFILE_SUFFIX);
	freopen(log_file, "w", stdout);

	// Log stderr
	sprintf(&err_file, "%s/%s/%s_%s_%s", user, ass_num, user, ass_num, ERRORFILE_SUFFIX);
	freopen(err_file, "w", stderr);

	if (DEBUG) 
	{
		printf("%sRedirecting stdout output%s\n", FILLER, FILLER);
		printf("$(%s)\n$(%s)\n$(%s)\n", command1, command2, command3);
	}
}

/* 
** Compiles C file (temp/filename) for user for assignment ass_num
** Binary will be at user/ass_num/bin
** Returns 0 if compiled with no errors otherwise positive number 
*/
int compile_source(char *filename, char *user, char *ass_num) 
{	
	if (DEBUG) printf("\n%sCompiling assignment %s [%s] for %s%s\n", 
		FILLER, ass_num, filename, user, FILLER);

	// Compile source
	char command[MAX_COMMAND_LEN];
	memset(&command, 0, MAX_COMMAND_LEN);
	sprintf(&command, "gcc -w %s/%s -o %s/%s/%s", TEMP, filename, user, ass_num, BIN);
	if (DEBUG) printf("$(%s)\n", command);
	system(command);
	
	if (DEBUG) printf("%sFinished compiling assignment %s [%s] for %s%s\n\n", 
		FILLER, ass_num, filename, user, FILLER);

	// Read error log file stats
	struct stat log_stat;
	stat(err_file, &log_stat);
	
	return log_stat.st_size;
}

int run_program(char *user, char *ass_num, char *input_file) {
	if (DEBUG) printf("\n%sRunning %s/%s/%s%s\n", FILLER, user, ass_num, BIN, FILLER);

	// Redirect output to OUT
	sprintf(&out_file, "%s/%s/%s_%s_%s_%s", 
		user, ass_num, user, ass_num, OUTFILE_SUFFIX, input_file);
	freopen(out_file, "w", stdout);

	// Run the binary
	char command[MAX_COMMAND_LEN];
	memset(&command, 0, MAX_COMMAND_LEN);
	if (input_file) {
		sprintf(&command, "./%s/%s/%s < %s", user, ass_num, BIN, input_file);
	} else {
		sprintf(&command, "./%s/%s/%s", user, ass_num, BIN);
	}
	system(command);	

	// Redirect output to logfile
	freopen(log_file, "a", stdout);
	
	if (DEBUG) printf("$(%s)\n", command);
	if (DEBUG) printf("%sFinished running %s/%s/%s%s\n\n", FILLER, user, ass_num, BIN, FILLER);

	// Read error log file stats
	struct stat log_stat;
	stat(err_file, &log_stat);
	return log_stat.st_size;
}

/*
** Compares the following 2 files:
** <user>/<user>_out_<input_file>
** master/out_<input_file>
*/
int judge(char *user, char *ass_num, char *input_file) {
	if (DEBUG) printf("\n%sJudging assignment %s with input %s for %s%s\n", 
		FILLER, ass_num, input_file, user, FILLER);

	// Set up file to send diff to
	sprintf(&diff_file, "%s/%s/%s_%s_%s", user, ass_num, user, ass_num, DIFF_SUFFIX);

	// Set up master file path
	char master_file[MAX_FILENAME_LEN];
	sprintf(&master_file, "%s/%s_%s", MASTER_DIR, OUTFILE_SUFFIX, input_file); 

	// Execute the diff and write to the diff file
	char command[MAX_COMMAND_LEN];
	sprintf(&command, "diff %s %s > %s", out_file, master_file, diff_file);
	if (DEBUG) printf("$(%s)\n", command);
	int return_code = system(command);
	
	if (DEBUG) printf("%sFinished judging assignment %s with input %s for %s%s\n\n", 
		FILLER, ass_num, input_file, user, FILLER);

	// Read error log file stats
	struct stat log_stat;
	stat(diff_file, &log_stat);
	
	return log_stat.st_size + return_code;

}

void clean_and_exit(int code) {
	if (DEBUG) printf("%sFinishing stdout redirect%s\n\n", FILLER, FILLER);
	fflush(stdout);
	fflush(stderr);
	freopen(TTY, "a", stderr);
	freopen(TTY, "a", stdout);
	if (DEBUG) printf("%sExecutor Finished%s\n", FILLER, FILLER);

	// Exit with the supplied exit code
	exit(code);
}

/******************************* Helpers ***********************************/
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

	// Change null terminators to delimiters
	buf[strlen(user)] = DELIM;
	buf[strlen(user) + 1 + strlen(filename)] = DELIM;

	// Write the message to the pipe
	write(pipe_fd, buf, strlen(buf));

	// Deallocate message
	free(buf);
}

int main(int argc, char **argv) {

	// Check arguments
	if (argc < 5) {
		printf("Usage: %s <source file> <username> \
			<assignment number> <pipe fd> [<input files>...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// Capture arguments
	char *source_file = argv[1];
	char *user = argv[2];
	char *ass_num = argv[3];
	int pipe_fd = atoi(argv[4]);
	int num_input_files = argc - 5; 
	char **input_files = &argv[5];
	
	// Create sandbox
	init_sandbox(user, ass_num);

	// Log arguments
	if (DEBUG) {
		printf("%sArgs: ", FILLER);
		for (int i=0; i<argc; i++) 
			printf("<%s>", argv[i]);
		printf("%s\n", FILLER);
	}

	// Compile submitted source code 
	int comp_result = compile_source(source_file, user, ass_num);

	// Write compilation result code to pipe, exit if errored
	if (comp_result) {
		send_ack(pipe_fd, COMP_ERR, user, source_file);
		clean_and_exit(EXIT_FAILURE);
	} else {
		send_ack(pipe_fd, COMP_AOK, user, source_file);
	}

	for (int i=0; i<num_input_files; i++) {

		// Run and exit if errored
		if (run_program(user, ass_num, input_files[i])) {
			send_ack(pipe_fd, RUN_ERR, user, source_file);
			clean_and_exit(EXIT_FAILURE);
		}

		send_ack(pipe_fd, RUN_AOK, user, source_file);

		// Judge the output against master
		if (judge(user, ass_num, input_files[i])) {
			send_ack(pipe_fd, CHK_ERR, user, source_file);
			clean_and_exit(EXIT_FAILURE);
		}

		send_ack(pipe_fd, CHK_AOK, user, source_file);
	
	}

	// Restore stdout and stderr and exit 
	clean_and_exit(EXIT_SUCCESS);
}
