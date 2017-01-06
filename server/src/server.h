#ifndef SERVER_H
#define SERVER_H

typedef struct {
  char *user;
  char *ass_num;
  char *filename;
  int socket_fd;
} Request;

/*
** Sets up listen queue socket and returns its file descriptor
*/
int set_up_server();

/*
** Blocks and waits for a client to make a request
** Upon receipt, moves the connection to a new socket and returns its fd
*/
int listen_for_requests(int listen_queue_socket);

/* 
** Actually does the job of receiving the request data from the client
** Returns 0 for no errors and 1 if any errors occur.
*/
int receive_request(Request *request);

/*
** Assumes socket_fd is a valid connection
** Sends message on the socket connection
*/
int send_message(int socket_fd, char *message);

/*
** Sends the content of the file over the socket.
*/
int send_file(int socket_fd, char *filepath);

/*
** Closes the socket connection and does any necassary clean-up.
*/
int close_connection(int socket_fd);

#endif
