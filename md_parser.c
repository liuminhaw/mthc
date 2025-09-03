#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_reader.h"
#include "md_parser.h"
#include "md_regex.h"
#include "str_utils.h"

static const int INDENT_SIZE = 4;

const int parsers_count = 11;
Parsers parsers[] = {{heading_parser, 0},      {blockquote_parser, 1},
                     {ordered_list_parser, 1}, {unordered_list_parser, 1},
                     {codeblock_parser, 1},    {horizontal_line_parser, 0},
                     {plain_parser, 0},        {link_reference_parser, 0},
                     {html_tag_parser, 1},     {paragraph_parser, 1},
                     {section_break_parser, 0}};

MDBlock *block_parsing(MDBlock *prnt_block, MDBlock *curr_block,
                       PeekReader *reader, MDLinkReference *link_ref_head) {
  LOGF("parsing block: %s\n", peek_reader_current(reader));
  MDBlock *new_block = NULL;

  for (int i = 0; i < parsers_count; i++) {
    new_block = parsers[i].parser(prnt_block, curr_block, reader);
    if (parsers[i].multiline) {
      if (new_block != NULL && new_block == curr_block) {
        return NULL;
      }
    }
    if (new_block != NULL) {
      if (new_block != curr_block && curr_block != NULL) {
        child_parsing_exec(link_ref_head, curr_block);
      }
      return new_block;
    }
  }

  return NULL;
}

MDBlock *content_block_parsing(MDBlock *prnt_block, MDBlock *curr_block,
                               PeekReader *reader,
                               MDLinkReference *link_ref_head) {
  MDBlock *new_block = list_item_parser(prnt_block, curr_block, reader);
  if (new_block != NULL) {
    inline_parsing(link_ref_head, new_block);
    // printf("list item block content: %s\n", new_block->content);
    if (strchr(new_block->content, '\n') != NULL) {
      LOGF("parse list item child block\n");
      new_block->child = child_block_parsing(link_ref_head, new_block);
    }
    // printf("return list item block\n");
    return new_block;
  }

  new_block = block_parsing(prnt_block, curr_block, reader, link_ref_head);
  return new_block;
}

void child_parsing_exec(MDLinkReference *head, MDBlock *block) {
  if (block) {
    switch (block->block) {
    case BLOCKQUOTE:
    case ORDERED_LIST:
    case UNORDERED_LIST:
      if (block->child == NULL) {
        block->child = child_block_parsing(head, block);
      }
      break;
    default:
      break;
    }
  }
}

MDBlock *child_block_parsing(MDLinkReference *link_ref_head,
                             MDBlock *prnt_block) {
  LOGF("child block parsing content:\n%s\n", prnt_block->content);
  MDBlock *head_block = NULL;
  MDBlock *tail_block = head_block;
  MDBlock *new_block = NULL;

  int line_count = 0;
  char **lines = content_splitter(prnt_block->content, '\n', &line_count);
  if (lines == NULL) {
    perror("content_splitter failed");
    return NULL;
  }

  PeekReader *reader =
      new_peek_reader_from_lines(lines, line_count, DEFAULT_PEEK_COUNT);
  if (!reader) {
    return NULL;
  }

  do {
    new_block =
        content_block_parsing(prnt_block, tail_block, reader, link_ref_head);
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
  } while (reader->count > 0);
  free_peek_reader(reader);

  child_parsing_exec(link_ref_head, tail_block);
  inline_parsing(link_ref_head, tail_block);

  return head_block;
}

void inline_parsing(MDLinkReference *list, MDBlock *block) {
  if (block == NULL || block->content == NULL) {
    return;
  }

  if (block->block == CODEBLOCK || block->block == SECTION_BREAK) {
    return;
  }

  char *content = block->content;
  LOGF("inline origin content: %s\n", content);
  char *emphasis_content = emphasis_parser(content);
  LOGF("inline emphasis content: %s\n", emphasis_content);
  if (emphasis_content != NULL) {
    // free(NULL) is safe in C and does nothing
    free(block->content);
    block->content = emphasis_content;
    LOGF("replace content\n");
  }

  char *image_content = image_parser(block->content);
  LOGF("inline image content: %s\n", image_content);
  if (image_content != NULL && image_content != block->content) {
    if (block->content) {
      free(block->content);
    }
    block->content = image_content;
  }

  char *link_content = link_parser(list, block->content);
  LOGF("inline link content: %s\n", link_content);
  if (link_content != NULL && link_content != block->content) {
    if (block->content) {
      free(block->content);
    }
    block->content = link_content;
  }

  LOGF("block content after inline parsing: %s\n", block->content);

  return;
}

