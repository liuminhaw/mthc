#ifndef MD_REGEX_H
#define MD_REGEX_H

#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#endif

#include <string.h>
#include <pcre2.h>
#include "file_reader.h"

typedef struct {
  char *label;
  char *url;
  char *src; // source for image links
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

MDLinkRegex *new_md_link(const char *label, const char *url, const char *title, const char *src, int start, int end);
MDLinkRegex *parse_markdown_links(MDLinkReference *head, const char *str, size_t *result_count);
MDLinkRegex *parse_markdown_general_links(const char *str, size_t *result_count);
MDLinkRegex *parse_markdown_links_tag(MDLinkReference *head, const char *str, size_t *result_count);
MDLinkRegex *parse_simple_addresses(const char *str, size_t *result_count);
MDLinkRegex *parse_markdown_images(const char *str, size_t *result_count);
void free_md_links(MDLinkRegex *links, size_t count);

MDLinkReference *parse_markdown_links_reference(char *str);
MDLinkReference *new_md_link_reference(const char *label, const char *url, const char *title);
MDLinkReference *find_link_reference(MDLinkReference *head, char *label);
MDLinkReference *gen_markdown_link_reference_list(PeekReader *reader);
void print_md_links_reference(MDLinkReference *head);
void free_md_link_reference(MDLinkReference *head);

void str_to_lower(char *str);
bool is_escaped_at(const char *str, PCRE2_SIZE pos);
bool is_escaped(const char *str, char start_target, char end_target, PCRE2_SIZE start, PCRE2_SIZE end);

char *escape_char_removal(char *str);

#endif
