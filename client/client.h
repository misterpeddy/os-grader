#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARG_DELIM ":"
#define HEADER_PREFIX "FBEGIN"
#define TCP_PACKET_SIZE 4096
#define MAX_ERROR_SIZE 1024

#define KNRM  "\x1B[00m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"

#define ACK_LEN 7 

/* Acknowledgment messages */
#define CMP_AOK   "CMP_AOK" /* Compilation succeeded            */
#define CMP_ERR   "CMP_ERR" /* Compilation failed         FATAL */
#define RUN_AOK   "RUN_AOK" /* A run succeeded                  */
#define RUN_ERR   "RUN_ERR" /* A run failed               FATAL */
#define TIM_OUT   "TIM_OUT" /* A run timed out            FATAL */
#define CHK_AOK   "CHK_AOK" /* A diff succeeded                 */
#define CHK_ERR   "CHK_ERR" /* A diff failed              FATAL */

#define JDG_ERR   "JDG_ERR" /* Solution not accepted      FATAL */
#define JDG_AOK   "JDG_AOK" /* Solution accepted          FATAL */

#define RCV_AOK   "RCV_AOK" /* Client request received          */
#define INV_USR   "INV_USR" /* Invalid username           FATAL */
#define INV_MOD   "INV_MOD" /* Invalid module number      FATAL */
#define UNK_ERR   "UNK_ERR" /* Unknown error              FATAL */
#define BEG_FIL   "BEG_FIL" /* About to stream file             */

/*
** Establishes a TCP connection to server on specified port
** Returns file descriptor for socket or -1 for error and writes error 
** error description in the error buffer.
*/
int connect_to_server(char *server, int PORT, char *error);

/*
** If ack is an ack_code, echos its meaning, otherwise echos 
** the content of the message.
** Returns 1 if no more messages will be received
*/
int handle_ack(char *ack);

/*
** Sends the request packet {username, module number, source file}
** to the server over socket_fd. It then waits for the server to send
** back acknowledgement messages, echoing them as they are received.
** It halts when server closes the connection.
** TODO: Break up into 2 loosely coupled routines.
*/
int send_request(int socket_fd, char *lfile, char *user, char *module_num); 

