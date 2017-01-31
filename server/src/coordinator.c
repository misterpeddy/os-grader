#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

#include "macros.h"
#include "server.h"
#include "settings.h"
#include "db.h"

int fd[2];
int listen_queue_socket;
sqlite3 *db;

Module *modules[NUM_REGISTERED_MODULES];

long id_incrementor = 0;
pthread_mutex_t inc_lock;

Judge *active_judges[MAX_JUDGES];
pthread_mutex_t judge_lock;

/************************ Module Helpers ****************************/

/*
** Returns positive int if filename is a valid input filename. 
** Valid input filename:  not "." or ".." and does not have the
** OUTFILE_PREFIX or SOLFILE_PREFIX. Returns 0 otherwise.
*/
char is_input_file (char *filename) {
  // Check against "." and ".."
  if (!strcmp(filename, ".") || !strcmp(filename, "..")) return 0;
  
  // Check against OUTFILE prefix
  int i;
  if (strlen(filename) > strlen(OUTFILE_PREFIX)) {
    for (i=0; i<strlen(OUTFILE_PREFIX); i++){
      if (filename[i] != OUTFILE_PREFIX[i])  break;
    } 
    if (i == strlen(OUTFILE_PREFIX)) return 0;
  }

  // Check against SOLFILE prefix
  if (strlen(filename) > strlen(SOLFILE_PREFIX)) {
    for (i=0; i<strlen(SOLFILE_PREFIX); i++){
      if (filename[i] != SOLFILE_PREFIX[i])  break;
    } 
    if (i == strlen(SOLFILE_PREFIX)) return 0;
  }

  return 1;
}

/*
** Returns 1 if module_name is a registered module, 0 otherwise.
*/
char is_registered(char *module_name) {
  int i;
  for (i=0; i<NUM_REGISTERED_MODULES; i++) {
    if (!strcmp(module_name, REGISTERED_MODULES[i])) return 1;
  }
  return 0;
}

/************************ Module Routines ****************************/

/*
** Given an empty module, fills it with the content of dirent provided.
** Specifically, sets the module number, num_input_files and input_files array.
*/
void init_module(Module *module, struct dirent *module_entry) {

  // Compute path of the module, set module number
  char module_path[MAX_FILENAME_LEN];
  sprintf(module_path, "%s%s", MODULES_ROOT, module_entry->d_name);
  strcpy(&module->number, module_entry->d_name);

  // Open module directory 
  DIR *module_dir = opendir(module_path);

  // Count number of input files
  int num_files=0;
  struct dirent *ent;
  while ((ent = readdir(module_dir)) != NULL) {
    if (is_input_file(ent->d_name)) {
      num_files++; 
    }
  }
  
  // Set number of input files, return if none found
  module->num_input_files = num_files;
  if (!num_files) return;
  
  // Allocate space for the input file paths
  module->input_files = (char **) malloc(num_files * sizeof(char *));
  memset(module->input_files, 0, num_files*sizeof(char *));
  
  // Reset the directory cursor
  closedir(module_dir);
  module_dir = opendir(module_path);

  // Set path for each input file in module struct
  int file_counter=0;
  while ((ent = readdir(module_dir)) != NULL) {
    if (is_input_file(ent->d_name)) {
      module->input_files[file_counter] = (char *)malloc(MAX_FILENAME_LEN);
      memset(module->input_files[file_counter], 0, MAX_FILENAME_LEN);
      strcpy(module->input_files[file_counter], ent->d_name);
      file_counter++; 
    }
  }

  closedir(module_dir);
}

/*
** Initializes the modules array by calling init_module for each registered
** module.
*/
int init_modules() { 
  DIR *dir = opendir(MODULES_ROOT);
  struct dirent *ent;
  int mod_count=0;

  // Allocate space for and call init_module for each registered 
  // module in modules directory
  while ((ent = readdir(dir)) != NULL) {
    if (strcmp(ent->d_name, ".") && 
      strcmp(ent->d_name, "..") &&
      is_registered(ent->d_name)) {
        modules[mod_count] = (Module *)malloc(sizeof(Module));
        init_module(modules[mod_count++], ent);
    }
  }

  closedir(dir);

  if (VERBOSE) print_modules();
}

