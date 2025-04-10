#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "file_reader.h"

void ltrim_space(char *str);
void print_html(MDBlock *block);
void child_parsing_exec(MDBlock *block);

int main(int argc, char *argv[]) {
  MDBlock *head_block = NULL;
  MDBlock *tail_block = head_block;
  MDBlock *new_block = NULL;

  FILE *md_file = fopen(argv[1], "r");

  char *line;
  while ((line = read_line(md_file, true)) != NULL) {
    printf("line: %s\n", line);

    new_block = block_parsing(tail_block, line);

    if (new_block != NULL) {
      printf("block: %d, content: %s\n", new_block->block, new_block->content);

      child_parsing_exec(tail_block);

      if (head_block == NULL) {
        head_block = new_block;
        tail_block = head_block;
      } else {
        tail_block->next = new_block;
        tail_block = new_block;
      }
    }

    free(line);
  }
  fclose(md_file);

  child_parsing_exec(tail_block);

  // Traverse block list
  printf("\n=== Traverse block list ===\n");
  traverse_block(head_block);

  // Generate HTML
  printf("\n=== Generate HTML ===\n");
  print_html(head_block);

  return 0;
}

void child_parsing_exec(MDBlock *block) {
  if (block) {
    switch (block->block) {
    case BLOCKQUOTE:
    case ORDERED_LIST:
    case UNORDERED_LIST:
      block->child = child_block_parsing(block);
      break;
    default:
      break;
    }
  }
}

void print_html(MDBlock *block) {
  if (block == NULL) {
    return;
  }

  if (block->block == SECTION_BREAK) {
    print_html(block->next);
    return;
  }

  if (block->child != NULL) {
    printf("<%s>\n", block->tag);
    print_html(block->child);
    printf("</%s>\n", block->tag);
  } else {
    printf("<%s>\n", block->tag);
    printf("%s\n", block->content);
    printf("</%s>\n", block->tag);
  }

  print_html(block->next);
}

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
