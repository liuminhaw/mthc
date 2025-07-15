#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

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

void traverse_block(MDBlock *block) {
  if (block == NULL) {
    return;
  }

  char *btag = blocktag_to_string(block->block);
  char *ttype = tagtype_to_string(block->type);
  char *sub_content = literal_newline_substitution(block->content);
  if (sub_content == NULL && block->block != SECTION_BREAK &&
      block->block != HORIZONTAL_LINE) {
    perror("literal_newline_substitution failed");
    return;
  }

  if (block->block == BLOCKQUOTE) {
    fprintf(stderr, "block: %s, type: %s, tag: %s\n", btag, ttype, block->tag);
  } else {
    fprintf(stderr, "block: %s, type: %s, tag: %s, content: %s\n", btag, ttype,
            block->tag, sub_content);
  }
  if (block->child != NULL) {
    fprintf(stderr, "traveling to child block\n");
  }
  traverse_block(block->child);
  // printf("traveling to next block\n");
  traverse_block(block->next);
  // printf("traverse block return\n");
}