// escape_char_parsing is used to remove escape '\' from escape characters of
// the given string. The string is modified in place.
void escape_char_parsing(char *str) {
  if (str == NULL) {
    return;
  }

  int len = strlen(str);
  for (int i = 0; i < len - 1; i++) {
    if (str[i] == '\\') {
      // Check if the next character is a special character
      if (strchr("\\`*_{}[]<>()#+-.!|", str[i + 1]) != NULL) {
        // Shift the rest of the string left by one character
        memmove(&str[i], &str[i + 1], len - i);
        len--;
        str[len] = '\0'; // Null-terminate the string
      }
    }
  }

  // printf("escape_char_parsing result: %s\n", str);
}

MDBlock *heading_parser(MDBlock *prnt_block, MDBlock *curr_block,
                        PeekReader *reader) {
  char *line = peek_reader_current(reader);
  char *line_ptr = line;
  int advanced_count = 1;

  int level = is_heading_syntax(&line_ptr);
  if (!level) {
    level = is_heading_alternate_syntax(reader);
    if (!level) {
      return NULL;
    }
    advanced_count++;
  }

  char *tag = malloc(3);
  if (tag == NULL) {
    perror("malloc failed");
    return NULL;
  }
  sprintf(tag, "h%d", level);

  MDBlock *new_block = new_mdblock(line_ptr, tag, level, BLOCK, 0);
  free(tag);

  for (int i = 0; i < advanced_count; i++) {
    if (!peek_reader_advance(reader)) {
      break; // Exit if no more lines to read
    }
  }

  return new_block;
}

MDBlock *html_tag_parser(MDBlock *prnt_block, MDBlock *curr_block,
                         PeekReader *reader) {
  MDBlock *new_block = NULL;

  char *line = peek_reader_current(reader);
  if (!is_html_start_tag(line, strlen(line))) {
    return NULL;
  }

  while (true) {
    line = peek_reader_current(reader);
    if (is_empty_or_whitespace(line)) {
      break;
    }

    if (new_block != NULL) {
      mdblock_content_update(new_block, line, "%s%s\n");
    } else {
      LOGF("new html tag block: %s\n", line);
      new_block = new_mdblock(line, "", PLAIN, NONE, 1);
    }

    peek_reader_advance(reader);
  }

  if (new_block->content != NULL) {
    size_t len = strlen(new_block->content);
    if (len > 0 && new_block->content[len - 1] == '\n') {
      new_block->content[len - 1] = '\0';
    }
  }
  return new_block;
}

MDBlock *paragraph_parser(MDBlock *prnt_block, MDBlock *curr_block,
                          PeekReader *reader) {
  MDBlock *new_block = NULL;
  while (true) {
    char *line = peek_reader_current(reader);

    if (is_empty_or_whitespace(line)) {
      break;
    }
    if (new_block != NULL) {
      mdblock_content_update(new_block, line, "%s %s");
    } else {
      LOGF("new paragraph block: %s\n", line);
      new_block = new_mdblock(line, "p", PARAGRAPH, BLOCK, 0);
    }

    peek_reader_advance(reader);
    if (!safe_paragraph_content(reader, 0)) {
      break;
    }
  }

  return new_block;
}

MDBlock *blockquote_parser(MDBlock *prnt_block, MDBlock *curr_block,
                           PeekReader *reader) {
  MDBlock *new_block = NULL;
  while (true) {
    char *line = peek_reader_current(reader);
    char *next_line = peek_reader_peek(reader, 1);

    if (is_empty_or_whitespace(line) || !is_blockquote_syntax(line)) {
      break;
    }

    // Removing leading '>' characters and spaces of the blockquote syntax
    line++;
    while (*line && isspace((unsigned char)*line)) {
      line++;
    }

    if (new_block == NULL) {
      new_block = new_mdblock(line, "blockquote", BLOCKQUOTE, BLOCK, 1);
    } else if (new_block != NULL) {
      mdblock_content_update(new_block, line, "%s%s\n");
    }

    peek_reader_advance(reader);
    if (!is_blockquote_syntax(next_line)) {
      break;
    }
  }
  return new_block;
}

