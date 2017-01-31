/**************************
 * @author: Pedram Pejman *
 * Module 0 Solution      *
 **************************/

#include <stdio.h>
#include <string.h>

#define EXIT             "exit"
#define MAX_INPUT_LEN     1024 

/*
 * Blocks and waits for input form stdin.
 * If input is EXIT keyword, terminates.
 * Otherwise, writes the number of bytes in the input to stdout.
 */
int main() {
  char buffer[MAX_INPUT_LEN];

  while (!feof(stdin)) {

    // Read input from stdin
    memset(buffer, 0, MAX_INPUT_LEN);
    fgets(buffer, MAX_INPUT_LEN, stdin);

    // Get rid of the \n at the end of input
    buffer[strlen(buffer)-1] = '\0';

    // Terminate if input is EXIT
    if (!strcmp(buffer, EXIT)) return 0;

    // Write buffer size (unsigned long) to stdout
    printf("%zu\n", strlen(buffer));
  }
}
