#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "macros.h"

char log_file[MAX_FILENAME_LEN];
char err_file[MAX_FILENAME_LEN];
char out_file[MAX_FILENAME_LEN];
char diff_file[MAX_FILENAME_LEN];

/******************************* Helpers ***********************************/

/*
** Writes acknowledgement (<message
*size><delimiter><judge_id><delimiter><ack_code><delimiter>)
** onto the pipe.
*/
void send_ack(int pipe_fd, const char *ack_str, char *judge_id) {
  // Compute size of the second and third bits + delimiters
  int size = strlen(ack_str) + strlen(judge_id) + 3 /* delimiters */;

  // Add the length of the size itself
  int size_len = 1;
  if (size > 8) size_len = 2;
  size += size_len;

  // Allocate space for the message
  char *buf = (char *)malloc(size);

  // Concat the message length, judge_id and acknowledgement string
  sprintf(buf, "%d", size);
  strcpy(buf + size_len + 1, judge_id);
  strcpy(buf + size_len + strlen(judge_id) + 2, ack_str);

  // Change null terminators to delimiters
  buf[size_len] = *DELIM;
  buf[size_len + 1 + strlen(judge_id)] = *DELIM;
  buf[size - 1] = *DELIM;

  // Write the message to the pipe
  write(pipe_fd, buf, strlen(buf));

  // Deallocate message
  free(buf);
}

/*
** Captures command using format string and arguments.
** Executes using system()
*/
int execute_cmd(const char * format, ...) {
  char command[MAX_COMMAND_LEN];

	// Capture arguments and write command string
  va_list ap; 
  va_start(ap, format);
  vsprintf(command, format, ap);

	// Execute the command and return result
  int ret = system(command);
  va_end(ap);

	// Log output
	printf("$ %s\n", command);

	// Log error
	if (ret) {
		printf("Warning - command (%s) caused an error code (%d)\n", command, ret);
		perror(command);
	}

  return ret;
}

/************************ Judge Routines *****************************/

/*
** Creates sandbox user/module_num and chdir's into it
** Copies all binaries, libraries and files needed to chroot
** Reroutes the stdout and stderr of the process to log_file and err_file
*/
void init_sandbox(char *user, char *module_num, char *filename) {

  // Create SANDBOX directory if it doesn't exist and chdir into it
  if (access(SANDBOX, F_OK) == -1) system("mkdir "SANDBOX);
  chdir(SANDBOX);

  // Delay logging commands until stdout/stderr redirect is done
	// Create user directory if does not exist already
  char command1[MAX_COMMAND_LEN];
  sprintf(command1, "mkdir %s", user);
  if (access(user, F_OK) == -1) system(command1);

	// Compute path for this submission's sandbox
	char work_dir[MAX_FILENAME_LEN];
	sprintf(work_dir, "%s/%s", user, module_num);

  // Remove preexisting sandbox for this module
  char command2[MAX_COMMAND_LEN];
  sprintf(command2, "rm -rf %s", work_dir);
  system(command2);

  // Create sandbox for this module and chdir into it
  char command3[MAX_COMMAND_LEN];
  sprintf(command3, "mkdir %s", work_dir);
  system(command3);
	chdir(work_dir);

  // Log stdout
  sprintf(log_file, "%s_%s_%s", user, module_num, LOGFILE_SUFFIX);
  freopen(log_file, "w", stdout);

  // Log stderr
  sprintf(err_file, "%s_%s_%s", user, module_num, ERRORFILE_SUFFIX);
  freopen(err_file, "w", stderr);

  printf("%sRedirecting stdout output%s\n", FILLER, FILLER);
  printf("$(%s)\n$(%s)\n$(%s)\n", command1, command2, command3);

  // Copy submitted sourcefile
  // TODO: Change to absolute path
  char command[MAX_COMMAND_LEN];
  memset(command, 0, MAX_COMMAND_LEN);
  sprintf(command, "cp ../../../%s/%s %s", TEMP, filename, MAINFILE_SUFFIX);
  system(command);
  printf("$(%s)\n", command);

	// Copy input/output files
	// TODO: Prevent the user process from reading/writing to these files
	execute_cmd("cp -rf ../../../%s/%s %s", MODULES, module_num, FILES);

  // Copy essential binaries and libraries
	execute_cmd("mkdir lib");
  execute_cmd("mkdir lib64");
  execute_cmd("mkdir bin");
  execute_cmd("cp -a /lib/x86_64-linux-gnu lib/");
  execute_cmd("cp /lib64/* lib64/");
  execute_cmd("cp /bin/bash bin/bash");
  execute_cmd("cp /usr/bin/diff bin/diff");
  execute_cmd("cp /usr/bin/gcc bin/gcc");
  execute_cmd("cp /bin/busybox bin/");
  execute_cmd("ln -s /bin/busybox bin/sh");
}

/*
** Compiles C file (MAINFILE_SUFFIX) for user for module module_num
** Binary filepath: <sandbox>/binary
** Returns 0 if compiled with no errors otherwise positive number
*/
int compile_source(char *user, char *module_num) {
  printf("\n%sCompiling module %s [%s] for %s%s\n", FILLER, module_num, 
      MAINFILE_SUFFIX, user, FILLER);fflush(stdout);

  // Compile source
	execute_cmd("gcc -w %s -o %s", MAINFILE_SUFFIX, BIN);

  printf("%sFinished compiling module %s [%s] for %s%s\n\n", FILLER, 
      module_num, MAINFILE_SUFFIX, user, FILLER);fflush(stdout);

  // Read error log file stats
  struct stat log_stat;
  stat(err_file, &log_stat);

  return log_stat.st_size;
}

