#ifndef MACROS_H
#define MACROS_H

#include <sys/time.h>

/* Shared macros */

#define DEBUG                   1
#define VERBOSE                 1
#define DELIM                   "*"
#define ARG_DELIM               ":"
#define MAX_FILENAME_LEN        128
#define MAX_COMMAND_LEN         512
#define MAX_PACKET_SIZE         512

/* Judge macros */

#define FILLER                  "*****"
#define TTY                     "/dev/tty"
#define MAX_ARGS                8
#define MAX_ENVS                8
#define MAX_FD                  32
#define MAX_TIME_ALLOWED        3
#define MAX_JUDGES              32

/* Coordinator macros */

#define MAX_INPUT_FILES         16
#define MAX_NUM_THREADS         16
#define MAX_DB_RESULT_LEN       1<<20
#define MILLI                   1000000
#define MIN_USER_LEN            3

/* Server macros */

#define PORT                    31337
#define HEADER_PREFIX           "FBEGIN"
#define ARG_DELIM               ":"
#define TCP_PACKET_SIZE         512
#define MAX_WAITING_CONNECTIONS 5

/* Internal filename suffixes */

#define LOGFILE_SUFFIX          "log.txt"
#define ERRORFILE_SUFFIX        "error.txt"
#define DIFF_SUFFIX             "diff.txt"
#define OUTFILE_PREFIX          "out"
#define SOLFILE_PREFIX          "solution"
#define SOLFILE_SUFFIX          ".c"
#define MAINFILE_SUFFIX         "main.c"

/* Internal directories */

#define BIN                     "bin"
#define MODULES                 "modules"
#define SANDBOX                 "sandbox"
#define JUDGE                   "judge"
#define TEMP                    "tmp"
#define SUB                     "submissions"

/* Command line argument options */

#define RUN_SERVER              "runserver"
#define MODULE_RESULTS          "module"
#define USER_RESULTS            "user"

/* Acknowledgment messages */
#define CMP_AOK   "CMP_AOK" /* Compilation succeeded            */
#define CMP_ERR   "CMP_ERR" /* Compilation failed         FATAL */
#define RUN_AOK   "RUN_AOK" /* A run succeeded                  */
#define RUN_ERR   "RUN_ERR" /* A run failed               FATAL */
#define TIM_OUT   "TIM_OUT" /* A run timed out            FATAL */
#define CHK_AOK   "CHK_AOK" /* A diff succeeded                 */
#define CHK_ERR   "CHK_ERR" /* A diff failed              FATAL */

#define JDG_ERR   "JDG_ERR" /* Solution not accepted      FATAL */
#define JDG_AOK   "JDG_AOK" /* Solution accepted          FATAL */

#define REQ_AOK   "REQ_AOK" /* Client request received          */
#define FIL_AOK   "FIL_AOK" /* File received OK                 */                            
#define HDR_AOK   "HDR_AOK" /* Header received OK               */  

#define INV_USR   "INV_USR" /* Invalid username           FATAL */
#define INV_MOD   "INV_MOD" /* Invalid module number      FATAL */
#define UNK_ERR   "UNK_ERR" /* Unknown error              FATAL */
#define BEG_FIL   "BEG_FIL" /* About to stream file             */
#define BEG_SOL   "BEG_SOL" /* About to stream solution         */

typedef struct {
  char id[64];                  /* Unique identifier for each judge       */
  int pid;                      /* Judge process' pid                     */
  char *source_path;            /* Source code filepath                   */
  char fd_w[MAX_FD];            /* C string containing pipe write-end fd  */
  char user[MAX_FILENAME_LEN];  /* Username of submitted file's owner     */
  char module_num[2 << 5];      /* Module number                          */
  struct timeval time_struct;   /* Time of creation of judge              */
  int num_input_files;          /* Number of input files                  */
  char **input_files;           /* Array of input file paths              */
  char *exec_args[2 << 5];      /* Command line arguments                 */
  char *exec_envs[2 << 5];      /* Environment variables                  */
  int socket_fd;                /* Client connection socket fd            */
} Judge;

typedef struct {
  char number[MAX_FILENAME_LEN];  /* Module number              */
  int num_input_files;            /* Number of input files      */
  char **input_files;             /* Array of input file paths  */
} Module;

#endif
