#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <unistd.h>

#include "macros.h"

int fd[2];

long id_incrementor = 4;
pthread_mutex_t inc_lock;

Judge *active_judges[MAX_JUDGES];
pthread_mutex_t judge_lock;

int init_judge(Judge *judge)
{
	// Set the judge id
	pthread_mutex_lock(&inc_lock);
	sprintf(&judge->id, "%ld", id_incrementor++);
	pthread_mutex_unlock(&inc_lock);

	// Copy shared pipe descriptors, fd[0] for reading, fd[1] for writing
	sprintf(&judge->fd_w, "%d", fd[1]);

	// Capture source file path
	judge->source_path = (char *) malloc(strlen("prog.c") + 1);
	strcpy(judge->source_path, "prog.c");
	
	// Capture username
	strcpy(&judge->user, "pp5nv");
	
	// Set number of input files
	judge->num_input_files = 2;
	
	// Capture input file path
	judge->input_files = (char **) malloc(2);
	judge->input_files[0] = (char *) malloc(strlen("input1.txt") + 1);
	strcpy(judge->input_files[0], "input1.txt");
	judge->input_files[1] = (char *) malloc(strlen("input2.txt") + 1);
	strcpy(judge->input_files[1], "input2.txt");

	// Set command line arguments
	int k = 0;
	judge->exec_args[k++] = JUDGE;
	judge->exec_args[k++] = &judge->id;
	judge->exec_args[k++] = judge->source_path;
	judge->exec_args[k++] = &judge->user;
	judge->exec_args[k++] = "0";
	judge->exec_args[k++] = judge->fd_w;
	judge->exec_args[k++] = judge->input_files[0];
	judge->exec_args[k++] = judge->input_files[1];
	judge->exec_args[k++] = NULL;

	// Set environment variables
	judge->exec_envs[0] = NULL;

	// Set the time of creation
	gettimeofday(&judge->time_struct, NULL);

	// Add the judge to active_judges and return if errored
	if (add_judge(judge)) {
		// Reset the incrementor
		pthread_mutex_lock(&inc_lock);
		id_incrementor--;
		pthread_mutex_unlock(&inc_lock);

		if (DEBUG) printf("Could not add new judge\n");
		return 1;	
	}
	return 0;	
}

void destruct_judge(Judge *judge) 
{
	free(judge->source_path);
	for (int i=0; i<judge->num_input_files; i++) {
		free(judge->input_files[i]);
	}
	free(judge->input_files);
}

void listen_to_judges()
{
	while (1) {
		// Block until receive ack
		int nbytes, pipe_in = fd[0];
		char message[MAX_PACKET_SIZE];
		memset(&message, 0, MAX_PACKET_SIZE);
		nbytes = read(pipe_in, message, MAX_PACKET_SIZE);
		
		if (nbytes > 0) { 

			// Set up to parse message
			char *user, *judge_id, *ack_code, *token_state;
			char delimiter[2];
			delimiter[0] = DELIM; 

			// Parse the message
			user = strtok_r(message, delimiter, &token_state);
			judge_id = strtok_r(NULL, " *", &token_state);
			ack_code = strtok_r(NULL, " *", &token_state);
			
			if (DEBUG) printf("Received[%s][%s][%s] (%d bytes)\n", user, judge_id, ack_code, nbytes);

			// Act on the response
			if (!strcmp(ack_code, &JDG_AOK)) printf("DONE1\n");
			if (!strcmp(ack_code, &CHK_ERR)) printf("DONE2\n");
			if (!strcmp(ack_code, &RUN_ERR)) printf("DONE3\n");
			if (!strcmp(ack_code, &CMP_ERR)) printf("DONE4\n");


		} else {
			if (DEBUG) printf("Received Faulty message of length [%d]\n", nbytes);			
			usleep(100000);
		}
	}
}

void alarm_handler(int signal) {
	if (signal == SIGALRM) {
		if (DEBUG) printf("A judge process timed out - Exiting\n");
		//kill(child_pid, SIGKILL);
	}
}

int add_judge(Judge *judge) {
	int i=0;
	while (i<MAX_JUDGES) {
		if (!active_judges[i]) {
			active_judges[i] = judge;
			return 0;
		} 
		i++;
	}	
	return 1;
}

void handle_request() {
	Judge *judge = (Judge *)malloc(sizeof(Judge));

	if (init_judge(judge)) {
		exit(EXIT_FAILURE);
	}

	int pid = fork();
	if (pid == 0) {
		// Close reading end since executor will not read from the pipe
		close(fd[0]);
				
		if (DEBUG) printf("Starting Judge\n");
		execv(JUDGE, judge->exec_args);
		perror("||execve||");
		exit(EXIT_FAILURE);
	} else {
		judge->pid = pid;

		if (signal(SIGALRM, alarm_handler) == SIG_ERR && DEBUG)
			printf("Could not register alarm listener\n");	

		alarm(MAX_TIME_ALLOWED);
	}

	//destruct_judge(judge);
}

int main(int argc, char **argv) {
	// Read settings file and initialize assignments DS

	// Set up shared pipe
	pipe(fd);
	
	// Spawn a thread to listen on judges pipe
	pthread_t pipe_listener_thread;
	pthread_create(&pipe_listener_thread, NULL, &listen_to_judges, NULL);

	// Set up the mutex locks 
	if (pthread_mutex_init(&inc_lock, NULL) || 
		pthread_mutex_init(&judge_lock, NULL)) {
		if (DEBUG) printf("Could not initiazlie mutex locks - Exiting\n");
		exit(EXIT_FAILURE);
	}
	
	// Set up the listener to listen for incoming requests	

	// while something
	// if something happens
	handle_request();

	while(1);
	//end while
}
