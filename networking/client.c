#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARG_DELIM ":"
#define HEADER_PREFIX "FBEGIN"
#define TCP_PACKET_SIZE 4096

const char RCV_AOK[] = "RCV_AOK";

/*
** Establishes a TCP connection to server on specified port
** Returns file descriptor for socket or -1 for error
*/
int connect_to_server(char *server, int PORT) {
  // Interpret server address
  struct hostent *host_info = gethostbyname(server);
  if (host_info == NULL) {
    printf("Could not look up server address - Exiting\n");
    return -1;
  }

  // Create socket to connect to the server
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    printf("Cannot create socket_fd\n");
    return -1;
  }

  // Initialize socket address struct 
  struct sockaddr_in server_address;
  server_address.sin_family = host_info->h_addrtype;
  memcpy((char *)&server_address.sin_addr.s_addr, host_info->h_addr_list[0],
         host_info->h_length);
  server_address.sin_port = htons(PORT);

  // Try to connect to the server
  if (connect(socket_fd, (struct sockaddr *)&server_address,
              sizeof(server_address)) < 0) {
    printf("Cannot connect\n");
    return -1;
  }
  printf("Succesfully connected to the server\n");

  return socket_fd;
}

int send_file(int socket_fd, char *lfile, char *rfile, char *user, char *ass_num) {

  // Open file to send
  FILE *file_to_send = fopen(lfile, "r");
  if (!file_to_send) {
    printf("Error opening file\n");
    close(socket_fd);
    return 1;
  }

  // Compute file size
  long file_size;
  fseek(file_to_send, 0, SEEK_END);
  file_size = ftell(file_to_send);
  rewind(file_to_send);
  printf("Reading %s content - %d Kb\n", lfile, file_size/1024);
  
  // Write request header
  char write_buffer[TCP_PACKET_SIZE];
  sprintf(write_buffer, "%s%s%s%s%s%s%s%s%d\r\n", HEADER_PREFIX, ARG_DELIM, user,
          ARG_DELIM, ass_num, ARG_DELIM, rfile, ARG_DELIM, file_size);
  send(socket_fd, write_buffer, sizeof(write_buffer), 0);

  memset(&write_buffer, 0, TCP_PACKET_SIZE);
  int remaining = file_size, buff_size = 0, bytes_read;

  printf("Sending file content...\n");

  // Send the entire file over the socket_fd
  while (remaining > 0) {
    bytes_read = fread(&write_buffer, 1, sizeof(write_buffer), file_to_send);

    // Exit if read resulted in error
    if (bytes_read < 0) {
      perror("Error reading from file:");
      fclose(file_to_send);
      close(socket_fd);
      return 1;
    }

    // Send the data read
    if (remaining >= sizeof(write_buffer))
      buff_size = sizeof(write_buffer);
    else
      buff_size = remaining;
    send(socket_fd, write_buffer, buff_size, 0);
    remaining -= buff_size;
  }

  printf("Finished sending file content\nwaiting for confirmation from server...\n");

  // Wait for ack from server
  char read_buffer[TCP_PACKET_SIZE];
  if (recv(socket_fd, read_buffer, sizeof(read_buffer), 0) < 1) {
    printf("Server did not receive file - Exiting\n");
    return 1;
  }

  // Echo ack
  if (!strncmp(read_buffer, RCV_AOK, strlen(RCV_AOK))) {
    printf("Server succesfully received file\n");
  } else {
    printf("Server did not receive file - Exiting\n");
    return 1;
  }

  fclose(file_to_send);
  close(socket_fd);

  return 0;
}

int main(int argc, char **argv) {

  // Make sure we got the right number of arguments
  if (argc != 4) {
    printf("USAGE: %s <Username> <Assignment number> <File to submit>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Set request parameters
  const char *server_address = "localhost";
  const int port_number = 31337;
  const char *file_to_send = argv[3];
  const char *rename_file_to = argv[3];
  const char *user = argv[1];
  const char *ass_num = argv[2];

  int socket_fd = connect_to_server(server_address, port_number);
 
  if (socket_fd  < 0) {
    exit(EXIT_FAILURE);
  }

  if (send_file(socket_fd, file_to_send, rename_file_to, user, ass_num)) {
    exit(EXIT_FAILURE);
  }
  return 0;
}
