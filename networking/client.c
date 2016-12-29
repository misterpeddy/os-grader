#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARG_DELIM ":"
#define HEADER_PREFIX "FBEGIN"
#define TCP_PACKET_SIZE 4096

int fileSEND(char *server, int PORT, char *lfile, char *rfile, char *user,
             char *ass_num) {
  int socketDESC;
  struct sockaddr_in serverADDRESS;
  struct hostent *hostINFO;
  FILE *file_to_send;
  int ch;
  char toSEND[1];
  char remoteFILE[4096];
  int count1 = 1, count2 = 1, percent;

  hostINFO = gethostbyname(server);
  if (hostINFO == NULL) {
    printf("Problem interpreting host\n");
    return 1;
  }

  socketDESC = socket(AF_INET, SOCK_STREAM, 0);
  if (socketDESC < 0) {
    printf("Cannot create socket\n");
    return 1;
  }
  printf("Created socket\n");

  serverADDRESS.sin_family = hostINFO->h_addrtype;
  memcpy((char *)&serverADDRESS.sin_addr.s_addr, hostINFO->h_addr_list[0],
         hostINFO->h_length);
  serverADDRESS.sin_port = htons(PORT);

  if (connect(socketDESC, (struct sockaddr *)&serverADDRESS,
              sizeof(serverADDRESS)) < 0) {
    printf("Cannot connect\n");
    return 1;
  }
  printf("Connected to host\n");

  file_to_send = fopen(lfile, "r");
  if (!file_to_send) {
    printf("Error opening file\n");
    close(socketDESC);
    return 0;
  }
  printf("Opened file to send\n");

  long fileSIZE;
  fseek(file_to_send, 0, SEEK_END);
  fileSIZE = ftell(file_to_send);
  rewind(file_to_send);

  sprintf(remoteFILE, "%s%s%s%s%s%s%s%s%d\r\n", HEADER_PREFIX, ARG_DELIM, user,
          ARG_DELIM, ass_num, ARG_DELIM, rfile, ARG_DELIM, fileSIZE);
  send(socketDESC, remoteFILE, sizeof(remoteFILE), 0);
  printf("Sent request header\n");

  char buffer[TCP_PACKET_SIZE];
  int remaining = fileSIZE, buff_size = 0, bytes_read;

  printf("Sending file content\n");

  // Send the entire file over the socket
  while (remaining > 0) {
    bytes_read = fread(&buffer, 1, sizeof(buffer), file_to_send);

    if (bytes_read < 0) {
      perror("Error reading from file \n");
      fclose(file_to_send);
      close(socketDESC);
      exit(EXIT_FAILURE);
    }

    if (remaining >= sizeof(buffer))
      buff_size = sizeof(buffer);
    else
      buff_size = remaining;
    send(socketDESC, buffer, buff_size, 0);
    remaining -= buff_size;
  }

  printf("Finished sending file content\n");

  fclose(file_to_send);
  close(socketDESC);

  return 0;
}

int main() {
  fileSEND("localhost", 31337, "a.txt", "b.txt", "pp5nv", "2");
  return 0;
}
