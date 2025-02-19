#ifndef PARSER_H
#define PARSER_H

// Block type format
#define PSR_H1_PATTERN "^#\\s+(.*)$"
#define PSR_H2_PATTERN "^##\\s+(.*)$"
#define PSR_H3_PATTERN "^###\\s+(.*)$"
#define PSR_H4_PATTERN "^####\\s+(.*)$"
#define PSR_H5_PATTERN "^#####\\s+(.*)$"
#define PSR_H6_PATTERN "^######\\s+(.*)$"

#endif

typedef enum {
  H1, 
  H2,
  H3,
  H4,
  H5,
  H6,
  PARAGRAPH,
  SECTION_BREAK,
  INVALID,
} BlockType;

typedef struct MDBlock {
  char *content;
  BlockType type;
  struct MDBlock *next;
} MDBlock;


char *heading_parser(char *target_str, char *pattern, int match_count, int target_match);
bool is_empty_or_whitespace(const char *str); 

MDBlock *block_parsing(MDBlock *block, char *target_str);
char *read_paragraph(MDBlock *block, char *line);
