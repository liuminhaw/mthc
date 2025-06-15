#ifndef MD_REGEX_H
#define MD_REGEX_H

#include <string.h>

typedef struct {
  char *label;
  char *url;
  char *title; // Optional title
  int start;
  int end;
} MDLinkRegex;

MDLinkRegex *parse_markdown_links(const char *str, size_t *result_count);
void free_md_links(MDLinkRegex *links, size_t count);

#endif
