#ifndef MACROS_H
#define MACROS_H

#include <sys/time.h>

/* Shared macros */
#define DEBUG 1
#define FILLER "*****"
#define DELIM "*"
#define MAX_FILENAME_LEN 128
#define TTY "/dev/tty"
#define MAX_COMMAND_LEN 512
#define MAX_PACKET_SIZE 512

/* Judge macros */
#define MAX_ARGS 8
#define MAX_ENVS 8
#define FD_MAX 2 << 5
#define MAX_TIME_ALLOWED 3
#define MAX_JUDGES 32

#define LOGFILE_SUFFIX "log.txt"
#define ERRORFILE_SUFFIX "error.txt"
#define DIFF_SUFFIX "diff.txt"

#define OUTFILE_SUFFIX "out"
#define BIN "bin"
#define MODULES "modules"
#define SANDBOX "sandbox"
#define JUDGE "judge"
#define TEMP "tmp"
#define SUB "submissions"

/* Receiver macros */
#define MAX_NUM_THREADS 16
#define MILLI 1000000

/* Settings */
const char APP_ROOT[] = "/home/pedram/repos/os-grader/server/";
const char BIN_ROOT[] = "/home/pedram/repos/os-grader/server/bin/";

//TODO: remove null terminator
const char CMP_AOK[] = "CMP_AOK\0";
const char CMP_ERR[] = "CMP_ERR\0";
const char RUN_AOK[] = "RUN_AOK\0";
const char RUN_ERR[] = "RUN_ERR\0";
const char CHK_AOK[] = "CHK_AOK\0";
const char CHK_ERR[] = "CHK_ERR\0";
const char JDG_AOK[] = "JDG_AOK\0";

typedef struct {
  // Unique identifier for each judge
  char id[64];

  // Judge process' pid
  int pid;

  // Source code file path
  char *source_path;

  // C string containing pipe write-end fd
  char fd_w[FD_MAX];

  // Username of submitted file's owner
  char user[MAX_FILENAME_LEN];

  // Assignment number
  char ass_num[2 << 5];

  // To keep track of time of creation
  struct timeval time_struct;

  // Number of input files
  int num_input_files;

  // Path to the input files or NULL
  char **input_files;

  // Command line argument
  char *exec_args[2 << 5];

  // Environment variables
  char *exec_envs[2 << 5];
} Judge;

#endif
