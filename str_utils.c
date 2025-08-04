#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unictype.h>
#include <unistr.h>
// #include <unitypes.h>

#include "str_utils.h"

TagPair *new_tag_pair(PairType type, char *str, char *syntax, int start,
                      int end) {
  if (str == NULL || syntax == NULL) {
    return NULL;
  }

  char *tag;
  switch (type) {
  case PT_STRONG:
    tag = "strong";
    break;
  case PT_EM:
    tag = "em";
    break;
  case PT_CODE:
    tag = "code";
    break;
  default:
    return NULL;
  }

  TagPair *pair = malloc(sizeof(TagPair));
  if (pair == NULL) {
    perror("malloc failed");
    return NULL;
  }

  pair->type = type;
  pair->str = str_copy(str);
  pair->syntax = syntax;
  pair->tag = tag;
  pair->start = pair->str + start;
  pair->end = pair->str + end;

  return pair;
}

void update_tag_pair_str(TagPair *pair, char *sub_str) {
  if (pair == NULL || sub_str == NULL) {
    return;
  }

  size_t sub_str_len = strlen(sub_str);
  size_t original_len = strlen(pair->str);
  size_t new_len = original_len - (pair->end - pair->start) +
                   (2 * strlen(pair->syntax)) + sub_str_len;
  char *new_str = malloc(new_len + 1);
  if (new_str == NULL) {
    perror("malloc failed");
    return;
  }

  // Copy the part before the start of the tag pair
  memcpy(new_str, pair->str, pair->start - pair->str + strlen(pair->syntax));
  // Copy the new substituted string
  memcpy(new_str + (pair->start - pair->str) + strlen(pair->syntax), sub_str,
         sub_str_len);
  // Copy the part after the end of the tag pair
  memcpy(new_str + (pair->start - pair->str) + strlen(pair->syntax) +
             sub_str_len,
         pair->end - strlen(pair->syntax),
         original_len - (pair->end - pair->str) + strlen(pair->syntax));
  new_str[new_len] = '\0'; // Null-terminate the string

  pair->start = new_str + (pair->start - pair->str); // Update start pointer
  pair->end = pair->start + sub_str_len + (2 * strlen(pair->syntax));
  free(pair->str);
  pair->str = new_str;

  return;
}

void free_tag_pair(TagPair *pair) {
  if (pair == NULL) {
    return;
  }

  free(pair->str);
  free(pair);
}

char *str_copy(const char *str) {
  if (str == NULL) {
    return NULL;
  }

  size_t len = strlen(str);
  char *copy = malloc(len + 1);
  if (copy == NULL) {
    perror("malloc failed");
    return NULL;
  }

  strcpy(copy, str);

  return copy;
}

