#ifndef MD_REGEX_H
#define MD_REGEX_H

#include <string.h>
#include "file_reader.h"

typedef struct {
  char *label;
  char *url;
  char *title; // Optional title
  int start;
  int end;
} MDLinkRegex;

typedef struct MDLinkReference {
  char *label;
  char *url;
  char *title; // Optional title
  struct MDLinkReference *next; // For linked list of references
} MDLinkReference;

MDLinkRegex *parse_markdown_links(const char *str, size_t *result_count);
void free_md_links(MDLinkRegex *links, size_t count);

MDLinkReference *parse_markdown_links_reference(PeekReader *reader);
MDLinkReference *new_md_link_reference(const char *label, const char *url,
                                           const char *title);
void print_md_links_reference(MDLinkReference *head);
void free_md_link_reference(MDLinkReference *head);

void str_to_lower(char *str);

#endif