MDBlock *ordered_list_parser(MDBlock *prnt_block, MDBlock *curr_block,
                             PeekReader *reader) {
  MDBlock *new_block = NULL;
  while (true) {
    char *line = peek_reader_current(reader);

    if (is_empty_or_whitespace(line)) {
      break;
    }

    if (new_block != NULL) {
      mdblock_content_update(new_block, line, "%s%s\n");
    } else if (is_ordered_list_syntax(line, 1)) {
      new_block = new_mdblock(line, "ol", ORDERED_LIST, BLOCK, 1);
    } else {
      break;
    }

    peek_reader_advance(reader);
    if (!safe_ordered_list_content(reader, 0)) {
      break;
    }
  }

  return new_block;
}

MDBlock *unordered_list_parser(MDBlock *prnt_block, MDBlock *curr_block,
                               PeekReader *reader) {
  MDBlock *new_block = NULL;
  while (true) {
    char *line = peek_reader_current(reader);

    if (is_empty_or_whitespace(line)) {
      break;
    }

    if (new_block != NULL) {
      mdblock_content_update(new_block, line, "%s%s\n");
    } else if (is_unordered_list_syntax(line)) {
      new_block = new_mdblock(line, "ul", UNORDERED_LIST, BLOCK, 1);
    } else {
      break;
    }

    peek_reader_advance(reader);
    if (!safe_unordered_list_content(reader, 0)) {
      break;
    }
  }

  return new_block;
}

MDBlock *list_item_parser(MDBlock *prnt_block, MDBlock *prev_block,
                          PeekReader *reader) {
  MDBlock *new_block = NULL;
  while (true) {
    char *line = peek_reader_current(reader);
    char *next_line = peek_reader_peek(reader, 1);
    // printf("list item parsing: %s\n", line);
    // printf("parent block: %d\n", prnt_block->block);

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
      if (new_block == NULL) {
        return NULL;
      }
    }

    // printf("list item offset: %d\n", offset);
    if (!offset && new_block != NULL) {
      if (is_indented_line(INDENT_SIZE, line)) {
        char *line_ptr = line + INDENT_SIZE;
        mdblock_content_update(new_block, line_ptr, "%s\n%s");
      } else {
        mdblock_content_update(new_block, line, "%s %s");
      }
    } else if (offset) {
      char *line_ptr = line + offset;
      // printf("new list item block\n");
      new_block = new_mdblock(line_ptr, "li", LIST_ITEM, BLOCK, 0);
    }

    peek_reader_advance(reader);
    if (!safe_paragraph_content(reader, 0) &&
        !is_indented_line(INDENT_SIZE, next_line)) {
      break;
    }
  }

  return new_block;
}

MDBlock *codeblock_parser(MDBlock *prnt_block, MDBlock *curr_block,
                          PeekReader *reader) {
  LOGF("Enter codeblock parser\n");
  MDBlock *new_block = NULL;
  while (true) {
    char *line = peek_reader_current(reader);
    char *next_line = peek_reader_peek(reader, 1);

    bool is_indented = is_indented_line(INDENT_SIZE, line);
    if (!is_indented && is_empty_or_whitespace(line)) {
      break;
    }

    if (is_indented_line(INDENT_SIZE, line)) {
      line += INDENT_SIZE;
    } else if (is_indented_tab(line)) {
      line++;
    } else {
      break;
    }

    if (new_block != NULL) {
      mdblock_content_update(new_block, line, "%s\n%s");
    } else {
      new_block = new_mdblock(line, "pre", CODEBLOCK, BLOCK, 0);
    }

    peek_reader_advance(reader);
    if (!is_indented_line(INDENT_SIZE, next_line) &&
        !is_indented_tab(next_line)) {
      break;
    }
  }

  return new_block;
}

