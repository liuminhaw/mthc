#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "file_reader.h"
#include "md_regex.h"

void print_html(MDBlock *block);

int main(int argc, char *argv[]) {
  MDBlock *head_block = NULL;
  MDBlock *tail_block = head_block;
  MDBlock *new_block = NULL;

  FILE *md_file = fopen(argv[1], "r");

  // Read through the file to get all reference links
  PeekReader *reader = new_peek_reader_from_file(md_file, DEFAULT_PEEK_COUNT);
  if (!reader) {
    fprintf(stderr, "Failed to create peek reader\n");
    return 1;
  }
  MDLinkReference *link_ref_head = parse_markdown_links_reference(reader);

  rewind(md_file);
  reader = new_peek_reader_from_file(md_file, DEFAULT_PEEK_COUNT);
  if (!reader) {
    fprintf(stderr, "Failed to create peek reader\n");
    return 1;
  }

  do {
    new_block = block_parsing(NULL, tail_block, reader);

    if (new_block != NULL) {
      printf("block: %d, content: %s\n", new_block->block, new_block->content);

      child_parsing_exec(tail_block);
      inline_parsing(tail_block);

      if (head_block == NULL) {
        head_block = new_block;
        tail_block = head_block;
      } else {
        tail_block->next = new_block;
        tail_block = new_block;
      }
    }
  } while (reader->count > 0);
  fclose(md_file);

  child_parsing_exec(tail_block);
  inline_parsing(tail_block);

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
  } else if (block->type == NONE) {
    printf("%s\n", block->content);
  } else if (block->type == SELF_CLOSING) {
    printf("<%s />\n", block->tag);
  } else if (block->type == BLOCK) {
    printf("<%s>\n", block->tag);
    if (block->content != NULL) {
      printf("%s\n", block->content);
    }
    printf("</%s>\n", block->tag);
  } else {
    fprintf(stderr, "Unknown block type: %d\n", block->type);
  }

  print_html(block->next);
}
