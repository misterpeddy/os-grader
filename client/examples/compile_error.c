#include <stdio.h>

int main() {
  printf("$");
  fgets(buffer, 128, stdin);
  printf("(%d)[%s]\n", strlen(buffer), buffer);
  fgets(buffer, 128, stdin);
  printf("(%d)[%s]\n", strlen(buffer), buffer);
}

