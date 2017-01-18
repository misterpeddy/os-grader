/**************************
 * @author: Pedram Pejman *
 * Module 1 Solution      *
 **************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_INPUT_LEN   128
#define OPERATORS       "*/+-"

/*
 * Enumeration of supported operations
 */
enum {
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE
}; 

/*
 * Fills in arg1, arg2, operator from values in input
 * Expects input format <arg1><operator><arg2> 
 * Returns 1 if input is bad 0 otherwise.
 */
int interpret_input(char *input, uint32_t *arg1, uint32_t *arg2, char *operator) {
  if (strlen(input) < 3) return 1;

  // Figure out which operator is supplied
  int op_index = strcspn(input, OPERATORS);
  switch (input[op_index]) 
  {
    case '+':
      *operator = ADD;
      break;
    case '-':
      *operator = SUBTRACT;
      break;
    case '*':
      *operator = MULTIPLY;
      break;
    case '/':
      *operator = DIVIDE;
      break;
    default:
      // Unreachable
      return 1;
  }

  // Capture the two arguments
  *arg1 = atoi(strtok(input, OPERATORS));
  *arg2 = atoi(strtok(NULL, OPERATORS));

  return 0;
}

/*
 * Returns the result of carrying out the operation specified by operator
 * on arg1 and arg2. Assumes both arguments and the result are positive integers.
 */
uint32_t do_operation(uint32_t arg1, uint32_t arg2, char operator) {
  switch (operator)
  {
    case ADD:
      return arg1 + arg2;
    case SUBTRACT:
      return arg1 - arg2;
    case MULTIPLY:
      return arg1 * arg2;
    case DIVIDE:
      return arg1 / arg2;
  }
}

int main() {

  // Read input from stdin
  char buffer[MAX_INPUT_LEN];
  memset(buffer, 0, MAX_INPUT_LEN);
  fgets(buffer, MAX_INPUT_LEN, stdin);

  // Capture the operands and operator; if error occured, exit
  uint32_t arg1, arg2;
  char operator;
  if (interpret_input(buffer, &arg1, &arg2, &operator))
   exit(EXIT_FAILURE); 
  
  // Print out answer
  printf("%u\n", do_operation(arg1, arg2, operator));
}
