#ifndef PARSER_H
#define PARSER_H

// Block type format
// #define PSR_H1_PATTERN "^#\\s+(.*)$"
// #define PSR_H2_PATTERN "^##\\s+(.*)$"
// #define PSR_H3_PATTERN "^###\\s+(.*)$"
// #define PSR_H4_PATTERN "^####\\s+(.*)$"
// #define PSR_H5_PATTERN "^#####\\s+(.*)$"
// #define PSR_H6_PATTERN "^######\\s+(.*)$"


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
  SECTION_BREAK,
} BlockTag;

typedef enum {
  BLOCK,
  INLINE,
} TagType;

typedef struct MDBlock {
  char *content;
  char *tag;
  TagType type;
  BlockTag block;
  struct MDBlock *child;
  struct MDBlock *next;
} MDBlock;


MDBlock* new_mdblock(char *content, char *html_tag, BlockTag block_tag, 
                     TagType type, int content_newline);

MDBlock* block_parsing(MDBlock *block, char *target_str);
MDBlock* heading_parser(char *line);
MDBlock* blockquote_parser(MDBlock *block, char *line);
MDBlock* ordered_list_parser(MDBlock *block, char *line);
MDBlock* unordered_list_parser(MDBlock *block, char *line);
MDBlock *list_item_parser(MDBlock *prnt_block, MDBlock *prev_block, char *line);
MDBlock* paragraph_parser(MDBlock* block, char* line);
MDBlock* child_block_parsing(MDBlock *block);

bool is_header_block(MDBlock block);
bool is_empty_or_whitespace(const char *str); 
int is_ordered_list_syntax(char *str, int first_item); 
int is_unordered_list_syntax(char *cp);

void mdblock_content_update(MDBlock *block, char *content, char *formatter);

char* blocktag_to_string(BlockTag block);
char* tagtype_to_string(TagType type);

#endif
