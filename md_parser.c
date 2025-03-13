#include <ctype.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md_parser.h"

MDBlock *block_parsing(MDBlock *curr_block, char *target_str) {
  MDBlock *new_block = NULL;

  new_block = heading_parser(target_str);
  if (new_block != NULL) {
    return new_block;
  }

  new_block = blockquote_parser(curr_block, target_str);
  if (new_block == curr_block) {
    return NULL;
  } else if (new_block != NULL) {
    return new_block;
  }

  new_block = paragraph_parser(curr_block, target_str);
  if (new_block == curr_block) {
    return NULL;
  } else if (new_block != NULL) {
    return new_block;
  }

  new_block = malloc(sizeof(MDBlock));
  new_block->block = SECTION_BREAK;
  new_block->content = NULL;
  new_block->type = INLINE;
  return new_block;
}

MDBlock *heading_parser(char *line) {
  int line_length = strlen(line);
  int heading_level = 0;

  while (*line == '#') {
    heading_level++;
    line++;
  }

  // Make sure there is a space after header hash marks for it to be a header
  if (!isspace((unsigned char)*line)) {
    return NULL;
  }

  while (*line && isspace((unsigned char)*line)) {
    line++;
  }

  if (heading_level > 0 && heading_level < 7) {
    char *content = strdup(line);
    if (content == NULL) {
      perror("strdup failed");
      return NULL;
    }

    char *tag = malloc(3);
    if (tag == NULL) {
      perror("malloc failed");
      return NULL;
    }
    sprintf(tag, "h%d", heading_level);

    MDBlock *new_block = malloc(sizeof(MDBlock));
    if (new_block == NULL) {
      perror("malloc failed");
      return NULL;
    }

    new_block->content = content;
    new_block->tag = tag;
    new_block->block = heading_level;
    new_block->type = BLOCK;
    new_block->child = NULL;
    new_block->next = NULL;
    return new_block;
  }

  return NULL;
}

MDBlock *paragraph_parser(MDBlock *block, char *line) {
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  if (block != NULL && block->block == PARAGRAPH) {
    size_t len = strlen(block->content) + strlen(line) + 2;
    char *paragraph = malloc(len);
    if (!paragraph) {
      perror("malloc failed");
      return NULL;
    }

    sprintf(paragraph, "%s %s", block->content, line);
    free(block->content);
    block->content = paragraph;

    return block;
  }

  MDBlock *new_block = malloc(sizeof(MDBlock));
  if (new_block == NULL) {
    perror("malloc failed");
    return NULL;
  }

  size_t len = strlen(line);
  char *paragraph = malloc(len + 1);
  if (!paragraph) {
    perror("malloc failed");
    return NULL;
  }

  strcpy(paragraph, line);
  new_block->content = paragraph;
  new_block->tag = "p";
  new_block->block = PARAGRAPH;
  new_block->type = BLOCK;
  new_block->child = NULL;
  new_block->next = NULL;
  return new_block;
}

MDBlock *blockquote_parser(MDBlock *block, char *line) {
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  char *line_ptr = line;
  int line_length = strlen(line_ptr);
  while (*line_ptr == '>') {
    line_ptr++;
  }

  if (!isspace((unsigned char)*line_ptr) && *line_ptr != '\0') {
    printf("No space or newline after blockquote\n");
    return NULL;
  }

  // Removing leading '>' characters and spaces of the blockquote syntax
  line++;
  while (*line && isspace((unsigned char)*line)) {
    line++;
  }

  if (block != NULL && block->block == BLOCKQUOTE) {
    // TODO: Already in blockquote, append to the content
    printf("Appending to blockquote block\n");
    size_t len = strlen(block->content) + strlen(line) + 2;
    char *content = malloc(len);
    if (!content) {
      perror("malloc failed");
      return NULL;
    }

    sprintf(content, "%s%s\n", block->content, line);
    free(block->content);
    block->content = content;

    return block;
  }

  if (block == NULL || block->block == SECTION_BREAK || is_header_block(*block)) {
    // TODO: Create a new blockquote block
    printf("Creating new blockquote block\n");
    MDBlock *new_block = malloc(sizeof(MDBlock));
    if (new_block == NULL) {
      perror("malloc failed");
      return NULL;
    }

    size_t len = strlen(line);
    char *content = malloc(len + 2);
    if (!content) {
      perror("malloc failed");
      return NULL;
    }

    sprintf(content, "%s\n", line);
    new_block->content = content;
    new_block->tag = "blockquote";
    new_block->block = BLOCKQUOTE;
    new_block->type = BLOCK;
    new_block->child = NULL;
    new_block->next = NULL;
    return new_block;
  }

  return NULL;
}

bool is_header_block(MDBlock block) {
  return 1 <= block.block && block.block <= 6;
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

char *blocktag_to_string(BlockTag block) {
  switch (block) {
  case H1:
    return "H1";
  case H2:
    return "H2";
  case H3:
    return "H3";
  case H4:
    return "H4";
  case H5:
    return "H5";
  case H6:
    return "H6";
  case PARAGRAPH:
    return "PARAGRAPH";
  case SECTION_BREAK:
    return "SECTION BREAK";
  case BLOCKQUOTE:
    return "BLOCKQUOTE";
  default:
    return "INVALID";
  }
}

char *tagtype_to_string(TagType type) {
  switch (type) {
  case BLOCK:
    return "BLOCK";
  case INLINE:
    return "INLINE";
  default:
    return "INVALID";
  }
}
