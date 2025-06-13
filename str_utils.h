#ifndef STR_UTILS_H
#define STR_UTILS_H

#include <unictype.h>

typedef enum {
  PT_NONE,
  PT_STRONG,
  PT_EM,
  PT_CODE,
} PairType;

typedef struct {
  char *str; // The original full string
  uint8_t *ptr; // Dynamic pointer in the str string
} StrRecorder;

typedef struct {
  PairType type;
  char *str;
  char *syntax;
  char *tag; // TODO: use PairType to detemine tag
  char *start;
  char *end; // exclusive end
} TagPair;

TagPair *new_tag_pair(PairType type, char *str, char *syntax, int start, int end);
TagPair *find_emphasis_pair(char *str, uint8_t *start_ptr);
TagPair *find_code_tag_pair(char *str, uint8_t *start_ptr);

void free_tag_pair(TagPair *pair);
void update_tag_pair_str(TagPair *pair, char *sub_str);

char *fullstr_sub_tagpair(char *str, PairType parent_pair);
void str_sub_tagpair(TagPair *pair, StrRecorder *recorder);

bool run_pair_finder(PairType type, int check_count, PairType check_types[]);
TagPair *(*pair_finder_fn_exec(char *str, PairType parent_type))(char *str, uint8_t *start_ptr);

char *str_copy(const char *str);
bool str_peek(const uint8_t *str, int offset, ucs4_t *result);
const uint8_t *str_move(const uint8_t *str, int offset);

bool is_utf8_word(ucs4_t ch);


#endif