// fullstr_sub_tagpair recursively substitutes tag pairs in the input string.
// If there is nothing substituted, it returns the original string.
// Otherwise, a new string is returned with the substitutions made.
char *fullstr_sub_tagpair(char *str, PairType parent_type, bool *sub) {
  StrRecorder *recorder = malloc(sizeof(StrRecorder));
  if (recorder == NULL) {
    perror("malloc failed");
    return NULL;
  }
  recorder->str = strdup(str);
  recorder->ptr =
      (uint8_t *)(recorder->str); // Pointer to the start of the string

  while (true) {
    TagPair *pair = NULL;

    fprintf(stderr, "parent type: %d, pt code: %d\n", parent_type, PT_CODE);
    TagPair *(*fn)(char *str, uint8_t *start_ptr) =
        pair_finder_fn_exec(recorder->str, parent_type);
    if (fn == NULL) {
      char *result = strdup(recorder->str);
      free_str_recorder(recorder);
      return result;
    }
    pair = fn(recorder->str, recorder->ptr);
    // if (pair != NULL) {
    //   printf("Found tag pair type: %d\n", pair->type);
    //   printf("Found tag pair string: %s\n", pair->str);
    //   printf("Found tag pair syntax: %s\n", pair->syntax);
    //   printf("Found tag pair tag: %s\n", pair->tag);
    //   printf("Found tag pair start: %s\n", pair->start);
    //   printf("Found tag pair end: %s\n", pair->end);
    // }

    if (pair == NULL) {
      // printf("No tag pair found, returning original string: %s\n",
      // recorder->str);
      char *result = strdup(recorder->str);
      free_str_recorder(recorder);
      return result;
    }

    size_t child_str_len = pair->end - pair->start - (2 * strlen(pair->syntax));
    char *child_str = malloc(sizeof(char) * (child_str_len + 1));

    if (child_str == NULL) {
      perror("malloc failed");
      free_tag_pair(pair);
      free_str_recorder(recorder);
      return NULL;
    }

    strncpy(child_str, pair->start + strlen(pair->syntax),
            pair->end - pair->start - (2 * strlen(pair->syntax)));
    child_str[child_str_len] = '\0'; // Null-terminate the string

    char *substituted = fullstr_sub_tagpair(child_str, pair->type, sub);
    if (sub) {
      update_tag_pair_str(pair, substituted);
    }
    free(substituted);
    free(child_str);

    str_sub_tagpair(pair, recorder);
    *sub = true;
    free_tag_pair(pair);
  }
}

void str_sub_tagpair(TagPair *pair, StrRecorder *recorder) {
  if (pair == NULL || pair->str == NULL || pair->start == NULL ||
      pair->end == NULL) {
    return;
  }

  size_t substr_len = pair->end - pair->start;
  size_t syntax_len = strlen(pair->syntax);
  size_t prefix_len = strlen(pair->tag) + 2; // +2 for < and >
  size_t suffix_len = strlen(pair->tag) + 3; // +3 for </ and >
  size_t replacement_len = substr_len - (2 * syntax_len);
  size_t total_len = strlen(pair->str) - substr_len + prefix_len + suffix_len +
                     replacement_len;

  char *result = malloc(total_len + 1);
  if (result == NULL) {
    perror("malloc failed");
    return;
  }

  char *open_tag = malloc(prefix_len + 1);
  if (open_tag == NULL) {
    perror("malloc failed");
    free(result);
    return;
  }
  char *close_tag = malloc(suffix_len + 1);
  if (close_tag == NULL) {
    perror("malloc failed");
    free(open_tag);
    free(result);
    return;
  }
  sprintf(open_tag, "<%s>", pair->tag);   // Create <tag>
  sprintf(close_tag, "</%s>", pair->tag); // Create </tag>

  // Copy the part before the start of the tag pair
  memcpy(result, pair->str, pair->start - pair->str);
  memcpy(result + (pair->start - pair->str), open_tag, prefix_len);
  memcpy(result + (pair->start - pair->str) + prefix_len,
         pair->start + syntax_len, replacement_len);
  memcpy(result + (pair->start - pair->str) + prefix_len + replacement_len,
         close_tag, suffix_len);
  memcpy(result + (pair->start - pair->str) + prefix_len + replacement_len +
             suffix_len,
         pair->end, strlen(pair->end) + 1); // +1 for null terminator
  result[total_len] = '\0';                 // Null-terminate the string

  free(open_tag);
  free(close_tag);

  if (recorder->str != NULL) {
    free(recorder->str);
  }
  recorder->str = result;
  recorder->ptr = (uint8_t *)result + (pair->end - pair->str) +
                  (prefix_len + suffix_len - (2 * syntax_len));

  return;
}

