#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "macros.h"

#define DEBUG 1

int init_judge(int argc, char **argv, Judge *judge)
{

	// Create shared pipe, fd[0] for reading, fd[1] for writing
	pipe(judge->fd);
	sprintf(&judge->fd_w, "%d", judge->fd[1]);

	// Capture source file path
	judge->source_path = (char *) malloc(strlen("prog.c") + 1);
	strcpy(judge->source_path, "prog.c");
	
	// Capture username
	strcpy(&judge->user, "pp5nv");
	
	// Capture input file path
	judge->input_files = (char **) malloc(2);
	judge->input_files[0] = (char *) malloc(strlen("input1.txt") + 1);
	strcpy(judge->input_files[0], "input1.txt");
	judge->input_files[1] = (char *) malloc(strlen("input2.txt") + 1);
	strcpy(judge->input_files[1], "input2.txt");

	return 0;	
}

void destruct_judge(Judge *judge) 
{
	free(judge->source_path);
	free(judge->input_files[0]);
	free(judge->input_files[1]);
	free(judge->input_files);
}

int wait_for_comp_ack(Judge *judge)
{
	// Block until receive ack
	int nbytes, pipe_in = judge->fd[0];
	char message[MAX_PACKET_SIZE];
	nbytes = read(pipe_in, message, MAX_PACKET_SIZE);
	
	// Error if empty or bad message
	if (nbytes <= 0) return 1;

	// Set up to parse message
	char *user, *source_path, *ack_code, *token_state;
	char delimiter[2];
	delimiter[0] = DELIM; 

	// Parse the message
	user = strtok_r(message, delimiter, &token_state);
	source_path = strtok_r(NULL, " *", &token_state);
	ack_code = strtok_r(NULL, " *", &token_state);
	
	printf("Received[%s][%s][%s]\n", user, source_path, ack_code);
	return 0;
}

int main(int argc, char **argv) 
{
	Judge judge;

	if (init_judge(argc, argv, &judge)) 
	{
		exit(EXIT_FAILURE);
	}
	if (fork() == 0) 
	{
		// Close reading end since executor will not read from the pipe
		close(judge.fd[0]);

		// Set arguments and environment variables
		char *exec_args[6];
		exec_args[0] = EXECUTOR;
		exec_args[1] = judge.source_path;
		exec_args[2] = &judge.user;
		exec_args[3] = judge.fd_w;
		exec_args[4] = judge.input_files[0];
		exec_args[5] = judge.input_files[1];
		exec_args[6] = NULL;

		char *exec_envs[] = {NULL};

		if (DEBUG) printf("Starting Execution\n");
		execv(EXECUTOR, &exec_args);
		perror("||execve||");
	} else 
	{
		// Close writing end since receiver will not write to the pipe
		close(judge.fd[1]);	

		while (!wait_for_comp_ack(&judge));

		// Wait on child
		wait(NULL);

		if (DEBUG) printf("Finished Execution\n");
	}

	destruct_judge(&judge);
}