MDBlock *horizontal_line_parser(MDBlock *prnt_block, MDBlock *curr_block,
                                PeekReader *reader) {
  char *line = peek_reader_current(reader);
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  char *trimmed_line = ltrim_space(line);

  if (curr_block == NULL || curr_block->block == SECTION_BREAK) {
    size_t len = strlen(trimmed_line);
    if (len >= 3 &&
        (strspn(trimmed_line, "-") == len || strspn(trimmed_line, "*") == len ||
         strspn(trimmed_line, "_") == len)) {
      MDBlock *new_block =
          new_mdblock(NULL, "hr", HORIZONTAL_LINE, SELF_CLOSING, 0);
      free(trimmed_line);
      peek_reader_advance(reader);
      return new_block;
    }
  }

  free(trimmed_line);
  return NULL;
}

MDBlock *link_reference_parser(MDBlock *prnt_block, MDBlock *curr_block,
                               PeekReader *reader) {
  char *line = peek_reader_current(reader);
  if (is_empty_or_whitespace(line)) {
    return NULL;
  }

  MDLinkReference *ref = parse_markdown_links_reference(line);
  if (ref == NULL) {
    return NULL;
  }

  MDBlock *new_block = new_mdblock(line, NULL, LINK_REFERENCE, NONE, 0);

  free_md_link_reference(ref);

  peek_reader_advance(reader);
  return new_block;
}

MDBlock *plain_parser(MDBlock *prnt_block, MDBlock *curr_block,
                      PeekReader *reader) {
  char *line = peek_reader_current(reader);
  if (is_empty_or_whitespace(line) || prnt_block == NULL) {
    return NULL;
  }

  if (prnt_block->block != LIST_ITEM || curr_block != NULL) {
    return NULL;
  }

  MDBlock *new_block = new_mdblock(line, "", PLAIN, NONE, 0);

  peek_reader_advance(reader);
  return new_block;
}

MDBlock *section_break_parser(MDBlock *prnt_block, MDBlock *curr_block,
                              PeekReader *reader) {
  char *line = peek_reader_current(reader);
  if (!is_empty_or_whitespace(line)) {
    LOGF("section: not empty or whitespace\n");
    return NULL;
  }

  LOGF("new section break\n");
  MDBlock *new_block = new_mdblock(NULL, NULL, SECTION_BREAK, INLINE, 0);

  peek_reader_advance(reader);
  return new_block;
}

