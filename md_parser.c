#include <ctype.h>
// #include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_reader.h"
#include "md_parser.h"

MDBlock *block_parsing(MDBlock *curr_block, char *target_str) {
  printf("parsing block: %s\n", target_str);
  MDBlock *new_block = NULL;

  new_block = heading_parser(target_str);
  if (new_block != NULL) {
    printf("heading block\n");
    return new_block;
  }

  new_block = blockquote_parser(curr_block, target_str);
  if (new_block != NULL && new_block == curr_block) {
    printf("read content of blockquote block\n");
    return NULL;
  } else if (new_block != NULL) {
    printf("new blockquote block\n");
    return new_block;
  }

  new_block = ordered_list_parser(curr_block, target_str);
  if (new_block != NULL && new_block == curr_block) {
    printf("read content of ordered list block\n");
    return NULL;
  } else if (new_block != NULL) {
    printf("new ordered list block\n");
    return new_block;
  }

  new_block = unordered_list_parser(curr_block, target_str);
  if (new_block != NULL && new_block == curr_block) {
    printf("read content of unordered list block\n");
    return NULL;
  } else if (new_block != NULL) {
    printf("new unordered list block\n");
    return new_block;
  }

  new_block = paragraph_parser(curr_block, target_str);
  if (new_block == curr_block) {
    printf("paragraph block\n");
    return NULL;
  } else if (new_block != NULL) {
    printf("paragraph block\n");
    return new_block;
  }

  new_block = malloc(sizeof(MDBlock));
  new_block->block = SECTION_BREAK;
  new_block->content = NULL;
  new_block->type = INLINE;
  new_block->child = NULL;
  new_block->next = NULL;
  return new_block;
}

MDBlock *child_block_parsing(MDBlock *block) {
  printf("child block parsing content: %s\n", block->content);
  MDBlock *head_block = NULL;
  MDBlock *tail_block = head_block;
  MDBlock *new_block = NULL;

  int line_count = 0;
  char **lines = content_splitter(block->content, '\n', &line_count);
  if (lines == NULL) {
    perror("content_splitter failed");
    return NULL;
  }

  for (int i = 0; i < line_count; i++) {
    new_block = block_parsing(tail_block, lines[i]);
    if (new_block != NULL) {
      if (tail_block != NULL && tail_block->block == BLOCKQUOTE) {
        tail_block->child = child_block_parsing(tail_block);
      }
      if (head_block == NULL) {
        head_block = new_block;
        tail_block = head_block;
      } else {
        tail_block->next = new_block;
        tail_block = new_block;
      }
    }
  }
  if (tail_block != NULL && tail_block->block == BLOCKQUOTE) {
    tail_block->child = child_block_parsing(tail_block);
  }

  return head_block;
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
    char *tag = malloc(3);
    if (tag == NULL) {
      perror("malloc failed");
      return NULL;
    }
    sprintf(tag, "h%d", heading_level);

    MDBlock *new_block = new_mdblock(line, tag, heading_level, BLOCK, 0);

    return new_block;
  }

  return NULL;
}

MDBlock *paragraph_parser(MDBlock *block, char *line) {
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  if (block != NULL && block->block == PARAGRAPH) {
    mdblock_content_update(block, line, "%s %s");

    return block;
  }

  MDBlock *new_block = new_mdblock(line, "p", PARAGRAPH, BLOCK, 0);

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
    return NULL;
  }

  // Removing leading '>' characters and spaces of the blockquote syntax
  line++;
  while (*line && isspace((unsigned char)*line)) {
    line++;
  }

  if (block != NULL && block->block == BLOCKQUOTE) {
    mdblock_content_update(block, line, "%s%s\n");

    return block;
  }

  if (block == NULL || block->block == SECTION_BREAK ||
      is_header_block(*block)) {
    MDBlock *new_block = new_mdblock(line, "blockquote", BLOCKQUOTE, BLOCK, 1);

    return new_block;
  }

  return NULL;
}

MDBlock *ordered_list_parser(MDBlock *block, char *line) {
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  char *line_ptr = line;
  int line_length = strlen(line_ptr);

  if (!isdigit((unsigned char)*line_ptr)) {
    return NULL;
  }

  while (*line_ptr && isdigit((unsigned char)*line_ptr)) {
    line_ptr++;
  }

  if (*line_ptr != '.') {
    return NULL;
  }
  line_ptr++;

  if (!isspace((unsigned char)*line_ptr)) {
    return NULL;
  }

  // Read content in ordered list
  if (block != NULL && block->block == ORDERED_LIST) {
    mdblock_content_update(block, line, "%s%s\n");

    return block;
  }

  if (block == NULL || block->block != ORDERED_LIST) {
    char *line_ptr = line;

    if (!*line_ptr || *line_ptr != '1') {
      return NULL;
    }
    line_ptr++;
    if (*line_ptr != '.') {
      return NULL;
    }
    line_ptr++;
    if (!isspace((unsigned char)*line_ptr)) {
      return NULL;
    }

    MDBlock *new_block = new_mdblock(line, "ol", ORDERED_LIST, BLOCK, 1);

    return new_block;
  }

  return NULL;
}

MDBlock *unordered_list_parser(MDBlock *block, char *line) {
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  char *line_ptr = line;
  int line_length = strlen(line_ptr);

  if (!is_unordered_list_syntax(line)) {
    printf("init check: not unordered list syntax\n");
    return NULL;
  }

  if (block != NULL && block->block == UNORDERED_LIST) {
    mdblock_content_update(block, line, "%s%s\n");

    return block;
  }

  if (block == NULL || block->block != UNORDERED_LIST) {
    char *line_ptr = line;

    if (!is_unordered_list_syntax(line)) {
      printf("check: not unordered list syntax\n");
      return NULL;
    }

    MDBlock *new_block = new_mdblock(line, "ul", UNORDERED_LIST, BLOCK, 1);

    return new_block;
  }

  return NULL;
}

MDBlock *new_mdblock(char *content, char *html_tag, BlockTag block_tag,
                     TagType type, int content_newline) {
  MDBlock *block = malloc(sizeof(MDBlock));
  if (!block) {
    perror("malloc failed");
    return NULL;
  }

  size_t len = strlen(content);
  if (content_newline) {
    len += 2; // +1 for '\n' and +1 for '\0'
  } else {
    len += 1; // +1 for '\0'
  }
  char *b_content = malloc(len);
  if (!b_content) {
    perror("malloc failed");
    free(block);
    return NULL;
  }
  if (content_newline) {
    sprintf(b_content, "%s\n", content);
  } else {
    strcpy(b_content, content);
  }

  block->content = b_content;
  block->tag = html_tag;
  block->block = block_tag;
  block->type = type;
  block->child = NULL;
  block->next = NULL;

  return block;
}

bool is_header_block(MDBlock block) {
  return 1 <= block.block && block.block <= 6;
}

bool is_unordered_list_syntax(char *str) {
  if (str == NULL || strlen(str) < 2) {
    return 0;
  }

  if (strncmp(str, "- ", 2) == 0 || strncmp(str, "* ", 2) == 0 ||
      strncmp(str, "+ ", 2) == 0) {
    return 1;
  }

  return 0;
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

void mdblock_content_update(MDBlock *block, char *content, char *formatter) {
  size_t len = strlen(block->content) + strlen(content) + 2;
  char *paragraph = malloc(len);
  if (!paragraph) {
    perror("malloc failed");
    return;
  }

  sprintf(paragraph, formatter, block->content, content);
  free(block->content);
  block->content = paragraph;
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
