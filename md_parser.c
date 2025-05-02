#include <ctype.h>
// #include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_reader.h"
#include "md_parser.h"

static const int INDENT_SIZE = 4;

const int parsers_count = 8;
Parsers parsers[] = {{heading_parser, 0},      {blockquote_parser, 1},
                     {ordered_list_parser, 1}, {unordered_list_parser, 1},
                     {codeblock_parser, 1},    {plain_parser, 0},
                     {paragraph_parser, 1},    {section_break_parser, 0}};

MDBlock *block_parsing(MDBlock *prnt_block, MDBlock *curr_block,
                       char *target_str) {
  printf("parsing block: %s\n", target_str);
  MDBlock *new_block = NULL;

  for (int i = 0; i < parsers_count; i++) {
    new_block = parsers[i].parser(prnt_block, curr_block, target_str);
    if (parsers[i].multiline) {
      if (new_block != NULL && new_block == curr_block) {
        return NULL;
      }
    }
    if (new_block != NULL) {
      if (new_block != curr_block && curr_block != NULL) {
        child_parsing_exec(curr_block);
      }
      return new_block;
    }
  }

  return NULL;
}

MDBlock *content_block_parsing(MDBlock *prnt_block, MDBlock *curr_block,
                               char *target_str) {
  MDBlock *new_block = list_item_parser(prnt_block, curr_block, target_str);
  if (new_block != NULL) {
    printf("list item block\n");
    if (strchr(new_block->content, '\n') != NULL) {
      printf("parse list item child block\n");
      new_block->child = child_block_parsing(new_block);
    }
    return new_block;
  }

  new_block = block_parsing(prnt_block, curr_block, target_str);
  return new_block;
}

void child_parsing_exec(MDBlock *block) {
  if (block) {
    switch (block->block) {
    case BLOCKQUOTE:
    case ORDERED_LIST:
    case UNORDERED_LIST:
      block->child = child_block_parsing(block);
      break;
    default:
      break;
    }
  }
}

