#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "file_reader.h"
#include "md_regex.h"

void generate_html(MDBlock *block);
void print_html(MDBlock *block);

int main(int argc, char *argv[]) {
  bool check_flag = false;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--check") == 0) {
      check_flag = true;
    }
  }

  MDBlock *head_block = NULL;
  MDBlock *tail_block = head_block;
  MDBlock *new_block = NULL;

  FILE *md_file = fopen(argv[argc - 1], "r");

  // Read through the file to get all reference links
  PeekReader *reader = new_peek_reader_from_file(md_file, DEFAULT_PEEK_COUNT);
  if (!reader) {
    fprintf(stderr, "Failed to create peek reader\n");
    return 1;
  }
  MDLinkReference *link_ref_head = gen_markdown_link_reference_list(reader);

  rewind(md_file);
  reader = new_peek_reader_from_file(md_file, DEFAULT_PEEK_COUNT);
  if (!reader) {
    fprintf(stderr, "Failed to create peek reader\n");
    return 1;
  }

  do {
    new_block = block_parsing(NULL, tail_block, reader, link_ref_head);

    if (new_block != NULL) {
      fprintf(stderr, "block: %d, content: %s\n", new_block->block,
              new_block->content);

      child_parsing_exec(link_ref_head, tail_block);
      inline_parsing(link_ref_head, tail_block);

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

  child_parsing_exec(link_ref_head, tail_block);
  inline_parsing(link_ref_head, tail_block);

  // Traverse block list
  fprintf(stderr, "\n=== Traverse block list ===\n");
  traverse_block(head_block);

  // Generate HTML
  fprintf(stderr, "\n=== Generate HTML ===\n");
  if (check_flag) {
    print_html(head_block);
  } else {
    generate_html(head_block);
  }

  return 0;
}

void generate_html(MDBlock *block) {
  if (block == NULL) {
    return;
  }

  FILE *css_file = fopen("css/catppuccin-mocha.css", "r");
  PeekReader *reader = new_peek_reader_from_file(css_file, DEFAULT_PEEK_COUNT);
  if (!reader) {
    fprintf(stderr, "Failed to create peek reader for CSS\n");
    return;
  }

  printf("<!DOCTYPE html>\n");
  printf("<html>\n");
  printf("<head>\n");
  printf("<meta charset=\"UTF-8\">\n");
  printf("<title>Placeholder title</title>\n");
  printf("<meta name=\"viewport\" content=\"width=device-width, "
         "initial-scale=1.0\">\n");
  // printf("<link rel=\"stylesheet\" href=\"css/catppuccin-mocha.css\">\n");
  printf("<style>\n");
  do {
    char *line = peek_reader_current(reader);
    if (line != NULL) {
      printf("%s\n", line);
    }
    if (!peek_reader_advance(reader)) {
      break; // Exit loop if no more lines to read
    }
  } while (reader->count > 0);
  printf("</style>\n");
  printf("</head>\n");
  printf("<body>\n");
  printf("<div class=\"container\">\n");
  print_html(block);
  printf("</div>\n");
  printf("</body>\n");
}

void print_html(MDBlock *block) {
  if (block == NULL) {
    return;
  }

  if (block->block == SECTION_BREAK || block->block == LINK_REFERENCE) {
    print_html(block->next);
    return;
  }

  if (block->child != NULL) {
    printf("<%s>\n", block->tag);
    print_html(block->child);
    printf("</%s>\n", block->tag);
  } else if (block->type == NONE) {
    escape_char_parsing(block->content);
    printf("%s\n", block->content);
  } else if (block->type == SELF_CLOSING) {
    printf("<%s />\n", block->tag);
  } else if (block->type == BLOCK) {
    printf("<%s>\n", block->tag);
    if (block->content != NULL) {
      if (block->block != CODEBLOCK) {
        escape_char_parsing(block->content);
      }
      printf("%s\n", block->content);
    }
    printf("</%s>\n", block->tag);
  } else {
    fprintf(stderr, "Unknown block type: %d\n", block->type);
  }

  print_html(block->next);
}
