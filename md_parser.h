#ifndef PARSER_H
#define PARSER_H

// Block type format
// #define PSR_H1_PATTERN "^#\\s+(.*)$"
// #define PSR_H2_PATTERN "^##\\s+(.*)$"
// #define PSR_H3_PATTERN "^###\\s+(.*)$"
// #define PSR_H4_PATTERN "^####\\s+(.*)$"
// #define PSR_H5_PATTERN "^#####\\s+(.*)$"
// #define PSR_H6_PATTERN "^######\\s+(.*)$"

#endif

typedef enum {
  INVALID,
  H1, // 1
  H2, // 2
  H3, // 3
  H4, // 4
  H5, // 5 
  H6, // 6
  PARAGRAPH,
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
  struct MDBlock *next;
} MDBlock;


MDBlock *block_parsing(MDBlock *block, char *target_str);
MDBlock *heading_parser(char *line);
MDBlock *paragraph_parser(MDBlock* block, char* line);

bool is_empty_or_whitespace(const char *str); 

char *blocktag_to_string(BlockTag block);
char *tagtype_to_string(TagType type);
