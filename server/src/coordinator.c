#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

#include "macros.h"
#include "server.h"
#include "settings.h"

int fd[2];

long id_incrementor = 4;
pthread_mutex_t inc_lock;

Judge *active_judges[MAX_JUDGES];
pthread_mutex_t judge_lock;

/************************ Judge Routines ****************************/

/*
** Initializes the judge struct and adds it to active_judges
*/
int init_judge(Judge *judge, Request *request) {
  // Set the judge id
  pthread_mutex_lock(&inc_lock);
  sprintf(&judge->id, "%ld", id_incrementor++);
  pthread_mutex_unlock(&inc_lock);

  // Copy shared pipe descriptors, fd[0] for reading, fd[1] for writing
  sprintf(&judge->fd_w, "%d", fd[1]);

  // Capture source file path
  judge->source_path = (char *)malloc(strlen(request->filename) + 1);
  strcpy(judge->source_path, request->filename);

  // Capture username
  strcpy(&judge->user, request->user);

  // Capture assignment number
  strcpy(&judge->ass_num, request->ass_num);

  // TODO: get this from the assignments DS
  // Set number of input files
  judge->num_input_files = 2;

  // Capture input file path
  judge->input_files = (char **)malloc(2);
  judge->input_files[0] = (char *)malloc(strlen("input1.txt") + 1);
  strcpy(judge->input_files[0], "input1.txt");
  judge->input_files[1] = (char *)malloc(strlen("input2.txt") + 1);
  strcpy(judge->input_files[1], "input2.txt");

  // Set command line arguments
  int k = 0;
  judge->exec_args[k++] = JUDGE;
  judge->exec_args[k++] = &judge->id;
  judge->exec_args[k++] = judge->source_path;
  judge->exec_args[k++] = &judge->user;
  judge->exec_args[k++] = &judge->ass_num;
  judge->exec_args[k++] = judge->fd_w;
  judge->exec_args[k++] = judge->input_files[0];
  judge->exec_args[k++] = judge->input_files[1];
  judge->exec_args[k++] = NULL;

  // Set environment variables
  judge->exec_envs[0] = NULL;

  // Set the time of creation
  gettimeofday(&judge->time_struct, NULL);

  // Set the socket fd
  judge->socket_fd = request->socket_fd;

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

/*
** Adds judge to the first empty spot in active_judges
** If no empty spots, returns 1
*/
int add_judge(Judge *judge) {
  int i = 0;
  while (i < MAX_JUDGES) {
    if (!active_judges[i]) {
      active_judges[i] = judge;
      return 0;
    }
    i++;
  }
  return 1;
}

/*
** Finds judge with id_str judge id from active_judges
*/
Judge *get_judge(char *id_str) {
  for (int i = 0; i < MAX_JUDGES; i++) {
    if (active_judges[i] && !strcmp(active_judges[i]->id, id_str))
      return active_judges[i];
  }
  return NULL;
}

/*
** Frees all memory associated with a Judge struct.
** Also removes the entry from the active_judges list
*/
void destruct_judge(Judge *judge) {
  // Free all input file paths
  for (int i = 0; i < judge->num_input_files; i++) {
    free(judge->input_files[i]);
  }

  // Free input file array
  free(judge->input_files);

  // Free source path
  free(judge->source_path);

  // Remove from active judges
  for (int i = 0; i < MAX_JUDGES; i++) {
    if (active_judges[i] && !strcmp(active_judges[i]->id, judge->id))
      active_judges[i] = NULL;
  }
}

/************************* I/O Routines*****************************/

/*
** Blocking call to sit and listen on the read-end of the judges shared pipe.
** When receives a fatal ack {CMP_ERR, RUN_ERR, CHK_ERR} or judge complete
** {JDG_AOK} it destructs that judge and records its run result.
** TODO: Actually record the result
*/
void listen_to_judges() {
  while (1) {
    // Block until receive an ack to process
    int nbytes, pipe_in = fd[0], bytes_read = 0;
    char *message = (char *)malloc(MAX_PACKET_SIZE);
    memset(message, 0, MAX_PACKET_SIZE);
    nbytes = read(pipe_in, message, MAX_PACKET_SIZE);

    while (nbytes > bytes_read) {
      // Set up to parse message
      char *size, *judge_id, *ack_code, *token_state;

      // Parse the message
      size = strtok_r(message, DELIM, &token_state);
      judge_id = strtok_r(NULL, DELIM, &token_state);
      ack_code = strtok_r(NULL, DELIM, &token_state);

      bytes_read += atoi(size);
      message = message + bytes_read;

      if (DEBUG)
        printf("Received %d bytes from judge (%s): %s \n", nbytes, judge_id, ack_code);

      // Find out which judge sent the response
      Judge *judge = get_judge(judge_id);

      // Echo ack to the client
      send_message(judge->socket_fd, ack_code);

      // Act on fatal codes 
      if (!strcmp(ack_code, &JDG_AOK) || !strcmp(ack_code, &CMP_ERR) ||
          !strcmp(ack_code, &RUN_ERR) || !strcmp(ack_code, &CHK_ERR)) {

        // If an error code, echo error file to client
        if (!strcmp(ack_code, &CMP_ERR) || !strcmp(ack_code, &RUN_ERR)) {

          // Compute error file path
          char log_file[MAX_FILENAME_LEN];
          sprintf(&log_file, "%s/%s/%s/%s_%s_%s", SUB, judge->user, judge->ass_num, judge->user, 
              judge->ass_num, ERRORFILE_SUFFIX);

          // TODO: prune the output that is sent over 
          send_file(judge->socket_fd, log_file);
        }
        

        if (judge) {
          // TODO: Record the result in database
          if (DEBUG)
            printf("Judge with pid (%d) for user (%s) is Done\n", judge->pid,
                   &judge->user);

          close_connection(judge->socket_fd);
          destruct_judge(judge);
          free(judge);
        }
      }
    }
  }
  // TODO: free(message) in final destructor
}

/**************************** Handlers *******************************/

/*
** Handler for signals of type SIGLARM
** Checks to see if any judges have max'd out their time limit - if yes destroys
*them.
*/
void alarm_handler(int signal) {
  if (signal != SIGALRM) {
    return;
  }

  // Get current time
  struct timeval start, end;
  int lifetime;
  gettimeofday(&end, NULL);

  // Find processes that have max'd out their time limitation
  for (int i = 0; i < MAX_JUDGES; i++) {
    if (active_judges[i]) {
      start = active_judges[i]->time_struct;
      lifetime = (end.tv_sec * MILLI + end.tv_usec) -
                 (start.tv_sec * MILLI + start.tv_usec);

      // Check if process has max'd out
      if (lifetime > MAX_TIME_ALLOWED * MILLI) {
        if (DEBUG)
          printf("Judge process (%d) for user (%s) timed out - Exiting\n",
                 active_judges[i]->pid, &active_judges[i]->user);

        // Echo ack to the client
        send_message(active_judges[i]->socket_fd, TIM_OUT);

        // Do clean up for the judge
        int judge_pid = active_judges[i]->pid;
        close_connection(active_judges[i]->socket_fd);
        destruct_judge(active_judges[i]);
        free(active_judges[i]);

        // Kill the judge process
        // TODO: wait on it so it doesn't become a zombie
        kill(judge_pid, SIGKILL);
      }
    }
  }
}

/*
** Handler for incoming requests
** Creates a Judge struct with relevant info and forks off the process.
*/
void handle_request(Request *request) {
  Judge *judge = (Judge *)malloc(sizeof(Judge));

  if (init_judge(judge, request)) {
    exit(EXIT_FAILURE);
  }

  int pid = fork();
  if (pid == 0) {
    // Close reading end since executor will not read from the pipe
    close(fd[0]);

    // Locate judge binary
    char judge_bin[MAX_FILENAME_LEN];
    memset(&judge_bin, 0, MAX_FILENAME_LEN);
    strcpy(&judge_bin, BIN_ROOT);
    strcpy(&judge_bin[strlen(BIN_ROOT)], JUDGE);

    // Run the judge binary
    if (DEBUG) printf("Starting Judge\n", judge_bin);
    execv(judge_bin, judge->exec_args);
    perror("exec failure");
    exit(EXIT_FAILURE);
  } else {
    judge->pid = pid;

    alarm(MAX_TIME_ALLOWED);
  }
}

void init_request(Request *request) {
  request->user = (char *)malloc(MAX_FILENAME_LEN);
  request->ass_num = (char *)malloc(MAX_FILENAME_LEN);
  request->filename = (char *)malloc(MAX_FILENAME_LEN);
}

/******************************* Main ***********************************/

/*
** Starts listening for new connections, and handles incoming requests.
*/
int main(int argc, char **argv) {
  // Change current working directory to root of the application
  chdir(APP_ROOT);

  // Set up shared pipe
  pipe(fd);

  // Set up alarm listener
  if (signal(SIGALRM, alarm_handler) == SIG_ERR && DEBUG)
    printf("Could not register alarm listener\n");

  // Spawn a thread to listen on judges pipe
  pthread_t pipe_listener_thread;
  pthread_create(&pipe_listener_thread, NULL, (void *)&listen_to_judges, NULL);
  
  // Set up the mutex locks
  if (pthread_mutex_init(&inc_lock, NULL) ||
      pthread_mutex_init(&judge_lock, NULL)) {
    if (DEBUG) printf("Could not initiazlie mutex locks - Exiting\n");
    exit(EXIT_FAILURE);
  }

  // Set up initial socket for listen queue
  int listen_queue_socket = set_up_server();

  // Init and allocate enough space to hold the raw request
  Request request;
  init_request(&request);
  while (1) {
    
    // Block and listen for connections - upon receipt, move connection to new socket
    int connection_socket = listen_for_requests(listen_queue_socket);

    request.socket_fd = connection_socket;

    // TODO: Move this to a new thread to handle concurrent requests
    // Let the handler receive request data
    receive_request(&request);

    handle_request(&request);
  }


}
