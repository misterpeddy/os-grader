#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1
#define EXECUTOR "executor"

int main() {
	char filename[16];
	strcpy(&filename, "prog.c");
	char *exec_args[] = {EXECUTOR, &filename, NULL};
	char *exec_envs[] = {NULL};

	if (fork() == 0) {
		if (DEBUG) printf("Starting Execution\n");
		execv(EXECUTOR, exec_args);
		perror("||execve||");
	} else {
		wait(NULL);
	}
}
