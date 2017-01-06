#ifndef MACROS_H
#define MACROS_H

#include <sys/time.h>

/* Shared macros */
#define DEBUG 1
#define DELIM "*"
#define MAX_FILENAME_LEN 128
#define MAX_COMMAND_LEN 512
#define MAX_PACKET_SIZE 512

/* Judge macros */
#define FILLER "*****"
#define TTY "/dev/tty"
#define MAX_ARGS 8
#define MAX_ENVS 8
#define FD_MAX 2 << 5
#define MAX_TIME_ALLOWED 3
#define MAX_JUDGES 32

/* Coordinator macros */
#define MAX_NUM_THREADS 16
#define MILLI 1000000

/* Server macros */
#define PORT 31337
#define HEADER_PREFIX "FBEGIN"
#define ARG_DELIM ":"
#define TCP_PACKET_SIZE 4096
#define MAX_WAITING_CONNECTIONS 5

/* Internal filename suffixes */
#define LOGFILE_SUFFIX "log.txt"
#define ERRORFILE_SUFFIX "error.txt"
#define DIFF_SUFFIX "diff.txt"
#define OUTFILE_SUFFIX "out"

/* Internal directories */
#define BIN "bin"
#define MODULES "modules"
#define SANDBOX "sandbox"
#define JUDGE "judge"
#define TEMP "tmp"
#define SUB "submissions"

/* Settings */
#define MAX_INPUT_FILES 16

const char APP_ROOT[] = "/home/pedram/repos/os-grader/server/";
const char BIN_ROOT[] = "/home/pedram/repos/os-grader/server/bin/";
const char MODULES_ROOT[] = "/home/pedram/repos/os-grader/server/modules/";


/* Acknowledgment messages */
const char CMP_AOK[] = "CMP_AOK"; /* Compilation succeeded          */
const char CMP_ERR[] = "CMP_ERR"; /* Compilation failed       FATAL */
const char RUN_AOK[] = "RUN_AOK"; /* A run succeeded                */
const char RUN_ERR[] = "RUN_ERR"; /* A run failed             FATAL */
const char TIM_OUT[] = "TIM_OUT"; /* A run timed out          FATAL */
const char CHK_AOK[] = "CHK_AOK"; /* A diff succeeded               */
const char CHK_ERR[] = "CHK_ERR"; /* A diff failed            FATAL */

const char JDG_ERR[] = "JDG_ERR"; /* Solution not accepted          */
const char JDG_AOK[] = "JDG_AOK"; /* Solution accepted              */

const char RCV_AOK[] = "RCV_AOK"; /* Client request received        */


typedef struct {
  char id[64];                  /* Unique identifier for each judge */
  int pid;                      /* Judge process' pid */
  char *source_path;            /* Source code filepath */
  char fd_w[FD_MAX];            /* C string containing pipe write-end fd */
  char user[MAX_FILENAME_LEN];  /* Username of submitted file's owner */
  char ass_num[2 << 5];         /* Assignment number                     */
  struct timeval time_struct;   /* Time of creation of judge             */
  int num_input_files;          /* Number of input files                  */
  char **input_files;           /* Array of input file paths              */
  char *exec_args[2 << 5];      /* Command line arguments                 */
  char *exec_envs[2 << 5];      /* Environment variables                  */
  int socket_fd;                /* Client connection socket fd            */
} Judge;

typedef struct {
  char number[MAX_FILENAME_LEN];  /* Assignment number          */
  int num_input_files;            /* Number of input files      */
  char **input_files;             /* Array of input file paths  */
} Assignment;

#endif
