#include <ctype.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md_parser.h"

MDBlock *block_parsing(MDBlock *curr_block, char *target_str) {
  // printf("start block parsing\n");
  char *matched;
  MDBlock *new_block = malloc(sizeof(MDBlock));
  new_block->next = NULL;

  // level 1 heading
  matched = heading_parser(target_str, PSR_H1_PATTERN, 2, 1);
  // printf("level 1 heading\n");
  if (matched != NULL) {
    new_block->content = matched;
    new_block->type = H1;
    return new_block;
  }

  // level 2 heading
  matched = heading_parser(target_str, PSR_H2_PATTERN, 2, 1);
  if (matched != NULL) {
    new_block->content = matched;
    new_block->type = H2;
    return new_block;
  }

  // level 3 heading
  matched = heading_parser(target_str, PSR_H3_PATTERN, 2, 1);
  if (matched != NULL) {
    new_block->content = matched;
    new_block->type = H3;
    return new_block;
  }

  // level 4 heading
  matched = heading_parser(target_str, PSR_H4_PATTERN, 2, 1);
  if (matched != NULL) {
    new_block->content = matched;
    new_block->type = H4;
    return new_block;
  }

  // level 5 heading
  matched = heading_parser(target_str, PSR_H5_PATTERN, 2, 1);
  if (matched != NULL) {
    new_block->content = matched;
    new_block->type = H5;
    return new_block;
  }

  // level 6 heading
  matched = heading_parser(target_str, PSR_H6_PATTERN, 2, 1);
  if (matched != NULL) {
    new_block->content = matched;
    new_block->type = H6;
    return new_block;
  }

  // paragraph
  // printf("paragraph\n");
  matched = read_paragraph(curr_block, target_str);
  if (matched != NULL && curr_block != NULL && curr_block->type == PARAGRAPH) {
    free(new_block);
    free(curr_block->content);
    curr_block->content = matched;
    return NULL;
  } else if (matched != NULL) {
    new_block->content = matched;
    new_block->type = PARAGRAPH;
    return new_block;
  }

  new_block->type = SECTION_BREAK;
  new_block->content = NULL;
  return new_block;

  // free(new_block);
  // return NULL;
}

char *heading_parser(char *target_str, char *pattern, int match_count,
                     int target_match) {
  regex_t regex;
  // const char *pattern = "^#\\s+(.*)$";
  // Index 0 : whole match; Index 1 : first group
  regmatch_t matches[match_count];

  if (regcomp(&regex, pattern, REG_EXTENDED)) {
    fprintf(stderr, "Could not compile regex\n");
    return NULL;
  }

  int ret = regexec(&regex, target_str, match_count, matches, 0);
  if (!ret) {
    if (matches[target_match].rm_so != -1) {
      int start = matches[target_match].rm_so;
      int end = matches[target_match].rm_eo;
      int len = end - start;

      char *captured = malloc(len + 1);
      if (!captured) {
        fprintf(stderr, "malloc failed\n");
        // perror("malloc");
        return NULL;
      }

      strncpy(captured, target_str + start, len);
      captured[len] = '\0';

      regfree(&regex);
      return captured;
    }
  } else if (ret == REG_NOMATCH) {
    return NULL;
  } else {
    char error_message[100];
    regerror(ret, &regex, error_message, sizeof(error_message));
    fprintf(stderr, "Regex match failed: %s\n", error_message);
    return NULL;
  }

  regfree(&regex);
  return NULL;
}

char *read_paragraph(MDBlock *block, char *line) {
  // printf("read_paragraph\n");
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  if (block != NULL && block->type == PARAGRAPH) {
    size_t len = strlen(block->content) + strlen(line) + 2;
    char *paragraph = malloc(len);
    if (!paragraph) {
      perror("malloc failed");
      return NULL;
    }

    sprintf(paragraph, "%s %s", block->content, line);
    return paragraph;
  }

  size_t len = strlen(line);
  char *paragraph = malloc(len + 1);
  if (!paragraph) {
    perror("malloc failed");
    return NULL;
  }

  strcpy(paragraph, line);
  return paragraph;
}

/* Returns 1 if the string is empty or only whitespace, otherwise returns 0 */
bool is_empty_or_whitespace(const char *str) {
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
