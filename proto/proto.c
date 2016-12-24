#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1
#define EXECUTOR "executor"

int main() 
{
	int fd[2];
	char filename[16];
	char fd_w[32];
	char user[8];

	// Create shared pipe, fd[0] for reading, fd[1] for writing
	pipe(fd);
	sprintf(&fd_w, "%d", fd[1]);
	
	// Capture filename from argument 
	strcpy(&filename, "prog.c");
	
	// Capture username from argument
	strcpy(&user, "pp5nv");

	// Set arguments and environment variables
	char *exec_args[] = {EXECUTOR, &filename, user, fd_w, NULL};
	char *exec_envs[] = {NULL};

	if (fork() == 0) 
	{
		// Close reading end since executor will not read from the pipe
		close(fd[0]);

		if (DEBUG) printf("Starting Execution\n");
		execv(EXECUTOR, exec_args);
		perror("||execve||");
	} else 
	{
		// Close writing end since receiver will not write to the pipe
		close(fd[1]);	

		int nbytes;
		char buf[32];
		nbytes = read(fd[0], buf, sizeof(buf));
		printf("Received [%s]\n", buf);		

		wait(NULL);
		if (DEBUG) printf("Finished Execution\n");
	}
}