TagPair *find_emphasis_pair(char *str, uint8_t *start_ptr) {
  const uint8_t *traverse_ptr = start_ptr;
  const size_t str_len = strlen(str);

  ucs4_t ch = 0;
  ucs4_t prev_ch = 0;
  while (traverse_ptr != NULL) {
    // printf("traverse_ptr: %s\n", traverse_ptr);
    if (ch != 0) {
      prev_ch = ch;
    }

    const uint8_t *next = u8_next(&ch, traverse_ptr);
    if (!next) {
      // printf("No next character found, returning NULL\n");
      break;
    }

    if (prev_ch == '\\' && (ch == '*' || ch == '_')) {
      // If the previous character is a backslash, skip this character
      traverse_ptr = next;
      continue;
    }

    ucs4_t peek_ch;
    switch (ch) {
    case '*':
      if (str_peek(next, 0, &peek_ch) && peek_ch == '*') {
        // Found bold syntax
        const uint8_t *peek_ptr = str_move(next, 1);
        const uint8_t *end_ptr = u8_strstr(peek_ptr, (const uint8_t *)"**");
        while (end_ptr != NULL && *end_ptr >= 1 && *(end_ptr - 1) == '\\') {
          end_ptr = u8_strstr(str_move(end_ptr, 1), (const uint8_t *)"**");
        }
        if (end_ptr == NULL) {
          traverse_ptr = next;
          continue; // No closing tag found, continue searching
        } else if (str_peek(end_ptr, 2, &peek_ch) && peek_ch == '*') {
          end_ptr = str_move(end_ptr, 1); // Move past the second '*'
        }
        end_ptr = str_move(end_ptr, 2); // Move past the '**'
        TagPair *pair = new_tag_pair(PT_STRONG, str, "**",
                                     traverse_ptr - (const uint8_t *)str,
                                     end_ptr - (const uint8_t *)str);
        if (pair == NULL) {
          return NULL;
        }
        return pair;
      } else {
        // Found italic syntax
        const uint8_t *end_ptr = u8_strstr(next, (const uint8_t *)"*");
        while (end_ptr != NULL && *end_ptr >= 1 && *(end_ptr - 1) == '\\') {
          end_ptr = u8_strstr(str_move(end_ptr, 1), (const uint8_t *)"*");
        }
        if (end_ptr == NULL) {
          traverse_ptr = next;
          continue; // No closing tag found, continue searching
        }
        end_ptr = str_move(end_ptr, 1); // Move past the '*'
        TagPair *pair =
            new_tag_pair(PT_EM, str, "*", traverse_ptr - (const uint8_t *)str,
                         end_ptr - (const uint8_t *)str);
        if (pair == NULL) {
          return NULL;
        }
        return pair;
      }
      break;
    case '_':
      if (is_utf8_word(prev_ch)) {
        traverse_ptr = next;
        continue;
      } else if (str_peek(next, 0, &peek_ch) && peek_ch == '_') {
        // Found bold syntax
        const uint8_t *peek_ptr = str_move(next, 1);
        const uint8_t *end_ptr = NULL;

        do {
          end_ptr = u8_strstr(peek_ptr, (const uint8_t *)"__");
          while (end_ptr != NULL && *end_ptr >= 1 && *(end_ptr - 1) == '\\') {
            end_ptr = u8_strstr(str_move(end_ptr, 1), (const uint8_t *)"__");
          }
          if (end_ptr == NULL) {
            break;
          }
          str_peek(end_ptr, 2, &peek_ch);
          if (peek_ch == '_') {
            end_ptr = str_move(end_ptr, 1); // Move past the second '_'
            break;
          } else if (is_utf8_word(peek_ch)) {
            peek_ptr = str_move(end_ptr, 2); // Move past the '__'
          } else {
            break;
          }
        } while (true);

        if (end_ptr == NULL) {
          traverse_ptr = next;
          continue;
        }

        end_ptr = str_move(end_ptr, 2); // Move past the '__'
        TagPair *pair = new_tag_pair(PT_STRONG, str, "__",
                                     traverse_ptr - (const uint8_t *)str,
                                     end_ptr - (const uint8_t *)str);
        if (pair == NULL) {
          return NULL;
        }
        return pair;
      } else {
        // Found italic syntax
        const uint8_t *end_ptr = NULL;
        do {
          end_ptr = u8_strstr(next, (const uint8_t *)"_");
          while (end_ptr != NULL && *end_ptr >= 1 && *(end_ptr - 1) == '\\') {
            end_ptr = u8_strstr(str_move(end_ptr, 1), (const uint8_t *)"_");
          }
          if (end_ptr == NULL) {
            break;
          }
          str_peek(end_ptr, 1, &peek_ch);
          if (is_utf8_word(peek_ch)) {
            next = str_move(end_ptr, 1);
          } else {
            break;
          }
        } while (true);

        if (end_ptr == NULL) {
          traverse_ptr = next;
          continue;
        }

        end_ptr = str_move(end_ptr, 1); // Move past the '_'
        TagPair *pair =
            new_tag_pair(PT_EM, str, "_", traverse_ptr - (const uint8_t *)str,
                         end_ptr - (const uint8_t *)str);
        if (pair == NULL) {
          return NULL;
        }
        return pair;
      }
      break;
    default:
      traverse_ptr = next; // Move to the next character
    }
  }
  return NULL;
}

