// Build a web page that has the following content:
//
// Compile: gcc main.c -o webby
//
// Usage: ./webby -t "Title" -h "Heading" -c "Content..." -c "More content..."

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

/* Returns 1 if the string is empty or only whitespace, otherwise returns 0 */
int is_empty_or_whitespace(const char *str) {
  // Optional: Handle NULL pointers as empty
  if (str == NULL) {
    return 1;
  }

  // Iterate through each character in the string.
  while (*str) {
    if (!isspace((unsigned char)*str)) {
      // Found a non-whitespace character.
      return 0;
    }
    str++;
  }
  // The string is either empty or only contained whitespace.
  return 1;
}

int main(int argc, char *argv[]) {
  char contents[5000];
  char *matched;

  FILE *md_file = fopen(argv[1], "r");

  while (fgets(contents, 5000, md_file) != NULL) {
    if (strchr(contents, '\n') != NULL) {
      strchr(contents, '\n')[0] = '\0';
    }

    // regex matching
    // level 1 heading
    matched = heading_parser(contents, PSR_H1_PATTERN, 2, 1);
    if (matched != NULL) {
      printf("<h1>%s</h1>\n", matched);
    }

    // level 2 heading
    matched = heading_parser(contents, PSR_H2_PATTERN, 2, 1);
    if (matched != NULL) {
      printf("<h2>%s</h2>\n", matched);
    }

    // level 3 heading
    matched = heading_parser(contents, PSR_H3_PATTERN, 2, 1);
    if (matched != NULL) {
      printf("<h3>%s</h3>\n", matched);
    }

    // level 4 heading
    matched = heading_parser(contents, PSR_H4_PATTERN, 2, 1);
    if (matched != NULL) {
      printf("<h4>%s</h4>\n", matched);
    }

    // level 5 heading
    matched = heading_parser(contents, PSR_H5_PATTERN, 2, 1);
    if (matched != NULL) {
      printf("<h5>%s</h5>\n", matched);
    }

    // level 6 heading
    matched = heading_parser(contents, PSR_H6_PATTERN, 2, 1);
    if (matched != NULL) {
      printf("<h6>%s</h6>\n", matched);
    }
  }

  free(matched);
  fclose(md_file);

  return 0;
}
