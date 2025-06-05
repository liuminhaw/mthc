#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str_utils.h"

TagPair *new_tag_pair(char *str, char *syntax, char *tag, int start, int end) {
  if (str == NULL || syntax == NULL || tag == NULL) {
    return NULL;
  }

  TagPair *pair = malloc(sizeof(TagPair));
  if (pair == NULL) {
    perror("malloc failed");
    return NULL;
  }

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

char *fullstr_sub_tagpair(char *str) {
  char *src_str = str;
  int start_idx = 0;

  while (true) {
    TagPair *pair = find_tag_pair(src_str, start_idx);
    // if (pair != NULL) {
    //   printf("Found tag pair string: %s\n", pair->str);
    //   printf("Found tag pair syntax: %s\n", pair->syntax);
    //   printf("Found tag pair tag: %s\n", pair->tag);
    //   printf("Found tag pair start: %s\n", pair->start);
    //   printf("Found tag pair end: %s\n", pair->end);
    // }

    if (pair == NULL) {
      return src_str;
    }

    size_t child_str_len = pair->end - pair->start - (2 * strlen(pair->syntax));
    char *child_str = malloc(sizeof(char) * (child_str_len + 1));

    if (child_str == NULL) {
      perror("malloc failed");
      free_tag_pair(pair);
      return NULL;
    }

    strncpy(child_str, pair->start + strlen(pair->syntax),
            pair->end - pair->start - (2 * strlen(pair->syntax)));
    child_str[child_str_len] = '\0'; // Null-terminate the string

    char *substituted = fullstr_sub_tagpair(child_str);
    if (substituted != child_str) {
      // Update pair fields with new substituted string
      update_tag_pair_str(pair, substituted);
    }
    free(child_str);

    char *tmp = str_sub_tagpair(pair);
    start_idx = pair->end - pair->str;
    free_tag_pair(pair);

    src_str = tmp;
  }
}

char *str_sub_tagpair(TagPair *pair) {
  if (pair == NULL || pair->str == NULL || pair->start == NULL ||
      pair->end == NULL) {
    return NULL;
  }

  size_t substr_len = pair->end - pair->start;
  size_t syntax_len = strlen(pair->syntax);
  size_t prefix_len = strlen(pair->tag) + 2; // +2 for < and >
  size_t suffix_len = strlen(pair->tag) + 3; // +3 for </ and >
  // size_t replacement_len =
  //     prefix_len + suffix_len + substr_len - (2 * syntax_len);
  // size_t total_len = strlen(pair->str) - substr_len + replacement_len;
  size_t replacement_len = substr_len - (2 * syntax_len);
  size_t total_len = strlen(pair->str) - substr_len + prefix_len + suffix_len +
                     replacement_len;

  char *result = malloc(total_len + 1);
  if (result == NULL) {
    perror("malloc failed");
    return NULL;
  }

  char *open_tag = malloc(prefix_len + 1);
  if (open_tag == NULL) {
    perror("malloc failed");
    free(result);
    return NULL;
  }
  char *close_tag = malloc(suffix_len + 1);
  if (close_tag == NULL) {
    perror("malloc failed");
    free(open_tag);
    free(result);
    return NULL;
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

  return result;
}

TagPair *find_tag_pair(char *str, int start_idx) {
  const char *find_ptr = str;
  const int str_len = strlen(str);

  for (int i = start_idx; i < str_len; i++) {
    if (str[i] == '*' || str[i] == '_') {
      // Found a potential emphasis marker
      char *start_ptr = &str[i];
      char *end_ptr = NULL;

      switch (str[i]) {
      case '*':
        if (i + 1 < str_len && str[i + 1] == '*') {
          // Found bold syntax
          end_ptr = strstr(start_ptr + 2, "**");
          if (end_ptr == NULL) {
            continue;
          }
          TagPair *pair =
              new_tag_pair(str, "**", "strong", i, end_ptr + 2 - str);
          if (pair == NULL) {
            return NULL;
          }
          return pair;
        } else {
          // Found italic syntax
          end_ptr = strstr(start_ptr + 1, "*");
          if (end_ptr == NULL) {
            continue;
          }
          TagPair *pair = new_tag_pair(str, "*", "em", i, end_ptr + 1 - str);
          if (pair == NULL) {
            return NULL;
          }
          return pair;
        }
        break;
      case '_':
        if (i + 1 < str_len && str[i + 1] == '_') {
          // Found bold syntax
          end_ptr = strstr(start_ptr + 2, "__");
          if (end_ptr == NULL) {
            continue;
          }
          TagPair *pair =
              new_tag_pair(str, "__", "strong", i, end_ptr + 2 - str);
          if (pair == NULL) {
            return NULL;
          }
          return pair;
        } else {
          // Found italic syntax
          end_ptr = strstr(start_ptr + 1, "_");
          if (end_ptr == NULL) {
            continue;
          }
          TagPair *pair = new_tag_pair(str, "_", "em", i, end_ptr + 1 - str);
          if (pair == NULL) {
            return NULL;
          }
          return pair;
        }
        break;
      }
    }
  }

  return NULL;
}

#ifdef TEST_STR_UTILS
int main() {
  char *test_str = "This is a *test* string with **bold** and **_italic_** "
                   "text, snake_case_text.";

  char *result = fullstr_sub_tagpair(test_str);
  if (result == NULL) {
    printf("Error processing string.\n");
    return 1;
  }

  printf("Original: %s\n", test_str);
  printf("Substituted: %s\n", result);

  return 0;
}
#endif // TEST_STR_UTILS
