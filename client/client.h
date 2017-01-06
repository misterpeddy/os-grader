#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARG_DELIM ":"
#define HEADER_PREFIX "FBEGIN"
#define TCP_PACKET_SIZE 4096

#define KNRM  "\x1B[00m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"

#define ACK_LEN 7 

const char RCV_AOK[] = "RCV_AOK";
const char CMP_AOK[] = "CMP_AOK";
const char CMP_ERR[] = "CMP_ERR";
const char RUN_AOK[] = "RUN_AOK";
const char RUN_ERR[] = "RUN_ERR";
const char CHK_AOK[] = "CHK_AOK";
const char CHK_ERR[] = "CHK_ERR";
const char JDG_AOK[] = "JDG_AOK";
const char JDG_ERR[] = "JDG_ERR";
const char TIM_OUT[] = "TIM_OUT";
const char BEG_FIL[] = "BEG_FIL";

/*
** Establishes a TCP connection to server on specified port
** Returns file descriptor for socket or -1 for error
*/
int connect_to_server(char *server, int PORT);


