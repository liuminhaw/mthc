#ifndef STR_UTILS_H
#define STR_UTILS_H

#include <unictype.h>

typedef struct {
  char *str;
  char *syntax;
  char *tag;
  char *start;
  char *end; // exclusive end
} TagPair;

TagPair *new_tag_pair(char *str, char *syntax, char *tag, int start, int end);
// TagPair *find_tag_pair(char *str, int start_idx);
TagPair *find_tag_pair(char *str, uint8_t *start_ptr);

void free_tag_pair(TagPair *pair);
void update_tag_pair_str(TagPair *pair, char *sub_str);

char *fullstr_sub_tagpair(char *str);
char *str_copy(const char *str);
char *str_sub_tagpair(TagPair *pair);
// ucs4_t str_peek(const uint8_t *str, int offset);
bool str_peek(const uint8_t *str, int offset, ucs4_t *result);
const uint8_t *str_move(const uint8_t *str, int offset);


#endif