/*
** Destructs a module struct.
** TODO: Call it in fatal signal handler
*/
destruct_module(Module *module) {
  int i=0;
  if(!module->num_input_files) return;
  for (; i<module->num_input_files; i++) {   
    free(module->input_files[i]);
  }
  free(module->input_files);
  free(module);
}

/*
** Destructs the modules array by calling destuct_module.
** TODO: Call it in fatal signal handler
*/
destruct_modules() {
  int i=0, j=0;
  for (; i<NUM_REGISTERED_MODULES; i++) {
    if (modules[i]) destruct_module(modules[i]);
  }
}

Module *find_module(char *module_num) { 
  int i;
  for (i=0; i<NUM_REGISTERED_MODULES; i++) {
    if (!strcmp(modules[i]->number, module_num)) return modules[i];
  }
  return NULL;
}

void print_modules() {
  int i, j;
  for (i=0; i<NUM_REGISTERED_MODULES; i++) {
    printf("%d - Module [%s] - {", i, modules[i]->number);
      for (j=0; j<modules[i]->num_input_files; j++) {
        printf ("<%s>", modules[i]->input_files[j]);
      }
    printf("}\n");
  }
}

/************************ Judge Routines ****************************/

/*
** Initializes the judge struct and adds it to active_judges
*/
int init_judge(Judge *judge, Request *request) {
  // Set the judge id
  pthread_mutex_lock(&inc_lock);
  sprintf(&judge->id, "%ld", id_incrementor++);
  pthread_mutex_unlock(&inc_lock);

  // Set terminated bit
  judge->terminated = 0;

  // Copy shared pipe descriptors, fd[0] for reading, fd[1] for writing
  sprintf(&judge->fd_w, "%d", fd[1]);

  // Capture source file path
  judge->source_path = (char *)malloc(strlen(request->filename) + 1);
  strcpy(judge->source_path, request->filename);

  // Capture username
  strcpy(&judge->user, request->user);

  // Capture module number
  strcpy(&judge->module_num, request->module_num);

  // Find the right module
  Module *module = find_module(&judge->module_num);
  if (!module) {
    printf("Warning - could not find module for %s\n", judge->module_num);
    free(judge->source_path);
    return 1;
  }

  // Set number of input files
  judge->num_input_files = module->num_input_files;

  // Capture input files path
  judge->input_files = (char **)malloc(judge->num_input_files * sizeof(char *));
  int i;
  for (i=0; i< judge->num_input_files; i++) {
    judge->input_files[i] = (char *)malloc(strlen(module->input_files[i]) + 1);
    strcpy(judge->input_files[i], module->input_files[i]);
  }

  // Set command line arguments
  int k = 0;
  judge->exec_args[k++] = JUDGE;
  judge->exec_args[k++] = &judge->id;
  judge->exec_args[k++] = judge->source_path;
  judge->exec_args[k++] = &judge->user;
  judge->exec_args[k++] = &judge->module_num;
  judge->exec_args[k++] = judge->fd_w;
  for (i=0; i<judge->num_input_files; i++, k++)
    judge->exec_args[k] = judge->input_files[i];
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
  pthread_mutex_lock(&judge_lock);
  while (i < MAX_JUDGES) {
    if (!active_judges[i]) {
      active_judges[i] = judge;
      pthread_mutex_unlock(&judge_lock);
      return 0;
    }
    i++;
  }
  pthread_mutex_unlock(&judge_lock);
  return 1;
}

/*
** Finds judge with id_str judge id from active_judges
*/
Judge *get_judge(char *id_str) {
  int i;
  for (i = 0; i < MAX_JUDGES; i++) {
    if (active_judges[i] && !strcmp(active_judges[i]->id, id_str))
      return active_judges[i];
  }
  return NULL;
}

