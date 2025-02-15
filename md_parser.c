#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* heading_parser(char *target_str, char *pattern, int match_count, int target_match) {
  regex_t regex;
  // const char *pattern = "^#\\s+(.*)$";
  // Index 0 : whole match; Index 1 : first group
  regmatch_t matches[match_count];

  if (regcomp(&regex, pattern, REG_EXTENDED)) {
    fprintf(stderr, "Could not compile regex\n");
    return NULL;
  }

  int ret = regexec(&regex, target_str, match_count, matches, 0);
  if (!ret) {
    if (matches[target_match].rm_so != -1) {
      int start = matches[target_match].rm_so;
      int end = matches[target_match].rm_eo;
      int len = end - start;

      char *captured = malloc(len + 1);
      if (!captured) {
        fprintf(stderr, "malloc failed\n");
        // perror("malloc");
        return NULL;
      }

      strncpy(captured, target_str + start, len);
      captured[len] = '\0';
      
      regfree(&regex);
      return captured;
    }
  } else if (ret == REG_NOMATCH) {
    return NULL;
  } else {
    char error_message[100];
    regerror(ret, &regex, error_message, sizeof(error_message));
    fprintf(stderr, "Regex match failed: %s\n", error_message);
    return NULL;
  }

  regfree(&regex);
  return NULL;
}

