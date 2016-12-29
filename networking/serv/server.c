#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 31337
#define ARG_DELIM ":"
#define TCP_PACKET_SIZE 4096
#define HEADER_PREFIX "FBEGIN"
#define MAX_WAITING_CONNECTIONS 5

int parse_arguments(char **args, char *line) {
  int i = 0;
  args[i] = strtok(line, ARG_DELIM);
  while ((args[++i] = strtok(NULL, ARG_DELIM)) != NULL)
    ;
  return i - 1;
}

setup_socket() {}

int do_everything() {

  int listen_queue_socket, connection_socket;
  socklen_t client_address_len;
  struct sockaddr_in client_address, server_address;


  // Create socket to listen for incoming connections
  listen_queue_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_queue_socket < 0) {
    printf("Cannot create socket\n");
    close(listen_queue_socket);
    return 1;
  }
  printf("Created socket\n");

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(PORT);

  // Bind the socket to server_address
  if (bind(listen_queue_socket, (struct sockaddr *)&server_address,
           sizeof(server_address)) < 0) {
    printf("Cannot bind socket\n");
    close(listen_queue_socket);
    return 1;
  }
  printf("Bound socket\n");

  // Listen for new connections
  printf("Listening for connections on port %d\n", PORT);
  listen(listen_queue_socket, MAX_WAITING_CONNECTIONS);

  // Accept new connections, move them to connection_socket
  client_address_len = sizeof(client_address);
  connection_socket =
      accept(listen_queue_socket, (struct sockaddr *)&client_address,
             &client_address_len);
  printf("Accepted connection\n");

  // Make sure connection socket descriptor is valid
  if (connection_socket < 0) {
    printf("Cannot accept connection\n");
    close(listen_queue_socket);
    return 1;
  }
  printf("Connection is valid - retrieving header\n");

  // Receive the header_tokens and write the file
  char receive_buffer[TCP_PACKET_SIZE];
  if (recv(connection_socket, receive_buffer, sizeof(receive_buffer), 0)) {
    
    char *header_tokens[TCP_PACKET_SIZE];
    char *user, *ass_num, *filename, *filesize;

    // Make sure the first packet is the header data
    if (!strncmp(receive_buffer, HEADER_PREFIX, strlen(HEADER_PREFIX))) {

      // Parse the header and capture data
      parse_arguments(header_tokens, receive_buffer);
      user = header_tokens[1];
      ass_num = header_tokens[2];
      filename = header_tokens[3];
      filesize = header_tokens[4];
    
    } else {
      // Print error message and exit
      printf("Failed to received header - first packet content: [%s]\n",
             receive_buffer);
      close(connection_socket);
      close(listen_queue_socket);
      exit(EXIT_FAILURE);
    }

    printf("Retreived and parsed header information\n");

    // Open file to write to 
    FILE *file_to_write = fopen(filename, "w");
    int bytes_received, bytes_remaining = atoi(filesize);

    printf("Opened file to write to\n");

    // Listen until no more packets are being sent
    while ((bytes_received = recv(connection_socket, receive_buffer, TCP_PACKET_SIZE, 0)) != 0) {

      // Write the received packet of data to the file
      fwrite(receive_buffer, sizeof(receive_buffer[0]), bytes_received,
             file_to_write);

      bytes_remaining -= bytes_received;
    }

    printf("Succesfully retrieved file\n");

    // Make sure we received the number of bytes we expected
    if (bytes_remaining != 0) {
      printf("Warning - Expected %d bytes, received %d bytes\n");
    }

    close(listen_queue_socket);
  } else {
    printf("Client dropped connection\n");
  }
}

int main() { do_everything(); }
