#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "file_reader.h"

void print_html(MDBlock *block);

int main(int argc, char *argv[]) {
  MDBlock *head_block = NULL;
  MDBlock *tail_block = head_block;
  MDBlock *new_block = NULL;

  FILE *md_file = fopen(argv[1], "r");

  char *line;
  while ((line = read_line(md_file, true)) != NULL) {
    printf("line: %s\n", line);

    new_block = block_parsing(NULL, tail_block, line);

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
  } else if (block->block == PLAIN) {
    printf("%s\n", block->content);
  } else {
    printf("<%s>\n", block->tag);
    if (block->content != NULL) {
      printf("%s\n", block->content);
    }
    printf("</%s>\n", block->tag);
  }

  print_html(block->next);
}