TagPair *find_code_tag_pair(char *str, uint8_t *start_ptr) {
  const uint8_t *traverse_ptr = start_ptr;
  const size_t str_len = strlen(str);

  ucs4_t prev_ch = 0;
  ucs4_t ch = 0;
  while (traverse_ptr != NULL) {
    if (ch != 0) {
      prev_ch = ch;
    }

    const uint8_t *next = u8_next(&ch, traverse_ptr);
    if (!next) {
      break;
    }

    if (prev_ch == '\\' && ch == '`') {
      // If the previous character is a backslash, skip this character
      traverse_ptr = next;
      continue;
    }

    ucs4_t peek_ch;
    if (ch == '`') {
      if (str_peek(next, 0, &peek_ch) && peek_ch == '`') {
        // Found escaping backtick syntax
        const uint8_t *peek_ptr = str_move(next, 1);
        const uint8_t *end_ptr = u8_strstr(peek_ptr, (const uint8_t *)"``");
        if (end_ptr == NULL) {
          traverse_ptr = next;
          continue;
        }
        end_ptr = str_move(end_ptr, 2); // Move past the '``'
        TagPair *pair = new_tag_pair(PT_CODE, str, "``",
                                     traverse_ptr - (const uint8_t *)str,
                                     end_ptr - (const uint8_t *)str);
        return pair;
      } else {
        const uint8_t *end_ptr = u8_strstr(next, (const uint8_t *)"`");
        if (end_ptr == NULL) {
          traverse_ptr = next;
          continue;
        }
        end_ptr = str_move(end_ptr, 1); // Move past the '`'
        TagPair *pair =
            new_tag_pair(PT_CODE, str, "`", traverse_ptr - (const uint8_t *)str,
                         end_ptr - (const uint8_t *)str);
        return pair;
      }
    }

    traverse_ptr = next; // Move to the next character
  }
  return NULL;
}

bool run_pair_finder(PairType type, int check_count, PairType check_types[]) {
  bool flag = true;

  for (int i = 0; i < check_count; i++) {
    if (check_types[i] == type) {
      flag = false;
    }
  }

  return flag;
}

