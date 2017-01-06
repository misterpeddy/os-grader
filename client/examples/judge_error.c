#include <stdio.h>

int main() {
  char buffer[128];
  printf("$");
  fgets(buffer, 128, stdin);
  printf("(%d)[%s]\n", strlen(buffer), buffer);
  fgets(buffer, 128, stdin);
  printf("(%d)[%s] \n", strlen(buffer), buffer);
}