MDBlock *new_mdblock(char *content, char *html_tag, BlockTag block_tag,
                     TagType type, int content_newline) {
  MDBlock *block = malloc(sizeof(MDBlock));
  LOGF("[ALLOC] new_mdblock at %p, content: %s\n", (void *)block, content);
  if (!block) {
    perror("malloc failed");
    return NULL;
  }

  LOGF("new_mdblock line break parser\n");
  char *parsed_line = line_break_parser(content);
  if (parsed_line != NULL) {
    content = parsed_line;
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
  free(parsed_line);

  block->content = b_content;
  block->tag = html_tag ? strdup(html_tag) : NULL;
  block->block = block_tag;
  block->type = type;
  block->child = NULL;
  block->next = NULL;

  return block;
}

char *line_break_parser(const char *line) {
  if (line == NULL) {
    return NULL;
  }

  const char *substitute = "<br>";
  size_t line_len = strlen(line);
  LOGF("line len: %zu\n", line_len);
  if (line_len < 2) {
    return NULL;
  }
  size_t ws_count = 0;

  for (int i = line_len - 1; i >= 0; i--) {
    if (isspace((unsigned char)line[i])) {
      ws_count++;
    } else {
      break;
    }
  }

  // printf("line length: %zu, ws count: %zu\n", line_len, ws_count);
  if (ws_count < 2) {
    return NULL;
  }

  size_t new_len = line_len - ws_count + strlen(substitute) + 1;
  char *new_line = malloc(new_len);
  if (!new_line) {
    return NULL;
  }

  strncpy(new_line, line, line_len - ws_count);
  new_line[line_len - ws_count] = '\0'; // Null-terminate the string
  strcat(new_line, substitute);         // Append the line break substitute

  return new_line;
}

char *emphasis_parser(char *str) {
  if (str == NULL) {
    return NULL;
  }

  bool sub = false;
  char *result = fullstr_sub_tagpair(str, PT_NONE, &sub);
  return result;
}

char *link_parser(MDLinkReference *list, char *str) {
  LOGF("enter link_parser with str: %s\n", str);
  if (str == NULL) {
    return NULL;
  }

  size_t count = 0;
  MDLinkRegex *links = parse_markdown_links(list, str, &count);

  if (links == NULL || count == 0) {
    free_md_links(links, count);
    return str; // No links found, return original string
  }

  int offset = 0;
  char *dup_str = strdup(str);
  for (size_t i = 0; i < count; i++) {
    MDLinkRegex *link = &links[i];

    LOGF("link %zu: label=%s, url=%s, title=%s, start=%d, end=%d\n", i,
         link->label, link->url, link->title ? link->title : "NULL",
         link->start, link->end);

    link->start += offset;
    link->end += offset;

    char *sub_str = NULL;
    if (link->title) {
      // Has link title
      size_t link_len =
          strlen(link->label) + strlen(link->url) + strlen(link->title) + 24;
      sub_str = malloc((link_len + 1) * sizeof(char));
      if (sub_str == NULL) {
        perror("malloc failed");
        free_md_links(links, count);
        return dup_str; // Return original string on error
      }
      sprintf(sub_str, "<a href=\"%s\" title=\"%s\">%s</a>", link->url,
              link->title, link->label);
    } else {
      // No link title
      size_t link_len = strlen(link->label) + strlen(link->url) + 15;
      sub_str = malloc((link_len + 1) * sizeof(char));
      if (sub_str == NULL) {
        perror("malloc failed");
        free_md_links(links, count);
        return dup_str; // Return original string on error
      }
      sprintf(sub_str, "<a href=\"%s\">%s</a>", link->url, link->label);
    }

    char *tmp_str;
    size_t tmp_len =
        strlen(dup_str) + strlen(sub_str) - (link->end - link->start);
    tmp_str = malloc((tmp_len + 1) * sizeof(char));
    if (tmp_str == NULL) {
      perror("malloc failed");
      free(sub_str);
      free_md_links(links, count);
      return str; // Return original string on error
    }

    // Copy the part before the link
    memcpy(tmp_str, dup_str, link->start);
    memcpy(tmp_str + link->start, sub_str, strlen(sub_str));
    memcpy(tmp_str + link->start + strlen(sub_str), dup_str + link->end,
           strlen(dup_str) - link->end);
    tmp_str[tmp_len] = '\0'; // Null-terminate the string

    offset += strlen(sub_str) - (link->end - link->start);
    free(dup_str);
    free(sub_str);
    dup_str = tmp_str; // Update str to the new string with link replaced
  }
  free_md_links(links, count);

  return dup_str;
}

char *image_parser(char *str) {
  if (str == NULL) {
    return NULL;
  }

  size_t count = 0;
  MDLinkRegex *images = parse_markdown_images(str, &count);

  if (images == NULL || count == 0) {
    free_md_links(images, count);
    return str; // No images found, return original string
  }

  int offset = 0;
  char *dup_str = strdup(str);
  for (size_t i = 0; i < count; i++) {
    MDLinkRegex *image = &images[i];

    image->start += offset;
    image->end += offset;

    char *sub_str = NULL;
    char *str_template = NULL;
    size_t link_len = strlen(image->label) + strlen(image->src) + 19;
    if (image->title) {
      link_len += strlen(image->title) + 9;
      sub_str = malloc((link_len + 1) * sizeof(char));
      if (sub_str == NULL) {
        perror("malloc failed");
        free_md_links(images, count);
        return dup_str; // Return original string on error
      }
      sprintf(sub_str, "<img src=\"%s\" title=\"%s\" alt=\"%s\">", image->src,
              image->title, image->label);
    } else {
      sub_str = malloc((link_len + 1) * sizeof(char));
      if (sub_str == NULL) {
        perror("malloc failed");
        free_md_links(images, count);
        return dup_str; // Return original string on error
      }
      sprintf(sub_str, "<img src=\"%s\" alt=\"%s\">", image->src, image->label);
    }

    char *tmp_str;
    size_t tmp_len =
        strlen(dup_str) + strlen(sub_str) - (image->end - image->start);
    tmp_str = malloc((tmp_len + 1) * sizeof(char));
    if (tmp_str == NULL) {
      perror("malloc failed");
      free(sub_str);
      free_md_links(images, count);
      return str; // Return original string on error
    }

    // Copy the part before the image
    memcpy(tmp_str, dup_str, image->start);
    memcpy(tmp_str + image->start, sub_str, strlen(sub_str));
    memcpy(tmp_str + image->start + strlen(sub_str), dup_str + image->end,
           strlen(dup_str) - image->end);
    tmp_str[tmp_len] = '\0'; // Null-terminate the string

    offset += strlen(sub_str) - (image->end - image->start);
    free(dup_str);
    free(sub_str);
    dup_str = tmp_str; // Update str to the new string with image replaced
  }
  free_md_links(images, count);

  return dup_str;
}

int is_header_block(MDBlock block) {
  return 1 <= block.block && block.block <= 6;
}

// is_heading_syntax checks if the line starts with a heading syntax
// Returns the heading level (1-6) if it is a heading syntax, otherwise
// returns 0.
int is_heading_syntax(char **line) {
  if (is_empty_or_whitespace(*line)) {
    return 0;
  }

  int heading_level = 0;

  while (**line == '#') {
    heading_level++;
    (*line)++;
  }

  if (!isspace((unsigned char)**line)) {
    return 0;
  }

  while (**line && isspace((unsigned char)**line)) {
    (*line)++;
  }

  if (heading_level > 0 && heading_level < 7) {
    return heading_level;
  }
  return 0; // Not a header syntax
}

// is_heading_alternate_syntax checks if the line starts with a heading
// alternate syntax Returns 1 or 2 for matching alternate syntax h1 or h2
// respectively, otherwise returns 0.
int is_heading_alternate_syntax(PeekReader *reader) {
  char *line = peek_reader_current(reader);
  if (is_empty_or_whitespace(line)) {
    return 0;
  }

  char *next_line = peek_reader_peek(reader, 1);
  if (next_line == NULL) {
    return 0;
  }
  // printf("peek next line: %s\n", next_line);
  size_t len = strlen(next_line);
  if (len >= 2 && strspn(next_line, "=") == len) {
    return 1;
  } else if (len >= 2 && strspn(next_line, "-") == len) {
    return 2;
  }

  return 0;
}

bool is_blockquote_syntax(char *str) {
  if (str == NULL || *str != '>') {
    return false;
  }

  while (*str == '>') {
    str++;
  }
  if (!isspace((unsigned char)*str) && *str != '\0') {
    return false;
  }

  return true;
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

// Returned int which is the number of characters in the prefix of the list
// item
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

bool is_html_start_tag(char *s, size_t n) {
  if (s == NULL || n < 3) {
    return false;
  }

  size_t idx = 0;
  if (s[idx] != '<') {
    return false;
  }
  idx++;

  // Comment
  if (idx + 2 < n && s[idx] == '!' && s[idx + 1] == '-' && s[idx + 2] == '-') {
    idx += 3;
    for (; idx + 2 < n; idx++) {
      if (s[idx] == '-' && s[idx + 1] == '-' && s[idx + 2] == '>' &&
          idx + 3 == n) {
        return true;
      }
    }
  }

  // Declaration
  if (s[idx] == '!') {
    for (; idx < n; idx++) {
      if (s[idx] == '>' && idx + 1 == n) {
        return true;
      }
    }
  }

  // Processing instruction <? ... ?>
  if (s[idx] == '?') {
    idx++;
    for (; idx < n - 1; idx++) {
      if (s[idx] == '?' && s[idx + 1] == '>' && idx + 1 == n) {
        return true;
      }
    }
  }

  // Tag name
  if (idx >= n || !is_tag_name_start((unsigned char)s[idx])) {
    return false;
  }
  idx++;
  while (idx < n && is_tag_name_char((unsigned char)s[idx])) {
    idx++;
  }

  // Attributes / end
  while (true) {
    while (idx < n && isspace((unsigned char)s[idx])) {
      idx++;
    }
    if (idx >= n) {
      return false;
    }

    if (s[idx] == '>') {
      return true;
    }
    if (s[idx] == '/' && idx + 1 < n && s[idx + 1] == '>') {
      return true;
    }

    // attribute name
    if (!is_tag_name_start((unsigned char)s[idx])) {
      return false;
    }
    idx++;
    while (idx < n && is_tag_name_char((unsigned char)s[idx])) {
      idx++;
    }

    while (idx < n && isspace((unsigned char)s[idx])) {
      idx++;
    }
    if (idx < n && s[idx] == '=') {
      idx++;
      while (idx < n && isspace((unsigned char)s[idx])) {
        idx++;
      }
      if (idx >= n) {
        return false;
      }

      if (s[idx] == '"' || s[idx] == '\'') { // quoted attribute value
        char quote = s[idx++];
        while (idx < n && s[idx] != quote) {
          idx++;
        }
        if (idx >= n) {
          return false; // No closing quote
        }
        idx++;
      } else {
        size_t start = idx;
        while (idx < n) {
          unsigned char c = s[idx];
          if (isspace(c) || c == '"' || c == '\'' || c == '=' || c == '<' ||
              c == '>' || c == '`') {
            break;
          }
          idx++;
        }
        if (start == idx) {
          return false; // No attribute value
        }
      }
    }
  }
}

bool is_tag_name_start(int c) {
  return (c == '_' || c == ':' || (c >= 'A' && c <= 'Z') ||
          (c >= 'a' && c <= 'z'));
}

bool is_tag_name_char(int c) {
  return is_tag_name_start(c) || c == '-' || c == '.' || (c >= '0' && c <= '9');
}

bool is_indented_tab(char *str) {
  if (str == NULL || strlen(str) < 1) {
    return false;
  }

  if (*str == '\t') {
    return true;
  }
  return false;
}

/* Returns 1 if the string is empty or only whitespace, otherwise returns 0 */
bool is_empty_or_whitespace(const char *str) {
  // Optional: Handle NULL pointers as empty
  if (str == NULL) {
    return true;
  }

  // Iterate through each character in the string.
  while (*str) {
    if (!isspace((unsigned char)*str)) {
      // Found a non-whitespace character.
      return false;
    }
    str++;
  }
  // The string is either empty or only contained whitespace.
  return true;
}

bool safe_paragraph_content(PeekReader *reader, int peek) {
  char *line = peek_reader_peek(reader, peek);

  return !is_empty_or_whitespace(line) &&
         !is_indented_line(INDENT_SIZE, line) && !is_indented_tab(line) &&
         !is_blockquote_syntax(line) && !is_heading_syntax(&line) &&
         !is_heading_alternate_syntax(reader) &&
         !is_ordered_list_syntax(line, 0) && !is_unordered_list_syntax(line) &&
         !is_html_start_tag(line, strlen(line));
}

bool safe_ordered_list_content(PeekReader *reader, int peek) {
  char *line = peek_reader_peek(reader, peek);
  LOGF("safe ordered list content: %s\n", line);

  return is_ordered_list_syntax(line, 0) ||
         is_indented_line(INDENT_SIZE, line) ||
         safe_paragraph_content(reader, peek);
}

bool safe_unordered_list_content(PeekReader *reader, int peek) {
  char *line = peek_reader_peek(reader, peek);

  return is_unordered_list_syntax(line) ||
         is_indented_line(INDENT_SIZE, line) ||
         safe_paragraph_content(reader, peek);
}

void mdblock_content_update(MDBlock *block, char *content, char *formatter) {
  char *parsed_line = line_break_parser(content);
  if (parsed_line != NULL) {
    free(block->content);
    block->content = parsed_line;
  }

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

void free_mdblocks(MDBlock *block) {
  if (block == NULL) {
    return;
  }

  LOGF("[FREE] md_block at %p, content: %s\n", (void *)block, block->content);
  free(block->content);
  if (block->tag) {
    free(block->tag);
  }
  free_mdblocks(block->child);
  free_mdblocks(block->next);
  free(block);
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
  case SELF_CLOSING:
    return "SELF CLOSING";
  case NONE:
    return "NONE";
  default:
    return "INVALID";
  }
}
