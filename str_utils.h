#ifndef STR_UTILS_H
#define STR_UTILS_H

typedef struct {
  char *str;
  char *syntax;
  char *tag;
  char *start;
  char *end; // exclusive end
} TagPair;

TagPair *new_tag_pair(char *str, char *syntax, char *tag, int start, int end);
TagPair *find_tag_pair(char *str, int start_idx);

void free_tag_pair(TagPair *pair);

void update_tag_pair_str(TagPair *pair, char *sub_str);
char *str_copy(const char *str);
char *str_sub_tagpair(TagPair *pair);


#endif
