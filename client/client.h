#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define VERBOSE 1

#define server_address   "35.185.24.113"
//#define server_address   "localhost"
#define port_number       31337

#define INSTR            "instructor"
#define SOL_SUFFIX       "sol.c"
#define ARG_DELIM        ":"
#define HEADER_PREFIX    "FBEGIN"
#define TCP_PACKET_SIZE   1<<9
#define MAX_ERROR_SIZE    1<<10
#define MAX_FILENAME_LEN  1<<7
#define MAX_HEADER_ELEMS  1<<3
#define KILOBYTE          1<<10

#define KNRM             "\x1B[00m"
#define KRED             "\x1B[31m"
#define KGRN             "\x1B[32m"
#define KYEL             "\x1B[33m"

#define ACK_LEN 7 

/* Acknowledgment Codes */
#define CMP_AOK   "CMP_AOK" /* Compilation succeeded            */
#define CMP_ERR   "CMP_ERR" /* Compilation failed         FATAL */
#define RUN_AOK   "RUN_AOK" /* A run succeeded                  */
#define RUN_ERR   "RUN_ERR" /* A run failed               FATAL */
#define TIM_OUT   "TIM_OUT" /* A run timed out            FATAL */
#define CHK_AOK   "CHK_AOK" /* A diff succeeded                 */
#define CHK_ERR   "CHK_ERR" /* A diff failed              FATAL */

#define JDG_ERR   "JDG_ERR" /* Solution not accepted      FATAL */
#define JDG_AOK   "JDG_AOK" /* Solution accepted                */

#define REQ_AOK   "REQ_AOK" /* Client request received          */
#define FIL_AOK	  "FIL_AOK" /* File received OK 		*/
#define FIL_ERR	  "FIL_ERR" /* File received with error		*/
#define HDR_AOK	  "HDR_AOK" /* Header received OK		*/

#define INV_USR   "INV_USR" /* Invalid username           FATAL */
#define INV_MOD   "INV_MOD" /* Invalid module number      FATAL */
#define UNK_ERR   "UNK_ERR" /* Unknown error              FATAL */
#define BEG_FIL   "BEG_FIL" /* About to stream file             */
#define BEG_SOL   "BEG_SOL" /* About to stream solution         */

/* Acknowledgement Messages */
#define REQ_AOK_MSG KGRN "Server succesfully received file\n" KYEL
#define INV_USR_MSG KRED "Invalid username\n" KYEL
#define INV_MOD_MSG KRED "Invalid module number\n" KYEL
#define CMP_AOK_MSG KGRN "Succesfully compiled file\n" KYEL 
#define CMP_ERR_MSG KRED "Could not compile file using gcc\n" KYEL
#define RUN_AOK_MSG KGRN "Succesfully ran executable with input file\n" KYEL
#define RUN_ERR_MSG KRED "There was a runtime error while running with input file\n" KYEL 
#define CHK_AOK_MSG KGRN "Succesfully passed a test case\n" KYEL
#define CHK_ERR_MSG KRED "Did not pass a test case\n" KYEL
#define JDG_AOK_MSG KGRN "Your submission was accepted - great job!\n" KYEL
#define JDG_ERR_MSG KRED "Your submission was not accepted.\n" KYEL
#define TIM_OUT_MSG KRED "Submitted program timed out\n" KYEL
#define BEG_FIL_MSG KYEL "Following message was received from server:\n" KYEL
#define UNK_ERR_MSG KRED "Unknown error occured while processing request.\nPlease make sure you have complied with assignment requirements.\n" KYEL   
  
/*
** Indicates the state of the connection.
** Each ack can 
** 1) not change connection state
** 2) terminate it
** 3) send the client into file receiving mode
*/
typedef enum {
  NO_ACTION,
  TERMINATE,
  RECEIVE_SOLUTION
} AckResult;

/*
** Establishes a TCP connection to server on specified port
** Returns file descriptor for socket or -1 for error and writes error 
** error description in the error buffer.
*/
int connect_to_server(char *server, int PORT, char *error);

/*
** If ack is an ack_code, echos its meaning, otherwise echos 
** the content of the message.
** Returns an AckResult enum member
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

/*
** Receives the solution to the current module from the server
** Saves the file in the current directory
*/
void receive_solution(int socket_fd);
