#include "client.h"

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
    printf("Failed obtaining a socket file descriptor\n");
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
    printf("Cannot connect to server\n");
    return -1;
  }
  printf("Succesfully connected to the server\n");

  return socket_fd;
}

/*
** If ack is an ack_code, echos its meaning, otherwise echos 
** the content of the message.
** Returns 1 if no more messages will be received
*/
int handle_ack(char *ack) {
    if (!strncmp(ack, RCV_AOK, strlen(RCV_AOK))) {
        printf(KGRN "Server succesfully received file\n" KYEL);
    } else if (!strncmp(ack, INV_USR, strlen(INV_USR))){
        printf(KRED "Invalid username\n" KYEL);
        return 1;
    } else if (!strncmp(ack, INV_MOD, strlen(INV_MOD))){
        printf(KRED "Invalid module number\n" KYEL);
        return 1;
    } else if (!strncmp(ack, UNK_ERR, strlen(UNK_ERR))){
        printf(KRED "Unknown error occured while processing request. "
            "Please make sure you have complied with assignment requirements.\n" KYEL);
        return 1;
    } else if (!strncmp(ack, CMP_AOK, strlen(CMP_AOK))){
        printf(KGRN "Succesfully compiled file\n" KYEL);
    } else if (!strncmp(ack, CMP_ERR, strlen(CMP_ERR))){
        printf(KRED "Could not compile file using gcc\n" KYEL);
    } else if (!strncmp(ack, RUN_AOK, strlen(RUN_AOK))){
        printf(KGRN "Succesfully ran executable against input file\n" KYEL);
    } else if (!strncmp(ack, RUN_ERR, strlen(RUN_ERR))){
        printf(KRED "There was a runtime error while running against input file\n" KYEL);
    } else if (!strncmp(ack, CHK_AOK, strlen(CHK_AOK))){
        printf(KGRN "Succesfully passed a test case\n" KYEL);
    } else if (!strncmp(ack, CHK_ERR, strlen(CHK_ERR))){
        printf(KRED "Did not pass a test case\n" KYEL);
        return 1;
    } else if (!strncmp(ack, JDG_AOK, strlen(JDG_AOK))){
        printf(KGRN "Your submission was accepted - great job!\n" KYEL);
        return 1;
    } else if (!strncmp(ack, JDG_ERR, strlen(JDG_ERR))){
        printf(KRED "Your submission was not accepted.\n" KYEL);
    } else if (!strncmp(ack, TIM_OUT, strlen(TIM_OUT))){
        printf(KRED "Submitted program timed out\n" KYEL);
        return 1;
    } else if (!strncmp(ack, BEG_FIL, strlen(BEG_FIL))) {
        printf(KYEL "Following message was received from server:\n" KYEL);
    } else {
        printf("%s%s%s", KNRM, ack, KYEL);
    }

    return 0;
}

/*
** Sends the request packet {username, module number, source file}
** to the server over socket_fd. It then waits for the server to send
** back acknowledgement messages, echoing them as they are received.
** It halts when server closes the connection.
** TODO: Break up into 2 loosely coupled routines.
*/
int send_request(int socket_fd, char *lfile, char *user, char *module_num) {

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
  if (file_size > 1024)
    printf("Reading content of %s (%d Kb)\n", lfile, file_size/1024);
  else 
    printf("Reading content of %s (%d bytes)\n", lfile, file_size);
  
  // Write request header
  char write_buffer[TCP_PACKET_SIZE];
  sprintf(write_buffer, "%s%s%s%s%s%s%d\r\n", HEADER_PREFIX, ARG_DELIM, user,
          ARG_DELIM, module_num, ARG_DELIM, file_size);
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

  printf("Finished sending file content\nWaiting for confirmation from server...\n");

  char read_buffer[TCP_PACKET_SIZE];

  // Receive status updates (or error messages) until connection is closed or handle_ack
  // signals a force exit.
  while ((recv(socket_fd, read_buffer, ACK_LEN, 0) > 0)) {
    if (handle_ack(&read_buffer)) break;
    memset(read_buffer, 0, TCP_PACKET_SIZE);
  }

  printf("Connection closed - Exiting\n");
  fclose(file_to_send);

  return 0;
}

int main(int argc, char **argv) {

  // Make sure we got the right number of arguments
  if (argc != 4) {
    printf("USAGE: %s <Username> <Module number> <File to submit>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Change text color to yellow
  printf(KYEL);

  // Set request parameters
  const char *server_address = "localhost";
  const int port_number = 31337;
  const char *file_to_send = argv[3];
  const char *user = argv[1];
  const char *module_num = argv[2];

  // Establish a TCP connection to the server
  int socket_fd = connect_to_server(server_address, port_number);
 
  if (socket_fd  < 0) {
    exit(EXIT_FAILURE);
  }

  // Send the request and echo results
  if (send_request(socket_fd, file_to_send, user, module_num)) {
    close(socket_fd);
    exit(EXIT_FAILURE);
  }

  // Close connection
  close(socket_fd);

  return 0;
}
