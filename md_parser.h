#ifndef PARSER_H
#define PARSER_H

// Block type format
// #define PSR_H1_PATTERN "^#\\s+(.*)$"
// #define PSR_H2_PATTERN "^##\\s+(.*)$"
// #define PSR_H3_PATTERN "^###\\s+(.*)$"
// #define PSR_H4_PATTERN "^####\\s+(.*)$"
// #define PSR_H5_PATTERN "^#####\\s+(.*)$"
// #define PSR_H6_PATTERN "^######\\s+(.*)$"

#include "file_reader.h"
#include "md_regex.h"

typedef enum {
  INVALID,
  H1, // 1
  H2, // 2
  H3, // 3
  H4, // 4
  H5, // 5 
  H6, // 6
  PARAGRAPH,
  BLOCKQUOTE,
  ORDERED_LIST,
  UNORDERED_LIST,
  LIST_ITEM,
  CODEBLOCK,
  HORIZONTAL_LINE,
  SECTION_BREAK,
  LINK_REFERENCE,
  PLAIN // plain text that don't use any tag 
} BlockTag;

typedef enum {
  BLOCK,
  SELF_CLOSING,
  INLINE,
  NONE,
} TagType;

typedef struct MDBlock {
  char *content;
  char *tag;
  TagType type;
  BlockTag block;
  struct MDBlock *child;
  struct MDBlock *next;
} MDBlock;

typedef struct {
  MDBlock* (*parser)(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader);
  int multiline;
} Parsers;



MDBlock* new_mdblock(char *content, char *html_tag, BlockTag block_tag, 
                     TagType type, int content_newline);

MDBlock *block_parsing(MDBlock *prnt_block, MDBlock *block, PeekReader *reader, MDLinkReference *link_ref_head);
MDBlock *heading_parser(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader);
MDBlock *blockquote_parser(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader);
MDBlock *ordered_list_parser(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader);
MDBlock *unordered_list_parser(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader);
MDBlock *list_item_parser(MDBlock *prnt_block, MDBlock *prev_block, PeekReader *reader);
MDBlock *codeblock_parser(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader);
MDBlock *horizontal_line_parser(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader);
MDBlock *plain_parser(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader);
MDBlock *paragraph_parser(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader);
MDBlock *section_break_parser(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader);
MDBlock *link_reference_parser(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader);
MDBlock *child_block_parsing(MDLinkReference *link_ref_head, MDBlock *block);
MDBlock *content_block_parsing(MDBlock *prnt_block, MDBlock *curr_block, PeekReader *reader, MDLinkReference *link_ref_head);

void inline_parsing(MDLinkReference *list, MDBlock *block);
char *line_break_parser(const char *line);
char *emphasis_parser(char *str);
char *link_parser(MDLinkReference *list, char *str);

int is_header_block(MDBlock block);
int is_heading_syntax(char **str);
int is_heading_alternate_syntax(PeekReader *reader);
int is_ordered_list_syntax(char *str, int first_item); 
int is_unordered_list_syntax(char *cp);
int is_indented_line(int count, char *str);
bool is_empty_or_whitespace(const char *str); 
bool is_indented_tab(char *str);
bool is_blockquote_syntax(char *str);
bool safe_paragraph_content(PeekReader *reader, int peek);
bool safe_ordered_list_content(PeekReader *reader, int peek);
bool safe_unordered_list_content(PeekReader *reader, int peek);

void child_parsing_exec(MDLinkReference *link_ref_head, MDBlock *block);
void mdblock_content_update(MDBlock *block, char *content, char *formatter);

char* blocktag_to_string(BlockTag block);
char* tagtype_to_string(TagType type);

#endif
