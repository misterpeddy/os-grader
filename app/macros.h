#ifndef MACROS_H
#define MACROS_H

/* Shared macros */
#define DEBUG 1 
#define FILLER "*****"
#define DELIM 0x2A
#define MAX_FILENAME_LEN 128
#define TTY "/dev/tty"
#define MAX_COMMAND_LEN 512
#define MAX_PACKET_SIZE 512

/* Judge macros */
#define MAX_ARGS 8
#define MAX_ENVS 8
#define FD_MAX 2<<5

#define LOGFILE_SUFFIX "log.txt"
#define ERRORFILE_SUFFIX "error.txt"
#define DIFF_SUFFIX "diff.txt"

#define OUTFILE_SUFFIX "out"
#define BIN "bin"
#define MASTER_DIR "master"
#define SANDBOX "sandbox"
#define EXECUTOR "executor"

const char COMP_AOK[] = "COMP_AOK\0";
const char COMP_ERR[] = "COMP_ERR\0";
const char RUN_AOK[] = "RUN_AOK\0";
const char RUN_ERR[] = "RUN_ERR\0";
const char CHK_AOK[] = "CHK_AOK\0";
const char CHK_ERR[] = "CHK_ERR\0";

typedef struct {
	// Shared pipe file descriptors
	int fd[2];
	
	// Source code file path
	char *source_path;

	// C string containing pipe write-end fd 
	char fd_w[FD_MAX];

	// Username of submitted file's owner
	char user[MAX_FILENAME_LEN];

	// Path to the input files or NULL
	char **input_files;
} Judge;

#endif
