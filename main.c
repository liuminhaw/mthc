#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "file_reader.h"
#include "logger.h"
#include "md_regex.h"
#include "str_utils.h"
#include "style_css.h"

void generate_html(MDBlock *block, const unsigned char *css_theme,
                   unsigned int css_theme_len);
void print_html(MDBlock *block);
int write_css(FILE *fp, const unsigned char *p, size_t n);

static bool debug_mode = false;
static bool test_mode = false;
static bool css_style = true;

static const char *version = "{{VERSION}}";

static void usage(const char *prog_name) {
  fprintf(stdout,
          "Usage: %s [options] <markdown_file>\n"
          "\n"
          "Options: \n"
          "  --help             Show this help message\n"
          "  --output=FILE      Specify output html file (default: stdout)\n"
          "  --no-style         Disable CSS styling in the output HTML\n"
          "  --debug            Enable debug logging\n"
          "  --test             For testing purposes only\n"
          "  --version          Show version information\n",
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
    if (strcmp(argv[i], "--version") == 0) {
      fprintf(stdout, "Version: %s\n", version);
      return 0;
    }
  }

  const char *output_path = NULL;
  const unsigned char *css_theme = default_theme_css;
  unsigned int css_theme_len = default_theme_css_len;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--test") == 0) {
      test_mode = true;
    }
    if (strcmp(argv[i], "--no-style") == 0) {
      css_style = false;
    }
    if (strcmp(argv[i], "--debug") == 0) {
      debug_mode = true;
    }
    if (strncmp(argv[i], "--output=", 9) == 0) {
      output_path = argv[i] + 9;
    }
  }
  log_init(debug_mode);
  LOGF("Debug mode enabled\n");

  if (output_path != NULL) {
    FILE *output_file = freopen(output_path, "w", stdout);
    if (!output_file) {
      fprintf(stderr, "Failed to open output file: %s\n", output_path);
      return 1;
    }
  }

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
  if (test_mode) {
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
  if (css_style) {
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
           "highlight.min.js\"></script>\n");
  }
  printf("</head>\n");
  printf("<body>\n");
  if (css_style) {
    printf("<nav>\n");
    printf(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" fill=\"none\" viewBox=\"0 "
        "0 24 24\" stroke-width=\"1.5\" stroke=\"currentColor\" "
        "class=\"toggle-theme light-theme\">\n");
    printf(
        "<path stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"M12 "
        "3v2.25m6.364.386-1.591 1.591M21 12h-2.25m-.386 6.364-1.591-1.591M12 "
        "18.75V21m-4.773-4.227-1.591 1.591M5.25 12H3m4.227-4.773L5.636 "
        "5.636M15.75 12a3.75 3.75 0 1 1-7.5 0 3.75 3.75 0 0 1 7.5 0Z\" />\n");
    printf("</svg>\n");
    printf(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" fill=\"none\" viewBox=\"0 "
        "0 24 24\" stroke-width=\"1.5\" stroke=\"currentColor\" "
        "class=\"toggle-theme dark-theme\">\n");
    printf(
        "<path stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"M21.752 "
        "15.002A9.72 9.72 0 0 1 18 15.75c-5.385 0-9.75-4.365-9.75-9.75 "
        "0-1.33.266-2.597.748-3.752A9.753 9.753 0 0 0 3 11.25C3 16.635 7.365 "
        "21 12.75 21a9.753 9.753 0 0 0 9.002-5.998Z\" />\n");
    printf("</svg>\n");
    printf("</nav>\n");
  }
  printf("<div class=\"container\">\n");
  print_html(block);
  printf("</div>\n");
  if (css_style) {
    printf("<script>hljs.highlightAll();</script>\n");
    printf("<script>\n");
    printf("(function () {\n");
    printf("const saved = localStorage.getItem('theme');\n");
    printf("const prefersDark = window.matchMedia('(prefers-color-scheme: "
           "light)').matches;\n");
    printf("const theme = saved || (prefersDark ? 'light' : 'dark');\n");
    printf("document.body.classList.toggle('light', theme === 'light');\n");
    printf("document.documentElement.dataset.theme = theme;\n");
    printf("document.documentElement.style.colorScheme = theme;\n");
    printf("const lightThemeBtn = document.querySelector('.light-theme');\n");
    printf("const darkThemeBtn = document.querySelector('.dark-theme');\n");
    printf("if (theme === 'light') {\n");
    printf("lightThemeBtn.classList.add('hidden');\n");
    printf("darkThemeBtn.classList.remove('hidden');\n");
    printf("} else {\n");
    printf("darkThemeBtn.classList.add('hidden');\n");
    printf("lightThemeBtn.classList.remove('hidden');\n");
    printf("}\n");
    printf("})();\n");
    printf("</script>\n");
    printf("<script>\n");
    printf("const themes = document.querySelectorAll('.toggle-theme');\n");
    printf("themes.forEach(theme => {\n");
    printf("theme.addEventListener('click', () => {\n");
    printf("const current = document.body.classList.contains('light') ? "
           "'light' : 'dark';\n");
    printf("const next = current === 'light' ? 'dark' : 'light';\n");
    printf("localStorage.setItem('theme', next);\n");
    printf("document.documentElement.dataset.theme = theme;\n");
    printf("document.documentElement.style.colorScheme = theme;\n");
    printf("document.body.classList.toggle('light');\n");
    printf("themes.forEach(t => t.classList.toggle('hidden'));\n");
    printf("});\n");
    printf("});\n");
    printf("</script>\n");
  }
  printf("</body>\n");
  printf("</html>\n");

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
