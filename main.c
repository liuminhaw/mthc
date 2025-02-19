#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "file_reader.h"
#include "md_parser.h"

void ltrim_space(char *str) {
  char *p = str;
  // skip leading whitespace
  while (*p && isspace((unsigned char)*p)) {
    p++;
  }
  // move if there are leading spaces
  if (p != str) {
    memmove(str, p, strlen(p) + 1);
  }
}

int main(int argc, char *argv[]) {
  MDBlock *head_block = NULL;
  MDBlock *tail_block = head_block;
  MDBlock *new_block = NULL;

  FILE *md_file = fopen(argv[1], "r");

  char *line;
  while ((line = read_line(md_file, true)) != NULL) {
    // if (strchr(contents, '\n') != NULL) {
    //   strchr(contents, '\n')[0] = '\0';
    // }
    printf("line: %s\n", line);

    new_block = block_parsing(tail_block, line);
    if (new_block != NULL) {
      printf("type: %d, content: %s\n", new_block->type, new_block->content);
      if (head_block == NULL) {
        head_block = new_block;
        tail_block = head_block;
      }
      tail_block->next = new_block;
      tail_block = new_block;
    }
    free(line);
  }
  // free(matched);
  fclose(md_file);

  // Traverse block list
  printf("\n=== Traverse block list ===\n");
  MDBlock *curr_block = head_block;
  while (curr_block != NULL) {
    printf("type: %d, content: %s\n", curr_block->type, curr_block->content);
    curr_block = curr_block->next;
  }

  return 0;
}