/*
** Assuming judge is a valid judge, it copies the content of
** the sandbox to a permanent directory in submissions.
*/
void copy_sandbox_to_sub(Judge *judge) {
  char command[MAX_COMMAND_LEN];

  // Create user directory
  sprintf(command, "%s/%s", SUB, judge->user);
  mkdir(command, S_IRWXU | S_IRWXG);

  // Create module directory
  memset(command, 0, MAX_COMMAND_LEN);
  sprintf(command, "%s/%s/%s", SUB, judge->user, judge->module_num);
  mkdir(command, S_IRWXU | S_IRWXG);

  // Append date/time to directory name
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  memset(command, 0, MAX_COMMAND_LEN);
  sprintf(command, "cp -rf %s/%s/%s %s/%s/%s/%d_%d_%d_%d_%d_%d",
      SANDBOX, judge->user, judge->module_num,
      SUB, judge->user, judge->module_num, tm.tm_year + 1900, tm.tm_mon + 1,
      tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  system(command); 
  
  // Change owner and group to NON_PRIV_USR
  memset(command, 0, MAX_COMMAND_LEN);
  sprintf(command, "chown -R %d %s/%s", NON_PRIV_USR, SUB, judge->user);
  system(command);
  memset(command, 0, MAX_COMMAND_LEN);
  sprintf(command, "chgrp -R %d %s/%s", NON_PRIV_USR, SUB, judge->user);
  system(command);
}

/*
** Frees all memory associated with a Judge struct.
** Removes the entry from the active_judges list and trasnfers
** its content to the permanent SUB directory.
*/
void destruct_judge(Judge *judge) {
  
  // Make sure it still exists. Remove from active judges
  int found = 0, i;
  for (i = 0; i < MAX_JUDGES; i++) {
    if (active_judges[i] && !strcmp(active_judges[i]->id, judge->id)) {
      found = 1;
      active_judges[i] = NULL;
    }
  }
  if (!found) return;
 
  // Copy content of sandbox to submissions
  copy_sandbox_to_sub(judge);

  // Free all input file paths
  for (i = 0; i < judge->num_input_files; i++) {
    if (judge->input_files[i]) free(judge->input_files[i]);
  }

  // Free input file array
  free(judge->input_files);

  // Free source path
  free(judge->source_path);

  free(judge);
}

/*************************** Request Helpers *******************************/

/*
** Returns 0 if request is valid, 1 otherwise.
** Valid requests have valid usernames and contain a registered module number.
*/
int validate_request(Request *request) {

  // Send error message and return 1 if module_num not registered
  if (!is_registered(request->module_num)) {
    send_message(request->socket_fd, INV_MOD); 
    return 1;
  }

  if (strlen(request->user) < MIN_USER_LEN) {
    send_message(request->socket_fd, INV_USR);
    return 1;
  }

  int i;
  char *letter = &(request->user[0]);
  // Check user for invalid characters
  for(i = 0; i<strlen(request->user); i++, letter=&(request->user[i])){
    
    // If uppercase, convert to lowercase
    if (('A' <= *letter) && (*letter <= 'Z')) *letter = tolower(*letter);

    // If not [a-z0-9] send error message and return 1
    if (!(('0' <= *letter) && (*letter <= '9')) &&
        !(('a' <= *letter) && (*letter <= 'z'))) {
      send_message(request->socket_fd, INV_USR); 
      return 1;
    }
  }

  return 0;
}

/*
** Handler for incoming requests
** Creates a Judge struct with relevant info and forks off the process.
*/
void handle_request(Request *request) {
  Judge *judge = (Judge *)malloc(sizeof(Judge));

  if (validate_request(request)) return;

  if (init_judge(judge, request)) {
    send_message(request->socket_fd, UNK_ERR);
    return;
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
    if (DEBUG) printf("\nStarting Judge\n", judge_bin);
    execv(judge_bin, judge->exec_args);
    perror("exec failure");
    exit(EXIT_FAILURE);
  } else {
    judge->pid = pid;
    alarm(MAX_TIME_ALLOWED);
  }
}

/**************************** Coordinator Routines *******************************/

/*
** Blocking call to sit and listen on the read-end of the judges shared pipe.
** Message format: <message size><delimiter><judge_id><delimiter><ack_code><delimiter>
** Calls handle_ack(), which reacts appropriately based on type of ack.
*/
void listen_to_judges() {
  int bytes_received, bytes_read, pipe_in = fd[0];
  char message_content[MAX_PACKET_SIZE];
  char *message = &message_content;

  // Set up to parse message
  char *size=0, *judge_id=0, *ack_code=0, *token_state=0;

  while (1) {
    // Block until receive an ack to process
    bytes_read = 0;
    message = &message_content; 
    memset(message, 0, MAX_PACKET_SIZE);
    bytes_received = read(pipe_in, message, MAX_PACKET_SIZE);
    if (VERBOSE) printf("Received [%s]\n", message);

    while (bytes_received > bytes_read) {

      // Parse the message
      size = strtok_r(message, DELIM, &token_state);
      judge_id = strtok_r(NULL, DELIM, &token_state);
      ack_code = strtok_r(NULL, DELIM, &token_state);

      if (VERBOSE) printf("Read bytes (%d-%d)/%d from judge (%s): %s \n", bytes_read, bytes_read + atoi(size),
          bytes_received, judge_id, ack_code);

      bytes_read += atoi(size);
      message = message + atoi(size);

      // Find out which judge sent the response
      Judge *judge = get_judge(judge_id);

      if (!judge) {
        // Should never happen. TODO: Log this 
        // Break out of the inner loop
        printf("DANGER: judge %d has been deallocated\n");
        continue;
      }

      act_on_ack(judge, ack_code);
    }
  }
  // TODO: free(message) in final destructor
}

/*
** Generic signal handler registered for all signals. It will call the appropriate handler
** based on signal type.
*/
void signal_handler(int signal) {
  switch (signal) {
    case(SIGALRM):
      alarm_handler();
      break;
    case(SIGTERM):
      // Fall-through
    case(SIGINT):
      fatal_event_handler();
  }
}

/*
** Handler for fatal events.
** A fatal even could be a fatal signal (SIGTERM or SIGINT) or some bad input being received.
** Frees up system resources
*/
void fatal_event_handler() {
  // TODO: Free resources that will live after process is destroyed.
  // TODO: Cancel all worker threads (judge and pipe_listener)
  if (modules) destruct_modules();
  if (db) close_db(db);
  if (listen_queue_socket > 0) close(listen_queue_socket);
  exit(EXIT_FAILURE);
}

/*
** Handler for signals of type SIGLARM
** Checks to see if any judges have max'd out their time limit - if yes destroys them.
*/
void alarm_handler(int signal) {
  // Get current time
  struct timeval start, end;
  int lifetime;
  gettimeofday(&end, NULL);

  Judge *judge;

  // Find processes that have max'd out their time limitation
  int i;
  for (i = 0; i < MAX_JUDGES; i++) {
    if (active_judges[i]) {
      judge = active_judges[i];
      start = judge->time_struct;
      lifetime = (end.tv_sec * MILLI + end.tv_usec) -
                 (start.tv_sec * MILLI + start.tv_usec);

      // Check if process has max'd out
      if (lifetime > MAX_TIME_ALLOWED * MILLI) {

        // Don't do anything if judge process has terminated normally
        if (judge->terminated)
          continue;

        if (DEBUG)
          printf("Judge process (%d) for user (%s) timed out - Exiting\n",
                 judge->pid, &judge->user);

        // Echo ack to the client
        send_message(judge->socket_fd, TIM_OUT);

        // Record result in the database
        insert_record(db, judge->user, judge->module_num, TIM_OUT);

        // Do clean up for the judge
        int judge_pid = judge->pid;
        close_connection(judge->socket_fd);
        destruct_judge(judge);

        // Kill the judge process
        kill(judge_pid, SIGKILL);
        waitpid(judge_pid, NULL, NULL);
      }
    }
  }
}

/* 
** If supplied a non-fatal ack_code, echos to client and returns.
** For fatal ack_codes, records result in database and destructs the judge. 
** If an error file exists {CMP_ERR, RUN_ERR} echos its content to client. 
** If receives JDG_AOK sends over the solution file
*/
void act_on_ack(Judge *judge, char *ack_code) {
  
  // Echo ack to the client
  send_message(judge->socket_fd, ack_code);

  // If submission successful, send solution file over to client
  if (!strcmp(ack_code, &JDG_AOK)) {
    // Mark judge as terminated
    judge->terminated = 1;
    if (send_solution(judge->socket_fd, judge->module_num) && DEBUG)
      printf("Warning - could not send solution file\n");
  }

  // Don't do anything else for non-fatal acks
  if (strcmp(ack_code, &JDG_AOK) && strcmp(ack_code, &CMP_ERR) &&
      strcmp(ack_code, &RUN_ERR) && strcmp(ack_code, &CHK_ERR)) 
    return;

  // Mark judge as terminated if not already set
  if (!judge->terminated) judge->terminated = 1;

  // If an error code, echo error file to client
  if (!strcmp(ack_code, &CMP_ERR) || !strcmp(ack_code, &RUN_ERR)) {

    // Compute error file path
    char log_file[MAX_FILENAME_LEN];
    sprintf(log_file, "%s/%s/%s/%s_%s_%s", SANDBOX, judge->user, judge->module_num, judge->user, 
        judge->module_num, ERRORFILE_SUFFIX);

    // Send judge error ack
    send(judge->socket_fd, JDG_ERR, strlen(JDG_ERR), 0); 

    // TODO: prune the output that is sent over 
    send_file(judge->socket_fd, log_file);
  }

  // Record the result in database
  insert_record(db, judge->user, judge->module_num, ack_code);

  if (DEBUG)
    printf("Judge with pid (%d) for user (%s) is Done\n", judge->pid,
           judge->user);

 // Close socket connection, wait on judge process, and destruct judge
  close_connection(judge->socket_fd);
  waitpid(judge->pid, NULL, NULL);
  destruct_judge(judge);
}

void init_request(Request *request) {
  request->user = (char *)malloc(MAX_FILENAME_LEN);
  request->module_num = (char *)malloc(MAX_FILENAME_LEN);
  request->filename = (char *)malloc(MAX_FILENAME_LEN);
}

void destruct_request(Request *request) {
  free(request->user);
  free(request->module_num);
  free(request->filename);
}

void fatal_error(char *format, ...) {
  char message[MAX_COMMAND_LEN];

  va_list args_list;
  va_start(args_list, format);
  vsprintf(message, format, args_list);
  va_end(args_list);

  printf("Fatal: %s\n", message);
  fatal_event_handler();
}

void validate_dirs() {
  // Change current working directory to root of the application
  struct stat root_stat; 
  if ((stat(APP_ROOT, &root_stat) != 0) || !S_ISDIR(root_stat.st_mode))
    fatal_error("APP_ROOT [%s] not a valid directory. Please change in settings.", APP_ROOT);
  if ((stat(MODULES_ROOT, &root_stat) != 0) || !S_ISDIR(root_stat.st_mode))
    fatal_error("MODULES_ROOT [%s] not a valid directory. Please change in settings.", MODULES_ROOT);
  if ((stat(BIN_ROOT, &root_stat) != 0) || !S_ISDIR(root_stat.st_mode))
    fatal_error("BIN_ROOT [%s] not a valid directory. Please change in settings." BIN_ROOT);

  chdir(APP_ROOT);

  // Remove root privilege
  setegid(NON_PRIV_USR);
  seteuid(NON_PRIV_USR);

  // Make sure submissions directory exists
  if ((stat(SUB, &root_stat) != 0) || !S_ISDIR(root_stat.st_mode)) {
    if (mkdir(SUB, S_IRWXU | S_IRWXG))
      fatal_error("Cannot create %s directory - NON_PRIV_USR (%d) is most likely an invalid uid"
          , SUB, NON_PRIV_USR);
  }

  // Make sure sandbox directory exists
  if ((stat(SANDBOX, &root_stat) != 0) || !S_ISDIR(root_stat.st_mode)) {
    mkdir(SANDBOX, S_IRWXU | S_IRWXG);
  }

  // Restore root privilege
  seteuid(0);
  setegid(0);
}

/******************************* Main Routines ***********************************/

/*
** Starts listening for new connections, and handles incoming requests.
*/
int run_server() {

  // Stop if not running as root
  if (getuid()) {
    printf("%s", "This application requires escelated privileges. Please run as root.\n");
    exit(EXIT_FAILURE);
  }

  // Make sure all X_ROOT directories exist. Chdir into APP_ROOT
  validate_dirs();

  // Initialize modules
  init_modules();

  // Set up shared pipe
  pipe(fd);

  // Set up signal handler
  if ((signal(SIGALRM, signal_handler) == SIG_ERR ||
       signal(SIGTERM, signal_handler) == SIG_ERR || 
       signal(SIGINT, signal_handler) == SIG_ERR)
       && DEBUG)
    fatal_error("Could not register signal handler");

  // Open connection to the database
  if (open_db(&db, DB_PATH))
    fatal_error("Can't open database: %s\n", sqlite3_errmsg(db));

  // Create a submissions table if it does not already exist
  create_table(db);

  // Spawn a thread to listen on judges pipe
  pthread_t pipe_listener_thread;
  pthread_create(&pipe_listener_thread, NULL, (void *)&listen_to_judges, NULL);
  
  // Set up the mutex locks
  if (pthread_mutex_init(&inc_lock, NULL) ||
      pthread_mutex_init(&judge_lock, NULL)) 
    fatal_error("Could not initiazlie mutex lock");

  // Set up initial socket for listen queue
  if ((listen_queue_socket = set_up_server) < 0) 
    exit(EXIT_FAILURE);

  // Init and allocate enough space to hold the raw request
  Request request;

  int connection_socket;
  if (DEBUG) printf("Listening for connections on port %d...\n", PORT);

  // Until receive SIGKILL or SIGINT, continue listening
  // TODO: Better termination mechanism
  while (1) {
    
    // Block and listen for connections - upon receipt, move connection to new socket
    if ((connection_socket = listen_for_requests(listen_queue_socket)) >= 0) {

      // Set request's socket fd
      request.socket_fd = connection_socket;

      // Let the handler receive request data
      init_request(&request);
      receive_request(&request);
      handle_request(&request);
      destruct_request(&request);
    }
  }
}

/*
** Looks up and reports all records for specified module
*/
int module_results(char *module_num) {
  // Open connection to the database
  if (open_db(&db, DB_PATH))
    fatal_error("Can't open database: %s\n", sqlite3_errmsg(db));
   
  char response_buffer[MAX_DB_RESULT_LEN];

  if (lookup_module(db, module_num, response_buffer))
    fatal_error("Can't look up module: %s\n", sqlite3_errmsg(db));

  if (!*response_buffer) printf("No records were found for module %s\n", module_num);
  else printf("%s", response_buffer);
}

/*
** Looks up and reports all records for specified user
*/
int user_results(char *user) {
  // Open connection to the database
  if (open_db(&db, DB_PATH))
    fatal_error("Can't open database: %s\n", sqlite3_errmsg(db));

  char response_buffer[MAX_DB_RESULT_LEN];

  if (lookup_user(db, user, response_buffer))
    fatal_error("Can't look up user: %s\n", sqlite3_errmsg(db));

  if (!*response_buffer) printf("No records were found for user %s\n", user);
  else printf("%s", response_buffer);
}

int main(int argc, char **argv) {
  if (argc == 2 && !strcmp(argv[1], RUN_SERVER)) 
    return run_server();
  if(argc == 3 && !strcmp(argv[1], MODULE_RESULTS)) 
    return module_results(argv[2]);
  if (argc == 3 && !strcmp(argv[1], USER_RESULTS)) 
    return user_results(argv[2]);

  printf("USAGE: %s MODE [ARGUMENT] \nMODES: %s, %s, %s\n"
      , argv[0], RUN_SERVER, MODULE_RESULTS, USER_RESULTS);
  return 0;
}
