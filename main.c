#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "file_reader.h"
#include "logger.h"
#include "md_regex.h"
#include "str_utils.h"
#include "style_css.h"

// void generate_html(MDBlock *block, char *css_filepath);
void generate_html(MDBlock *block, const unsigned char *css_theme,
                   unsigned int css_theme_len);
void print_html(MDBlock *block);
// void write_css(int fd, const unsigned char *p, size_t n);
int write_css(FILE *fp, const unsigned char *p, size_t n);

static void usage(const char *prog_name) {
  fprintf(stdout,
          "Usage: %s [options] <markdown_file>\n"
          "\n"
          "Options: \n"
          "  --help        Show this help message and exit\n"
          "  --dark        Use dark theme for CSS\n"
          "  --debug       Enable debug logging\n"
          "  --test        For testing purposes only\n",
          prog_name);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return 0;
    }
  }

  bool test_flag = false;
  bool debug_flag = false;
  // char *css_theme = "css/catppuccin-latte.css";
  const unsigned char *css_theme = default_light_css;
  unsigned int css_theme_len = default_light_css_len;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--test") == 0) {
      test_flag = true;
    }
    if (strcmp(argv[i], "--dark") == 0) {
      css_theme = default_dark_css;
      css_theme_len = default_light_css_len;
      // css_theme = "css/catppuccin-mocha.css";
    }
    if (strcmp(argv[i], "--debug") == 0) {
      debug_flag = true;
    }
  }
  log_init(debug_flag);
  LOGF("Debug mode enabled\n");

  // FILE *css_file = fopen(css_theme, "r");
  // if (!css_file) {
  //   fprintf(stderr, "Failed to open CSS file: %s\n", css_theme);
  //   return 1;
  // } else {
  //   fclose(css_file);
  // }

  MDBlock *head_block = NULL;
  MDBlock *tail_block = head_block;
  MDBlock *new_block = NULL;

  FILE *md_file = fopen(argv[argc - 1], "r");
  if (!md_file) {
    fprintf(stderr, "Failed to open file: %s\n", argv[argc - 1]);
    return 1;
  }

  // Read through the file to get all reference links
  PeekReader *reader = new_peek_reader_from_file(md_file, DEFAULT_PEEK_COUNT);
  if (!reader) {
    fprintf(stderr, "Failed to create peek reader\n");
    return 1;
  }
  MDLinkReference *link_ref_head = gen_markdown_link_reference_list(reader);
  free_peek_reader(reader);

  rewind(md_file);
  reader = new_peek_reader_from_file(md_file, DEFAULT_PEEK_COUNT);
  if (!reader) {
    fprintf(stderr, "Failed to create peek reader\n");
    return 1;
  }

  do {
    new_block = block_parsing(NULL, tail_block, reader, link_ref_head);

    if (new_block != NULL) {
      LOGF("block: %d, content: %s\n", new_block->block, new_block->content);

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

  free_md_link_reference(link_ref_head);

  // Traverse block list
  LOGF("\n=== Traverse block list ===\n");
  traverse_block(head_block);

  // Generate HTML
  LOGF("\n=== Generate HTML ===\n");
  if (test_flag) {
    print_html(head_block);
  } else {
    generate_html(head_block, css_theme, css_theme_len);
  }

  free_mdblocks(head_block);
  free_peek_reader(reader);

  return 0;
}

// void generate_html(MDBlock *block, char *css_filepath) {
void generate_html(MDBlock *block, const unsigned char *css_theme,
                   unsigned int css_theme_len) {
  if (block == NULL) {
    return;
  }

  // FILE *css_file = fopen(css_filepath, "r");
  // PeekReader *reader = new_peek_reader_from_file(css_file,
  // DEFAULT_PEEK_COUNT); if (!reader) {
  //   fprintf(stderr, "Failed to create peek reader for CSS\n");
  //   return;
  // }

  printf("<!DOCTYPE html>\n");
  printf("<html>\n");
  printf("<head>\n");
  printf("<meta charset=\"UTF-8\">\n");
  printf("<title>Placeholder title</title>\n");
  printf("<meta name=\"viewport\" content=\"width=device-width, "
         "initial-scale=1.0\">\n");
  printf("<style>\n");
  // write_css(fileno(stdout), css_theme, (size_t)css_theme_len);
  write_css(stdout, css_theme, (size_t)css_theme_len);
  // do {
  //   char *line = peek_reader_current(reader);
  //   if (line != NULL) {
  //     printf("%s\n", line);
  //   }
  //   if (!peek_reader_advance(reader)) {
  //     break; // Exit loop if no more lines to read
  //   }
  // } while (reader->count > 0);
  printf("</style>\n");
  printf("<script "
         "src=\"https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.11.1/"
         "highlight.min.js\"></script>");
  printf("</head>\n");
  printf("<body>\n");
  printf("<div class=\"container\">\n");
  print_html(block);
  printf("</div>\n");
  printf("<script>hljs.highlightAll();</script>\n");
  printf("</body>\n");

  // free_peek_reader(reader);
  // fclose(css_file);
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
    char *heading_id;
    switch (block->block) {
    case H1:
    case H2:
    case H3:
    case H4:
    case H5:
    case H6:
      heading_id = convert_id_tag(block->content);
      printf("<%s id=\"%s\">\n", block->tag, heading_id);
      free(heading_id);
      break;
    default:
      printf("<%s>\n", block->tag);
    }
    if (block->content != NULL) {
      if (block->block != CODEBLOCK) {
        escape_char_parsing(block->content);
      }
      if (block->block == CODEBLOCK) {
        printf("<code>\n");
      }
      printf("%s\n", block->content);
      if (block->block == CODEBLOCK) {
        printf("</code>\n");
      }
    }
    printf("</%s>\n", block->tag);
  } else {
    LOGF("Unknown block type: %d\n", block->type);
  }

  print_html(block->next);
}

int write_css(FILE *fp, const unsigned char *p, size_t n) {
  size_t remaining = n;

  while (remaining > 0) {
    size_t written = fwrite(p, 1, remaining, fp);
    if (written == 0) {
      return -1;
    }
    p += written;
    remaining -= written;
  }
  return 0;
}
