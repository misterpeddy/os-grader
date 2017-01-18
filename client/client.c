#include "client.h"

/*
** Establishes a TCP connection to server on specified port
** Returns file descriptor for socket or -1 for error and writes error 
** error description in the error buffer.
*/
int connect_to_server(char *server, int PORT, char *error) {
  // Interpret server address
  struct hostent *host_info = gethostbyname(server);
  if (host_info == NULL) {
    sprintf(error, "Could not look up server address");
    return -1;
  }

  // Create socket to connect to the server
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    sprintf(error, "Failed obtaining a socket file descriptor");
    return -1;
  }

  // Initialize socket address struct 
  struct sockaddr_in server_addr;
  server_addr.sin_family = host_info->h_addrtype;
  memcpy((char *)&server_addr.sin_addr.s_addr, host_info->h_addr_list[0],
         host_info->h_length);
  server_addr.sin_port = htons(PORT);

  // Try to connect to the server
  if (connect(socket_fd, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    sprintf(error, "Cannot connect to server");
    return -1;
  }
  printf("Succesfully connected to the server\n");

  return socket_fd;
}

/*
** If ack is an ack_code, echos its meaning, otherwise echos 
** the content of the message.
** Returns an AckResult enum member
*/
int handle_ack(char *ack) {
	if (!strncmp(ack, REQ_AOK, strlen(REQ_AOK))) {
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
			printf(KGRN "Succesfully ran executable with input file\n" KYEL);
	} else if (!strncmp(ack, RUN_ERR, strlen(RUN_ERR))){
			printf(KRED "There was a runtime error while running with input file\n" KYEL);
	} else if (!strncmp(ack, BEG_SOL, strlen(BEG_SOL))){
			return RECEIVE_SOLUTION;
	} else if (!strncmp(ack, CHK_AOK, strlen(CHK_AOK))){
			printf(KGRN "Succesfully passed a test case\n" KYEL);
	} else if (!strncmp(ack, CHK_ERR, strlen(CHK_ERR))){
			printf(KRED "Did not pass a test case\n" KYEL);
			return 1;
	} else if (!strncmp(ack, JDG_AOK, strlen(JDG_AOK))){
			printf(KGRN "Your submission was accepted - great job!\n" KYEL);
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

	return NO_ACTION;
}

/*
** Parses the header of the request and returns an array of tokens
*/
int parse_arguments(char **args, char *line) {
  int i = 0;
  args[i] = strtok(line, ARG_DELIM);
  while ((args[++i] = strtok(NULL, ARG_DELIM)) != NULL);
  return i - 1;
}

/*
** Receives the solution to the current module from the server
** Saves the file in the current directory
*/
void receive_solution(int socket_fd) {

	// Receive the header_tokens and write the file
	char header_buffer[TCP_PACKET_SIZE];
	if (recv(socket_fd, header_buffer, sizeof(header_buffer), 0)) {
    char *header_tokens[MAX_HEADER_ELEMS];
    char module_num[MAX_FILENAME_LEN];
    int bytes_remaining, bytes_received;

    // Make sure the first packet is the header data
    if (!strncmp(header_buffer, HEADER_PREFIX, strlen(HEADER_PREFIX))) {

      // Parse the header and capture data
      parse_arguments(header_tokens, header_buffer);
      strcpy(module_num, header_tokens[1]);	
      bytes_remaining = atoi(header_tokens[2]);

			// Send header received ack
			send(socket_fd, HDR_AOK, strlen(HDR_AOK), 0);

    } else {
      // Print error message and exit
      printf("Failed to receive instructor solution received [%s]\n",
             header_buffer);
      close(socket_fd);
      return 1;
    }

    char filepath[MAX_FILENAME_LEN];
    sprintf(filepath, "%s_%s", module_num, SOL_SUFFIX);

    // Open file to write to and
    FILE *file_to_write = open(filepath, O_WRONLY | O_TRUNC | O_CREAT, 00664);

    int total_received=0;
	  char receive_buffer[TCP_PACKET_SIZE];

    // Listen until file is received or no more packets are being sent
		while ((bytes_remaining > 0) && 
        ((bytes_received = recv(socket_fd, receive_buffer, TCP_PACKET_SIZE, 0)) > 0)) {

      // Write the received packet of data to the file
      write(file_to_write, receive_buffer, bytes_received);
      total_received += bytes_received;
      bytes_remaining -= bytes_received;
    }

    // Make sure we received the number of bytes we expected
    if (!bytes_remaining) {
			send(socket_fd, FIL_AOK, strlen(FIL_AOK), 0);
			printf("Succesfully retrieved instructor's solution file - " 
          "Saved in current directory\n");
    } else {
			send(socket_fd, FIL_ERR, strlen(FIL_ERR), 0);
    }

    close(file_to_write);

  } else {
    printf("Server dropped connection\n\n");
  }

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
  
  // Write and send request header
  char write_buffer[TCP_PACKET_SIZE];
  sprintf(write_buffer, "%s%s%s%s%s%s%d\r\n", HEADER_PREFIX, ARG_DELIM, user,
          ARG_DELIM, module_num, ARG_DELIM, file_size);
  send(socket_fd, write_buffer, sizeof(write_buffer), 0);


	// Wait for header to be received
  memset(&write_buffer, 0, TCP_PACKET_SIZE);
	recv(socket_fd, write_buffer, TCP_PACKET_SIZE, 0);
	if (strcmp(write_buffer, HDR_AOK)) {
		printf("Error sending request to server\n");
		fclose(file_to_send);
	  close(socket_fd);
    return 1;  
	}

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
  int ack_result;
  while ((recv(socket_fd, read_buffer, ACK_LEN, 0) > 0)) {
    ack_result = handle_ack(&read_buffer);

    if (ack_result == TERMINATE) 
      break;
    else if (ack_result == RECEIVE_SOLUTION) 
      receive_solution(socket_fd);

    memset(read_buffer, 0, TCP_PACKET_SIZE);
  }

  printf("Connection closed - Exiting\n");
  fclose(file_to_send);

  return 0;
}

void fatal_error(char *message) {
    if (message) printf("fatal: %s\n", message);
    printf(KNRM);
    exit(EXIT_FAILURE);
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
  const char *file_to_send = argv[3];
  const char *user = argv[1];
  const char *module_num = argv[2];

  // Establish a TCP connection to the server
  char error_message[MAX_ERROR_SIZE];
  int socket_fd = connect_to_server(server_address, port_number, error_message);
 
  if (socket_fd  < 0) {
    fatal_error(error_message);
  }

  // Send the request and echo results
  if (send_request(socket_fd, file_to_send, user, module_num)) {
    close(socket_fd);
    fatal_error(NULL);
  }

  // Close connection
  close(socket_fd);

  printf(KNRM);

  return 0;
}