TagPair *(*pair_finder_fn_exec(char *str,
                               PairType parent_type))(char *str,
                                                      uint8_t *start_ptr) {
  PairType ap[] = {PT_CODE};
  if (!run_pair_finder(parent_type, 1, ap)) {
    return NULL;
  }

  TagPair *emphasis_pair = find_emphasis_pair(str, (uint8_t *)str);
  // if (emphasis_pair != NULL) {
  //   printf("Found tag emphasis_pair type: %d\n", emphasis_pair->type);
  //   printf("Found tag emphasis_pair string: %s\n", emphasis_pair->str);
  //   printf("Found tag emphasis_pair syntax: %s\n", emphasis_pair->syntax);
  //   printf("Found tag emphasis_pair tag: %s\n", emphasis_pair->tag);
  //   printf("Found tag emphasis_pair start: %s\n", emphasis_pair->start);
  //   printf("Found tag emphasis_pair end: %s\n", emphasis_pair->end);
  // }
  TagPair *code_pair = find_code_tag_pair(str, (uint8_t *)str);
  // if (code_pair != NULL) {
  //   printf("Found tag code_pair type: %d\n", code_pair->type);
  //   printf("Found tag code_pair string: %s\n", code_pair->str);
  //   printf("Found tag code_pair syntax: %s\n", code_pair->syntax);
  //   printf("Found tag code_pair tag: %s\n", code_pair->tag);
  //   printf("Found tag code_pair start: %s\n", code_pair->start);
  //   printf("Found tag code_pair end: %s\n", code_pair->end);
  // }

  char *result = NULL;
  if (emphasis_pair == NULL && code_pair == NULL) {
    return NULL;
  } else if (emphasis_pair == NULL) {
    free_tag_pair(code_pair);
    return find_code_tag_pair;
  } else if (code_pair == NULL) {
    free_tag_pair(emphasis_pair);
    return find_emphasis_pair;
  } else if (emphasis_pair->start - emphasis_pair->str <
             code_pair->start - code_pair->str) {
    free_tag_pair(code_pair);
    free_tag_pair(emphasis_pair);
    return find_emphasis_pair;
  } else {
    free_tag_pair(code_pair);
    free_tag_pair(emphasis_pair);
    return find_code_tag_pair;
  }
}

void free_str_recorder(StrRecorder *recorder) {
  if (recorder == NULL) {
    return;
  }

  if (recorder->str != NULL) {
    free(recorder->str);
  }
  free(recorder);
}

bool str_peek(const uint8_t *str, int offset, ucs4_t *result) {
  // printf("str_peek: %s, offset: %d\n", str, offset);
  if (str == NULL || offset < 0) {
    return false;
  }

  const uint8_t *ptr = str;
  // ucs4_t ch;
  for (int i = 0; i <= offset; i++) {
    ptr = u8_next(result, ptr);
    if (ptr == NULL) {
      return false; // Reached the end of the string
    }
  }

  return true;
}

const uint8_t *str_move(const uint8_t *str, int offset) {
  // printf("str_move: %s, offset: %d\n", str, offset);
  if (str == NULL || offset < 0) {
    return NULL;
  }

  const uint8_t *ptr = str;
  ucs4_t ch;
  for (int i = 0; i < offset; i++) {
    ptr = u8_next(&ch, ptr);
    if (ptr == NULL) {
      return NULL; // Reached the end of the string
    }
  }

  return ptr;
}

bool is_utf8_word(ucs4_t ch) { return uc_is_alpha(ch) || uc_is_digit(ch); }

#ifdef TEST_STR_UTILS
int main() {
  char *test_str = "emphasis in `inline *code*` should not be emphasized";
  // char *test_str =
  //     "This is a *test* string with **bold** and **_italic_** "
  //     "text, snake_case_text. ___simple bold and italic___, "
  //     "bold**in**text, __snake_case_bold_text__. "
  //     "_snake_case_italic_text_, ___snake_case_bold_italic_text___. "
  //     "Try with `code` and ``escape `code` syntax``. ";
  // char *test_str = "這是一個 *測試* 字串，包含 **粗體** 和 ___斜體___ 文字。"
  //                  "試試看 **_粗體斜體_**，以及 __蛇形_粗體__。";
  // char *test_str = "這是一個 *測試* 字串，包含 **粗體** 和 _斜體_ 文字。";

  char *result = fullstr_sub_tagpair(test_str, PT_NONE);
  if (result == NULL) {
    fprintf(stderr, "Error processing string.\n");
    return 1;
  }

  fprintf(stderr, "Original: %s\n\n", test_str);
  fprintf(stderr, "Substituted: %s\n", result);

  return 0;
}
#endif // TEST_STR_UTILS