MDBlock *child_block_parsing(MDBlock *prnt_block) {
  printf("child block parsing content: %s\n", prnt_block->content);
  MDBlock *head_block = NULL;
  MDBlock *tail_block = head_block;
  MDBlock *new_block = NULL;

  int line_count = 0;
  char **lines = content_splitter(prnt_block->content, '\n', &line_count);
  if (lines == NULL) {
    perror("content_splitter failed");
    return NULL;
  }

  for (int i = 0; i < line_count; i++) {
    new_block = content_block_parsing(prnt_block, tail_block, lines[i]);
    if (new_block != NULL) {
      if (new_block == tail_block) {
        continue;
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
  child_parsing_exec(tail_block);

  return head_block;
}

MDBlock *heading_parser(MDBlock *prnt_block, MDBlock *curr_block, char *line) {
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

MDBlock *paragraph_parser(MDBlock *prnt_block, MDBlock *curr_block,
                          char *line) {
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  if (curr_block != NULL && curr_block->block == PARAGRAPH) {
    mdblock_content_update(curr_block, line, "%s %s");

    return curr_block;
  }

  MDBlock *new_block = new_mdblock(line, "p", PARAGRAPH, BLOCK, 0);

  return new_block;
}

MDBlock *blockquote_parser(MDBlock *prnt_block, MDBlock *curr_block,
                           char *line) {
  if (is_empty_or_whitespace(line) || *line != '>') {
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

  if (curr_block != NULL && curr_block->block == BLOCKQUOTE) {
    mdblock_content_update(curr_block, line, "%s%s\n");

    return curr_block;
  }

  if (curr_block == NULL || curr_block->block == SECTION_BREAK ||
      is_header_block(*curr_block) || curr_block->block == LIST_ITEM ||
      curr_block->block == PLAIN) {
    MDBlock *new_block = new_mdblock(line, "blockquote", BLOCKQUOTE, BLOCK, 1);

    return new_block;
  }

  return NULL;
}

MDBlock *ordered_list_parser(MDBlock *prnt_block, MDBlock *curr_block,
                             char *line) {
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  char *line_ptr = line;
  int line_length = strlen(line_ptr);

  // Read content in ordered list
  if (curr_block != NULL && curr_block->block == ORDERED_LIST) {
    mdblock_content_update(curr_block, line, "%s%s\n");

    return curr_block;
  }

  if (curr_block == NULL || curr_block->block != ORDERED_LIST) {
    if (!is_ordered_list_syntax(line, 1)) {
      return NULL;
    }

    MDBlock *new_block = new_mdblock(line, "ol", ORDERED_LIST, BLOCK, 1);

    return new_block;
  }

  return NULL;
}

MDBlock *unordered_list_parser(MDBlock *prnt_block, MDBlock *curr_block,
                               char *line) {
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  char *line_ptr = line;
  int line_length = strlen(line_ptr);

  if (curr_block != NULL && curr_block->block == UNORDERED_LIST) {
    mdblock_content_update(curr_block, line, "%s%s\n");

    return curr_block;
  }

  if (curr_block == NULL || curr_block->block != UNORDERED_LIST) {
    if (!is_unordered_list_syntax(line)) {
      // printf("check: not unordered list syntax\n");
      return NULL;
    }

    MDBlock *new_block = new_mdblock(line, "ul", UNORDERED_LIST, BLOCK, 1);

    return new_block;
  }

  return NULL;
}

MDBlock *list_item_parser(MDBlock *prnt_block, MDBlock *prev_block,
                          char *line) {
  printf("list item parsing: %s\n", line);
  printf("parent block: %d\n", prnt_block->block);

  if (is_empty_or_whitespace(line) || prnt_block == NULL) {
    return NULL;
  }

  int offset = 0;
  switch (prnt_block->block) {
  case ORDERED_LIST:
    offset = is_ordered_list_syntax(line, 0);
    break;
  case UNORDERED_LIST:
    offset = is_unordered_list_syntax(line);
    break;
  default:
    offset = 0;
  }

  if (!offset) {
    if (prev_block != NULL && prev_block->block == LIST_ITEM) {
      if (is_indented_line(INDENT_SIZE, line)) {
        char *line_ptr = line + INDENT_SIZE;
        mdblock_content_update(prev_block, line_ptr, "%s\n%s");
      } else {
        mdblock_content_update(prev_block, line, "%s %s");
      }
      return prev_block;
    }
    return NULL;
  }

  char *line_ptr = line + offset;
  MDBlock *new_block = new_mdblock(line_ptr, "li", LIST_ITEM, BLOCK, 0);

  return new_block;
}

MDBlock *codeblock_parser(MDBlock *prnt_block, MDBlock *curr_block,
                          char *line) {
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  char *line_ptr = line;
  if (is_indented_line(INDENT_SIZE, line)) {
    printf("indented line\n");
    line_ptr += INDENT_SIZE;
  } else if (is_indented_tab(line)) {
    printf("indented tab\n");
    line_ptr++;
  } else {
    return NULL;
  }

  if (curr_block != NULL && curr_block->block == CODEBLOCK) {
    printf("code block content update\n");
    mdblock_content_update(curr_block, line_ptr, "%s\n%s");
    return curr_block;
  }

  if (curr_block == NULL || curr_block->block != CODEBLOCK) {
    printf("new code block\n");
    MDBlock *new_block = new_mdblock(line_ptr, "pre", CODEBLOCK, BLOCK, 0);
    return new_block;
  }

  return NULL;
}

MDBlock *plain_parser(MDBlock *prnt_block, MDBlock *curr_block, char *line) {
  if (is_empty_or_whitespace(line) || prnt_block == NULL) {
    return NULL;
  }

  if (prnt_block->block != LIST_ITEM || curr_block != NULL) {
    return NULL;
  }

  MDBlock *new_block = new_mdblock(line, "", PLAIN, INLINE, 0);

  return new_block;
}

MDBlock *section_break_parser(MDBlock *prnt_block, MDBlock *curr_block,
                              char *line) {
  if (!is_empty_or_whitespace(line)) {
    printf("section: not empty or whitespace\n");
    return NULL;
  }

  printf("new section break\n");
  MDBlock *new_block = new_mdblock(NULL, NULL, SECTION_BREAK, INLINE, 0);

  return new_block;
}

MDBlock *new_mdblock(char *content, char *html_tag, BlockTag block_tag,
                     TagType type, int content_newline) {
  MDBlock *block = malloc(sizeof(MDBlock));
  if (!block) {
    perror("malloc failed");
    return NULL;
  }

  char *b_content = NULL;
  if (content != NULL) {
    size_t len = strlen(content);
    if (content_newline) {
      len += 2; // +1 for '\n' and +1 for '\0'
    } else {
      len += 1; // +1 for '\0'
    }
    b_content = malloc(len);
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

// Returned int is the number of characters in the prefix of the list item
int is_ordered_list_syntax(char *str, int first_item) {
  if (str == NULL || strlen(str) < 3 || !isdigit((unsigned char)*str)) {
    return 0;
  }

  int prefix_count = 0;
  // If first_item is true, need to check if content starts with "1. "
  if (first_item) {
    if (strncmp(str, "1. ", 3) == 0) {
      return 3;
    } else {
      return 0;
    }
  }

  while (isdigit((unsigned char)*str)) {
    str++;
    prefix_count++;
  }

  if (*str != '.') {
    return 0;
  }
  str++;
  prefix_count++;

  if (!isspace((unsigned char)*str)) {
    return 0;
  }
  prefix_count++;

  return prefix_count;
}

// Returned int is the number of characters in the prefix of the list item
int is_unordered_list_syntax(char *str) {
  if (str == NULL || strlen(str) < 2) {
    return 0;
  }

  if (strncmp(str, "- ", 2) == 0 || strncmp(str, "* ", 2) == 0 ||
      strncmp(str, "+ ", 2) == 0) {
    return 2;
  }

  return 0;
}

// is_indented_line checks if the line is indented with `count` spaces.
// Returns `count` if the line is indented with `count` spaces, otherwise
// returns 0.
int is_indented_line(int count, char *str) {
  if (str == NULL || strlen(str) < count) {
    return 0;
  }

  for (int i = 0; i < count; i++) {
    if (str[i] != ' ') {
      return 0;
    }
  }
  return count;
}

int is_indented_tab(char *str) {
  if (str == NULL || strlen(str) < 1) {
    return 0;
  }

  if (*str == '\t') {
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
  case ORDERED_LIST:
    return "ORDERED LIST";
  case UNORDERED_LIST:
    return "UNORDERED LIST";
  case LIST_ITEM:
    return "LIST ITEM";
  case PLAIN:
    return "PLAIN";
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