/*
** Runs the program <sandbox>/binary with input <sandbox>/FILES/input_file
*/
int run_program(char *user, char *module_num, char *input_file) {
  printf("\n%sRunning %s/%s/%s%s\n", FILLER, user, module_num, BIN, FILLER);

  // Redirect output to OUT
  sprintf(out_file, "%s_%s_%s_%s", user, module_num, OUTFILE_PREFIX, input_file);
  freopen(out_file, "w", stdout);
  
  // Remove root privilege
  seteuid(NON_PRIV_USR);
  setegid(NON_PRIV_USR);

  // Run the binary
  char command[MAX_COMMAND_LEN];
	memset(command, 0, MAX_COMMAND_LEN);
  if (input_file) sprintf(command, "./%s < %s/%s", BIN, FILES, input_file);
  else sprintf(command, "./%s", BIN);
  int ret = system(command);
  
  // Restore root privilege
  seteuid(0);
  setegid(0);

  // Redirect output to logfile
  freopen(log_file, "a", stdout);

  // Log command and result
  printf("$ %s\n", command);
  printf("%sFinished running %s/%s/%s%s\n\n", FILLER, user, module_num, BIN, 
      FILLER);

  // Read error log file stats
  struct stat log_stat;
  stat(err_file, &log_stat);
  return log_stat.st_size;
}

/*
** Compares the following 2 files:
** <sandbox>/<user>_<module_num>_out_<input_file>
** <sandbox>/<module_num>/out_<input_file>
*/
int judge(char *user, char *module_num, char *input_file) {
  if (DEBUG)
    printf("\n%sJudging module %s with input %s for %s%s\n", FILLER,
           module_num, input_file, user, FILLER); fflush(stdout);

  // Set up file to send diff to
  sprintf(&diff_file, "%s_%s_%s", user, module_num, DIFF_SUFFIX);

  // Set up master file path
  char master_file[MAX_FILENAME_LEN];
  sprintf(&master_file, "%s/%s_%s", FILES, OUTFILE_PREFIX, input_file);

  // Execute the diff and write to the diff file
  int return_code = execute_cmd("diff %s %s > %s", out_file, master_file, diff_file);

  if (DEBUG)
    printf("%sFinished judging module %s with input %s for %s%s\n\n",
           FILLER, module_num, input_file, user, FILLER);

  // Read error log file stats
  struct stat log_stat;
  stat(diff_file, &log_stat);

  return log_stat.st_size + return_code;
}

void clean_and_exit(int code) {
  // Delete lib lib64 bin
  execute_cmd("rm -rf lib lib64 bin");

  if (DEBUG) printf("%sFinishing stdout redirect%s\n\n", FILLER, FILLER);

  // Reroute stdout and stderr to TTY
  fflush(stdout);
  fflush(stderr);
  freopen(TTY, "a", stderr);
  freopen(TTY, "a", stdout);

  // Exit with the supplied exit code
  exit(code);
}

/******************************* Main ***********************************/

/*
** Captures and logs arguments, creates sandbox, compiles the source code and
** runs the code. It also sends ack messages on the shared pipe after 
** compilation, each run, each diff and the end of the process.
*/
int main(int argc, char **argv) {
  // Check arguments
  if (argc < 5) {
    printf(
        "USAGE: %s <judge id> <source file> <username> <module number> <pipe fd> [<input files>...]\n",
        argv[0]);
    exit(EXIT_FAILURE);
  }

  // Capture arguments
  int k = 1;
  char *judge_id = argv[k++];
  char *source_file = argv[k++];
  char *user = argv[k++];
  char *module_num = argv[k++];
  int pipe_fd = atoi(argv[k++]);
  int num_input_files = argc - k;
  char **input_files = &argv[k];

  // Create sandbox
  init_sandbox(user, module_num, source_file);

  // Log arguments
  int i;
  if (DEBUG) {
    printf("%sArgs: ", FILLER);
    for (i = 0; i < argc; i++) printf("<%s>", argv[i]);
    printf("%s\n", FILLER);
  }

  // Compile submitted source code
  int comp_result = compile_source(user, module_num);

  // Write compilation result code to pipe, exit if errored
  if (comp_result) {
    if (DEBUG) printf("%sCompilation failed - Exiting%s\n\n", FILLER, FILLER);
    send_ack(pipe_fd, CMP_ERR, judge_id);
    clean_and_exit(EXIT_FAILURE);
  } else {
    send_ack(pipe_fd, CMP_AOK, judge_id);
  }

	// Change root
  if (chroot(".")) {
		perror("Chroot failed");
		send_ack(pipe_fd, RUN_ERR, judge_id);
		clean_and_exit(EXIT_FAILURE);
	}

  for (i = 0; i < num_input_files; i++) {
    // Run and exit if errored
    if (run_program(user, module_num, input_files[i])) {
      if (DEBUG) printf("%sRun #%d failed - Exiting%s\n\n", FILLER, i, FILLER);
      send_ack(pipe_fd, RUN_ERR, judge_id);
      clean_and_exit(EXIT_FAILURE);
    }

    send_ack(pipe_fd, RUN_AOK, judge_id);

    // Judge the output against master
    if (judge(user, module_num, input_files[i])) {
      if (DEBUG)
        printf("%sFailed test case #%d - Exiting%s\n\n", FILLER, i, FILLER);
      send_ack(pipe_fd, CHK_ERR, judge_id);
      clean_and_exit(EXIT_FAILURE);
    }

    send_ack(pipe_fd, CHK_AOK, judge_id);
  }

  // Send judge over ack
  send_ack(pipe_fd, JDG_AOK, judge_id);

  // Restore stdout and stderr and exit
  clean_and_exit(EXIT_SUCCESS);
}

