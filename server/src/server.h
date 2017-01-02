#ifndef SERVER_H
#define SERVER_H

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define PORT 31337
#define ARG_DELIM ":"
#define TCP_PACKET_SIZE 4096
#define HEADER_PREFIX "FBEGIN"
#define MAX_WAITING_CONNECTIONS 5
#define MAX_FILENAME_LEN 64
#define TEMP "tmp/"

typedef struct {
  char *user;
  char *ass_num;
  char *filename;
  int socket_fd;
} Request;

int set_up_server();
int listen_for_requests(int listen_queue_socket);
int receive_request(Request *request);
int send_message(int socket_fd, char *message);
int send_file(int socket_fd, char *filepath);
int close_connection(int socket_fd);

#endif
