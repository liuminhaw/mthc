#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Substitutes newline character with '\n' literal for debugging recognition of 
// the newline character
char *literal_newline_substitution(char *str) {
  if (str == NULL) {
    return NULL;
  }

  int newline_count = 0;
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == '\n') {
      newline_count++;
    }
  }

  size_t len = strlen(str);
  size_t new_len = len + newline_count;
  char *new_str = malloc(new_len + 1);
  if (new_str == NULL) {
    perror("malloc failed");
    return NULL;
  }

  for (size_t i = 0, j = 0; i < len; i++) {
    if (str[i] == '\n') {
      new_str[j++] = '\\';
      new_str[j++] = 'n';
    } else {
      new_str[j++] = str[i];
    }
  }
  new_str[new_len] = '\0';

  return new_str;
}
