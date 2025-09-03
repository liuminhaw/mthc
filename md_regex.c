#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#endif

#include <ctype.h>
#include <pcre2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_reader.h"
#include "md_regex.h"

MDLinkRegex *parse_markdown_links(MDLinkReference *head, const char *str,
                                  size_t *result_count) {
  size_t general_link_count = 0;
  MDLinkRegex *general_links =
      parse_markdown_general_links(str, &general_link_count);

  size_t simple_address_count = 0;
  MDLinkRegex *simple_addresses =
      parse_simple_addresses(str, &simple_address_count);

  size_t tag_link_count = 0;
  MDLinkRegex *tag_links = parse_markdown_links_tag(head, str, &tag_link_count);

  // Combine both arrays
  size_t total_count =
      general_link_count + simple_address_count + tag_link_count;
  MDLinkRegex *combined_links = malloc(total_count * sizeof(MDLinkRegex));
  if (!combined_links) {
    perror("malloc failed");
    free_md_links(general_links, general_link_count);
    free_md_links(tag_links, tag_link_count);
    free_md_links(simple_addresses, simple_address_count);
    *result_count = 0;
    return NULL;
  }

  memcpy(combined_links, general_links,
         general_link_count * sizeof(MDLinkRegex));
  memcpy(combined_links + general_link_count, tag_links,
         tag_link_count * sizeof(MDLinkRegex));
  memcpy(combined_links + general_link_count + tag_link_count, simple_addresses,
         simple_address_count * sizeof(MDLinkRegex));

  free(general_links);
  free(simple_addresses);
  free(tag_links);

  qsort(combined_links, total_count, sizeof(MDLinkRegex), cmp_md_link_start);

  *result_count = total_count;
  LOGF("parse markdown links: return combined.\n");
  return combined_links;
}

MDLinkRegex *parse_markdown_general_links(const char *str,
                                          size_t *result_count) {
  if (str == NULL) {
    return NULL;
  }

  PCRE2_SPTR8 subject = (PCRE2_SPTR8)str;
  PCRE2_SPTR8 pattern =
      (PCRE2_SPTR8) "(?s)<code>.*?</code>(*SKIP)(*FAIL)|"
                    "\\[([^\\]]+)\\]\\(([^)\\s]+)(?:\\s+\"([^\"]+)\")?\\)";

  int errorcode;
  PCRE2_SIZE erroroffset;

  // Compile with UTF and Unicode-property support
  pcre2_code_8 *re =
      pcre2_compile_8(pattern, PCRE2_ZERO_TERMINATED, PCRE2_UTF | PCRE2_UCP,
                      &errorcode, &erroroffset, NULL);
  if (!re) {
    PCRE2_UCHAR8 buffer[256];
    pcre2_get_error_message_8(errorcode, buffer, sizeof(buffer));
    LOGF("PCRE2 compilation failed at offset %zu: %s\n", erroroffset, buffer);
    return NULL;
  }

  /* Prepare match data block */
  pcre2_match_data_8 *md = pcre2_match_data_create_from_pattern_8(re, NULL);
  PCRE2_SIZE subject_len = strlen((char *)subject);
  PCRE2_SIZE offset = 0;
  int rc;

  // Setup dynamic array for storing MDLinkRegex output
  size_t capacity = 2;
  size_t count = 0;
  MDLinkRegex *arr = malloc(capacity * sizeof(MDLinkRegex));
  if (!arr) {
    perror("malloc MDLinkRegex failed");
    pcre2_match_data_free_8(md);
    pcre2_code_free_8(re);
    return NULL;
  }

  // Loop over matches
  while ((rc = pcre2_match_8(re, subject, subject_len, offset, 0, md, NULL)) >=
         0) {
    // [0]..[1] = full-match start/end, [2]..[3]=group1, [4]..[5]=group2,
    // [6]..[7]=group3
    PCRE2_SIZE *ov = pcre2_get_ovector_pointer_8(md);

    // Checking escape syntax
    // label part
    if (ov[2] >= 1 && subject[ov[2] - 1] == '[') {
      if (is_escaped_at((char *)subject, ov[2] - 1)) {
        offset = ov[1];
        continue; // Skip escaped links
      }
    }
    if (subject[ov[3]] == ']') {
      if (is_escaped_at((char *)subject, ov[3])) {
        offset = ov[1];
        continue; // Skip escaped links
      }
    }
    // Checking url part
    if (ov[4] >= 1 && subject[ov[4] - 1] == '(') {
      if (is_escaped_at((char *)subject, ov[4] - 1)) {
        offset = ov[1];
        continue;
      }
    }
    if (subject[ov[5]] == ')') {
      if (is_escaped_at((char *)subject, ov[5])) {
        offset = ov[1];
        continue;
      }
    }
    // Checking title part
    if (ov[6] != PCRE2_UNSET && ov[7] != PCRE2_UNSET) {
      if (ov[6] >= 1 && subject[ov[6] - 1] == '"') {
        if (is_escaped_at((char *)subject, ov[6] - 1)) {
          offset = ov[1];
          continue;
        }
      }
      if (subject[ov[7]] == '"') {
        if (is_escaped_at((char *)subject, ov[7])) {
          offset = ov[1];
          continue;
        }
      }
    }

    // Grow dynamic array
    if (count == capacity) {
      capacity *= 2;
      MDLinkRegex *tmp = realloc(arr, capacity * sizeof(*arr));
      if (!tmp) {
        perror("realloc");
        break;
      }
      arr = tmp;
    }

    // Extract captured info
    char *label = malloc(ov[3] - ov[2] + 1);
    char *url = malloc(ov[5] - ov[4] + 1);
    memcpy(label, subject + ov[2], ov[3] - ov[2]);
    memcpy(url, subject + ov[4], ov[5] - ov[4]);
    label[ov[3] - ov[2]] = '\0';
    url[ov[5] - ov[4]] = '\0';

    char *title = NULL;
    if (rc >= 4 && ov[7] != PCRE2_UNSET) {
      title = malloc(ov[7] - ov[6] + 1);
      memcpy(title, subject + ov[6], ov[7] - ov[6]);
      title[ov[7] - ov[6]] = '\0';
    }

    MDLinkRegex *link =
        new_md_link(label, url, title, NULL, (int)ov[0], (int)ov[1]);
    arr[count] = *link;
    free(link);

    count++;

    free(label);
    free(url);
    if (title) {
      free(title);
    }

    offset = ov[1];
  }

  pcre2_match_data_free_8(md);
  pcre2_code_free_8(re);

  *result_count = count;
  return arr;
}

MDLinkRegex *parse_markdown_links_tag(MDLinkReference *head, const char *str,
                                      size_t *result_count) {
  if (str == NULL) {
    return NULL;
  }

  PCRE2_SPTR8 subject = (PCRE2_SPTR8)str;
  PCRE2_SPTR8 pattern = (PCRE2_SPTR8) "(?s)<code>.*?</code>(*SKIP)(*FAIL)|"
                                      "\\[([^\\]]+)\\]\\ ?\\[([^\\]]+)\\]";

  int errorcode;
  PCRE2_SIZE erroroffset;

  pcre2_code_8 *re =
      pcre2_compile_8(pattern, PCRE2_ZERO_TERMINATED, PCRE2_UTF | PCRE2_UCP,
                      &errorcode, &erroroffset, NULL);
  if (!re) {
    PCRE2_UCHAR8 buffer[256];
    pcre2_get_error_message_8(errorcode, buffer, sizeof(buffer));
    LOGF("PCRE2 compilation failed at offset %zu: %s\n", erroroffset, buffer);
    return NULL;
  }
  /* Prepare match data block */
  pcre2_match_data_8 *md = pcre2_match_data_create_from_pattern_8(re, NULL);
  PCRE2_SIZE subject_len = strlen((char *)subject);
  PCRE2_SIZE offset = 0;
  int rc;

  // Setup dynamic array for storing MDLinkRegex output
  size_t capacity = 2;
  size_t count = 0;
  MDLinkRegex *arr = malloc(capacity * sizeof(MDLinkRegex));
  if (!arr) {
    perror("malloc MDLinkRegex failed");
    pcre2_match_data_free_8(md);
    pcre2_code_free_8(re);
    return NULL;
  }

  // Loop over matches
  while ((rc = pcre2_match_8(re, subject, subject_len, offset, 0, md, NULL)) >=
         0) {
    // [0]..[1] = full-match start/end, [2]..[3]=group1, [4]..[5]=group2
    PCRE2_SIZE *ov = pcre2_get_ovector_pointer_8(md);

    // Checking escape syntax
    // label part
    if (ov[2] >= 1 && subject[ov[2] - 1] == '[') {
      if (is_escaped_at((char *)subject, ov[2] - 1)) {
        offset = ov[1];
        continue;
      }
    }
    if (subject[ov[3]] == ']') {
      if (is_escaped_at((char *)subject, ov[3])) {
        offset = ov[1];
        continue;
      }
    }
    // tag part
    if (ov[4] >= 1 && subject[ov[4] - 1] == '[') {
      if (is_escaped_at((char *)subject, ov[4] - 1)) {
        offset = ov[1];
        continue;
      }
    }
    if (subject[ov[5]] == ']') {
      if (is_escaped_at((char *)subject, ov[5])) {
        offset = ov[1];
        continue;
      }
    }

    // Grow dynamic array
    if (count == capacity) {
      capacity *= 2;
      MDLinkRegex *tmp = realloc(arr, capacity * sizeof(*arr));
      if (!tmp) {
        perror("realloc");
        break;
      }
      arr = tmp;
    }

    // Extract captured info
    char *label = malloc(ov[3] - ov[2] + 1);
    char *tag = malloc(ov[5] - ov[4] + 1);
    memcpy(label, subject + ov[2], ov[3] - ov[2]);
    memcpy(tag, subject + ov[4], ov[5] - ov[4]);
    label[ov[3] - ov[2]] = '\0';
    tag[ov[5] - ov[4]] = '\0';

    MDLinkReference *ref = find_link_reference(head, tag);
    if (ref != NULL) {
      MDLinkRegex *link = new_md_link(label, ref->url, ref->title, NULL,
                                      (int)ov[0], (int)ov[1]);
      arr[count] = *link;
      free(link);

      count++;
    }

    free(label);
    free(tag);

    offset = ov[1];
  }

  pcre2_match_data_free_8(md);
  pcre2_code_free_8(re);

  *result_count = count;
  return arr;
}

MDLinkReference *parse_markdown_links_reference(char *str) {
  if (str == NULL) {
    return NULL;
  }

  PCRE2_SPTR8 pattern =
      (PCRE2_SPTR8) "(?s)<code>.*?</code>(*SKIP)(*FAIL)|"
                    "^[ \\t]*" // optional leading space/tabs
                    "\\[([A-Za-z0-9 "
                    "!\"#$%&'()*+,\\-./:;<=>?@[\\\\\\]^_`{|}~]+)\\]" // 1:
                                                                     // reference
                                                                     // label
                    ":[ \\t]+"        // literal “:” plus optional space/tabs
                    "<?([^> \\t]+)>?" // 2: URL, with optional < >
                    "(?:[ \\t]+"      // if there’s extra stuff, then…
                    "(?:"
                    "\"([^\"]*)\""   // 3: title in double‐quotes
                    "|'([^']*)'"     // 4: title in single‐quotes
                    "|\\(([^)]*)\\)" // 5: title in parentheses
                    ")"
                    ")?"
                    "[ \\t]*$"; // optional trailing space/tabs

  int errorcode;
  PCRE2_SIZE erroroffset;

  // Compile with UTF and Unicode-property support
  pcre2_code_8 *re =
      pcre2_compile_8(pattern, PCRE2_ZERO_TERMINATED, PCRE2_UTF | PCRE2_UCP,
                      &errorcode, &erroroffset, NULL);
  if (!re) {
    PCRE2_UCHAR8 buffer[256];
    pcre2_get_error_message_8(errorcode, buffer, sizeof(buffer));
    LOGF("PCRE2 compilation failed at offset %zu: %s\n", erroroffset, buffer);
    return NULL;
  }

  pcre2_match_data_8 *md = pcre2_match_data_create_from_pattern_8(re, NULL);
  PCRE2_SIZE str_len = strlen(str);
  PCRE2_SIZE offset = 0;
  int rc;

  if ((rc = pcre2_match_8(re, (PCRE2_SPTR8)str, str_len, offset, 0, md, NULL)) <
      0) {
    pcre2_match_data_free_8(md);
    pcre2_code_free_8(re);
    return NULL; // No match found
  }

  PCRE2_SIZE *ov = pcre2_get_ovector_pointer_8(md);

  // Checking escape syntax
  // label part
  PCRE2_SPTR8 subject = (PCRE2_SPTR8)str;
  if (ov[2] >= 1 && subject[ov[2] - 1] == '[') {
    if (is_escaped_at((char *)subject, ov[2] - 1)) {
      return NULL;
    }
  }
  if (subject[ov[3]] == ']') {
    if (is_escaped_at((char *)subject, ov[3])) {
      return NULL;
    }
  }
  // url part
  if (ov[4] >= 1 && subject[ov[4] - 1] == '<') {
    if (is_escaped_at((char *)subject, ov[4] - 1)) {
      return NULL;
    }
  }
  if (subject[ov[5]] == '>') {
    if (is_escaped_at((char *)subject, ov[5])) {
      return NULL;
    }
  }
  // title part
  if (ov[6] != PCRE2_UNSET && ov[7] != PCRE2_UNSET) {
    if (ov[6] >= 1 &&
        (subject[ov[6] - 1] == '"' || subject[ov[6] - 1] == '\'' ||
         subject[ov[6] - 1] == '(')) {
      if (is_escaped_at((char *)subject, ov[6] - 1)) {
        return NULL;
      }
    }
    if (subject[ov[7]] == '"' || subject[ov[7]] == '\'' ||
        subject[ov[7]] == ')') {
      if (is_escaped_at((char *)subject, ov[7])) {
        return NULL;
      }
    }
  }

  char *label = malloc(ov[3] - ov[2] + 1);
  char *url = malloc(ov[5] - ov[4] + 1);
  memcpy(label, str + ov[2], ov[3] - ov[2]);
  memcpy(url, str + ov[4], ov[5] - ov[4]);
  label[ov[3] - ov[2]] = '\0';
  url[ov[5] - ov[4]] = '\0';
  str_to_lower(label); // Normalize label to lowercase

  char *title = NULL;
  if (rc >= 4 && ov[7] != PCRE2_UNSET) {
    title = malloc(ov[7] - ov[6] + 1);
    memcpy(title, str + ov[6], ov[7] - ov[6]);
    title[ov[7] - ov[6]] = '\0';
  }

  MDLinkReference *ref = new_md_link_reference(label, url, title);

  pcre2_match_data_free_8(md);
  pcre2_code_free_8(re);

  free(label);
  free(url);
  if (title) {
    free(title);
  }

  return ref;
}

// parsing url and email addresses that is enclose in angle brackets.
// For example: <https://www.markdownguide.org> or <fake@example.com>
MDLinkRegex *parse_simple_addresses(const char *str, size_t *result_count) {
  if (str == NULL) {
    return NULL;
  }

  PCRE2_SPTR8 subject = (PCRE2_SPTR8)str;
  PCRE2_SPTR8 pattern =
      (PCRE2_SPTR8) "(?s)<code>.*?</code>(*SKIP)(*FAIL)|"
                    "<((https?://"
                    "[^\\s<>]+)|([a-zA-Z0-9._%+-]+@[a-zA-Z0-9."
                    "-]+\\.[a-zA-Z]{2,}))>";

  int errorcode;
  PCRE2_SIZE erroroffset;

  // Compile with UTF and Unicode-property support
  pcre2_code_8 *re =
      pcre2_compile_8(pattern, PCRE2_ZERO_TERMINATED, PCRE2_UTF | PCRE2_UCP,
                      &errorcode, &erroroffset, NULL);
  if (!re) {
    PCRE2_UCHAR8 buffer[256];
    pcre2_get_error_message_8(errorcode, buffer, sizeof(buffer));
    LOGF("PCRE2 compilation failed at offset %zu: %s\n", erroroffset, buffer);
    return NULL;
  }

  /* Prepare match data block */
  pcre2_match_data_8 *md = pcre2_match_data_create_from_pattern_8(re, NULL);
  PCRE2_SIZE subject_len = strlen((char *)subject);
  PCRE2_SIZE offset = 0;
  int rc;

  // Setup dynamic array for storing MDLinkRegex output
  size_t capacity = 2;
  size_t count = 0;
  MDLinkRegex *arr = malloc(capacity * sizeof(MDLinkRegex));
  if (!arr) {
    perror("malloc MDLinkRegex failed");
    pcre2_match_data_free_8(md);
    pcre2_code_free_8(re);
    return NULL;
  }

  // Loop over matches
  while ((rc = pcre2_match_8(re, subject, subject_len, offset, 0, md, NULL)) >=
         0) {
    // [0]..[1] = full-match start/end, [2]..[3]=group1, [4]..[5]=group2,
    // [6]..[7]=group3
    PCRE2_SIZE *ov = pcre2_get_ovector_pointer_8(md);

    // Checking escape syntax
    if (ov[2] >= 1 && subject[ov[2] - 1] == '<') {
      if (is_escaped_at((char *)subject, ov[2] - 1)) {
        offset = ov[1];
        continue; // Skip escaped links
      }
    }
    if (subject[ov[3]] == '>') {
      if (is_escaped_at((char *)subject, ov[3])) {
        offset = ov[1];
        continue; // Skip escaped links
      }
    }

    // Grow dynamic array
    if (count == capacity) {
      capacity *= 2;
      MDLinkRegex *tmp = realloc(arr, capacity * sizeof(*arr));
      if (!tmp) {
        perror("realloc");
        break;
      }
      arr = tmp;
    }

    // Extract captured info
    char *label = malloc(ov[3] - ov[2] + 1);
    char *url = NULL;
    if (ov[6] != PCRE2_UNSET && ov[7] != PCRE2_UNSET) {
      // Email address matched
      // Plus 7 for "mailto:" prefix
      url = malloc(ov[3] - ov[2] + 7 + 1);
      memcpy(url, "mailto:", 7);
      memcpy(url + 7, subject + ov[2], ov[3] - ov[2]);
      url[ov[3] - ov[2] + 7] = '\0';
    } else {
      // URL matched
      url = malloc(ov[3] - ov[2] + 1);
      memcpy(url, subject + ov[2], ov[3] - ov[2]);
      url[ov[3] - ov[2]] = '\0';
    }
    char *title = NULL;
    memcpy(label, subject + ov[2], ov[3] - ov[2]);
    label[ov[3] - ov[2]] = '\0';

    MDLinkRegex *link = new_md_link(label, url, title, NULL, (int)(ov[2] - 1),
                                    (int)(ov[3] + 1));
    arr[count] = *link;
    free(link);

    count++;

    free(label);
    free(url);

    offset = ov[1];
  }

  pcre2_match_data_free_8(md);
  pcre2_code_free_8(re);

  *result_count = count;
  return arr;
}

MDLinkRegex *parse_markdown_images(const char *str, size_t *result_count) {
  if (str == NULL) {
    return NULL;
  }

  PCRE2_SPTR8 subject = (PCRE2_SPTR8)str;
  PCRE2_SPTR8 pattern =
      (PCRE2_SPTR8) "!\\[(.*?)\\]\\((\\S+?)(?:\\s+\"([^\"]+)\")?\\)";

  int errorcode;
  PCRE2_SIZE erroroffset;

  // Compile with UTF and Unicode-property support
  pcre2_code_8 *re =
      pcre2_compile_8(pattern, PCRE2_ZERO_TERMINATED, PCRE2_UTF | PCRE2_UCP,
                      &errorcode, &erroroffset, NULL);
  if (!re) {
    PCRE2_UCHAR8 buffer[256];
    pcre2_get_error_message_8(errorcode, buffer, sizeof(buffer));
    LOGF("PCRE2 compilation failed at offset %zu: %s\n", erroroffset, buffer);
    return NULL;
  }

  /* Prepare match data block */
  pcre2_match_data_8 *md = pcre2_match_data_create_from_pattern_8(re, NULL);
  PCRE2_SIZE subject_len = strlen((char *)subject);
  PCRE2_SIZE offset = 0;
  int rc;

  // Setup dynamic array for storing MDLinkRegex output
  size_t capacity = 2;
  size_t count = 0;
  MDLinkRegex *arr = malloc(capacity * sizeof(MDLinkRegex));
  if (!arr) {
    perror("malloc MDLinkRegex failed");
    pcre2_match_data_free_8(md);
    pcre2_code_free_8(re);
    return NULL;
  }

  // Loop over matches
  while ((rc = pcre2_match_8(re, subject, subject_len, offset, 0, md, NULL)) >=
         0) {
    // [0]..[1] = full-match start/end, [2]..[3]=group1, [4]..[5]=group2,
    // [6]..[7]=group3
    PCRE2_SIZE *ov = pcre2_get_ovector_pointer_8(md);

    // Checking escape syntax
    // Checking starting `!`
    if (ov[0] >= 1) {
      if (is_escaped_at((char *)subject, ov[0])) {
        offset = ov[1];
        continue;
      }
    }
    // Checking label part
    if (ov[2] >= 1 && subject[ov[2] - 1] == '[') {
      if (is_escaped_at((char *)subject, ov[2] - 1)) {
        offset = ov[1];
        continue;
      }
    }
    if (subject[ov[3]] == ']') {
      if (is_escaped_at((char *)subject, ov[3])) {
        offset = ov[1];
        continue;
      }
    }
    // Checking src part
    if (ov[4] >= 1 && subject[ov[4] - 1] == '(') {
      if (is_escaped_at((char *)subject, ov[4] - 1)) {
        offset = ov[1];
        continue;
      }
    }
    if (subject[ov[5]] == ')') {
      if (is_escaped_at((char *)subject, ov[5])) {
        offset = ov[1];
        continue;
      }
    }
    // Checking title part
    if (ov[6] != PCRE2_UNSET && ov[7] != PCRE2_UNSET) {
      if (ov[6] >= 1 && subject[ov[6] - 1] == '"') {
        if (is_escaped_at((char *)subject, ov[6] - 1)) {
          offset = ov[1];
          continue;
        }
      }
      if (subject[ov[7]] == '"') {
        if (is_escaped_at((char *)subject, ov[7])) {
          offset = ov[1];
          continue;
        }
      }
    }

    // Grow dynamic array
    if (count == capacity) {
      capacity *= 2;
      MDLinkRegex *tmp = realloc(arr, capacity * sizeof(*arr));
      if (!tmp) {
        perror("realloc");
        break;
      }
      arr = tmp;
    }

    // Extract captured info
    char *label = NULL;
    char *src = NULL;
    char *url = NULL;
    char *title = NULL;
    label = malloc(ov[3] - ov[2] + 1);
    src = malloc(ov[5] - ov[4] + 1);
    memcpy(label, subject + ov[2], ov[3] - ov[2]);
    label[ov[3] - ov[2]] = '\0';
    memcpy(src, subject + ov[4], ov[5] - ov[4]);
    src[ov[5] - ov[4]] = '\0';
    if (ov[6] != PCRE2_UNSET && ov[7] != PCRE2_UNSET) {
      // Has title
      title = malloc(ov[7] - ov[6] + 1);
      memcpy(title, subject + ov[6], ov[7] - ov[6]);
      title[ov[7] - ov[6]] = '\0';
    }

    MDLinkRegex *link =
        new_md_link(label, url, title, src, (int)(ov[0]), (int)(ov[1]));
    arr[count] = *link;
    free(link);

    count++;

    free(label);
    free(url);
    free(title);
    free(src);

    offset = ov[1];
  }

  pcre2_match_data_free_8(md);
  pcre2_code_free_8(re);

  *result_count = count;
  return arr;
}

MDLinkReference *gen_markdown_link_reference_list(PeekReader *reader) {
  if (reader == NULL) {
    return NULL;
  }

  MDLinkReference *head = NULL;
  do {
    char *line = peek_reader_current(reader);

    MDLinkReference *ref = parse_markdown_links_reference(line);
    if (ref == NULL) {
      continue;
    }

    if (head == NULL) {
      head = ref; // First link reference
    } else {
      ref->next = head;
      head = ref;
    }

  } while (peek_reader_advance(reader));

  return head;
}

MDLinkReference *find_link_reference(MDLinkReference *head, char *label) {
  if (head == NULL || label == NULL) {
    return NULL;
  }

  str_to_lower(label); // Normalize label to lowercase

  MDLinkReference *current = head;
  while (current) {
    if (strcmp(current->label, label) == 0) {
      return current; // Found the reference
    }
    current = current->next;
  }
  return NULL; // Not found
}

MDLinkRegex *new_md_link(const char *label, const char *url, const char *title,
                         const char *src, int start, int end) {
  MDLinkRegex *link = malloc(sizeof(MDLinkRegex));
  if (!link) {
    perror("malloc MDLinkRegex failed");
    return NULL;
  }

  link->label = strdup(label);
  link->url = url ? strdup(url) : NULL;
  link->title = title ? strdup(title) : NULL;
  link->src = src ? strdup(src) : NULL;
  link->start = start;
  link->end = end;

  return link;
}

void free_md_links(MDLinkRegex *links, size_t count) {
  if (links == NULL) {
    return;
  }

  for (size_t i = 0; i < count; i++) {
    LOGF("freeing link %zu\n", i);
    LOGF("label\n");
    if (links[i].label) {
      free(links[i].label);
    }
    LOGF("url\n");
    if (links[i].url) {
      free(links[i].url);
    }
    LOGF("title\n");
    if (links[i].title) {
      free(links[i].title);
    }
    LOGF("src\n");
    if (links[i].src) {
      free(links[i].src);
    }
  }
  LOGF("free links\n");
  free(links);
}

MDLinkReference *new_md_link_reference(const char *label, const char *url,
                                       const char *title) {
  MDLinkReference *ref = malloc(sizeof(MDLinkReference));
  if (!ref) {
    perror("malloc MDLinkReference failed");
    return NULL;
  }

  ref->label = strdup(label);
  ref->url = strdup(url);
  ref->title = title ? strdup(title) : NULL;
  ref->next = NULL;

  return ref;
}

void print_md_links_reference(MDLinkReference *head) {
  MDLinkReference *current = head;
  while (current) {
    LOGF("Label: %s, URL: %s", current->label, current->url);
    if (current->title) {
      LOGF(", Title: %s", current->title);
    }
    LOGF("\n");
    current = current->next;
  }
}

void free_md_link_reference(MDLinkReference *head) {
  MDLinkReference *current = head;
  while (current) {
    MDLinkReference *next = current->next;
    free(current->label);
    free(current->url);
    free(current->title);
    free(current);
    current = next;
  }
}

void str_to_lower(char *str) {
  if (str == NULL) {
    return;
  }

  for (char *p = str; *p; p++) {
    *p = tolower((unsigned char)*p);
  }
}

bool is_escaped_at(const char *str, PCRE2_SIZE pos) {
  size_t backslashs = 0;
  while (pos > 0 && str[pos - 1] == '\\') {
    backslashs++;
    pos--;
  }

  return (backslashs % 2) == 1; // Odd number of backslashes means escaped
}

bool is_escaped(const char *str, char start_target, char end_target,
                PCRE2_SIZE start, PCRE2_SIZE end) {
  if (str == NULL || start >= end) {
    return false;
  }

  // Check if the target before the start position is escaped
  if (start >= 1 && str[start - 1] == start_target) {
    if (is_escaped_at(str, start - 1)) {
      return true;
    }
  }

  // Check if the target after the end position is escaped
  if (str[end] == end_target) {
    if (is_escaped_at(str, end)) {
      return true;
    }
  }

  return false;
}

static int cmp_md_link_start(const void *a, const void *b) {
  MDLinkRegex *linkA = (MDLinkRegex *)a;
  MDLinkRegex *linkB = (MDLinkRegex *)b;
  return linkA->start - linkB->start;
}

#ifdef TEST_MD_REGEX
int main(void) {
  // Test parse markdown links reference
  char *lines[] = {
      "[1]: https://example.com \"Example Link\"",
      "[2]: https://example.org",
      "[3]: https://example.net \"Another Example\"",
      "[4]:https://example.com/another \"Another Link\"",
      "[5]: <https://en.wikipedia.org/wiki/Hobbit#Lifestyle> 'Hobbit "
      "lifestyles'",
      "[abcde, ]: <https://en.wikipedia.org/wiki/Hobbit#Lifestyle> (Hobbit "
      "lifestyles)",
      "[UPPERCASE label]: <https://en.wikipedia.org/wiki/Hobbit#Lifestyle> "
      "(Hobbit lifestyles)",
      "[ddg]: https://duckduckgo.com \"The best search engine for privacy\"",
      "[google]: https://www.google.com \"Google Search Engine\"",
  };
  PeekReader *reader = new_peek_reader_from_lines(lines, 9, DEFAULT_PEEK_COUNT);
  if (!reader) {
    fprintf(stderr, "Failed to create peek reader\n");
    return 1;
  }

  MDLinkReference *ref_head = gen_markdown_link_reference_list(reader);
  print_md_links_reference(ref_head);

  printf("\n");

  /* Subject with two markdown links */
  PCRE2_SPTR8 subject =
      (PCRE2_SPTR8) "A privacy-focused link: [Duck Duck "
                    "Go](https://duckduckgo.com \"The best search engine for "
                    "privacy\")\n"
                    "And a bare link:          [Duck Duck "
                    "Go](https://duckduckgo.com \"最好的瀏覽器\")\n"
                    "<code>[Duck Duck Go](https://duckduckgo.com \"The best "
                    "search engine for privacy\")</code>\n"
                    "<code>The browser [Duck Duck Go](https://duckduckgo.com "
                    "\"The best "
                    "search engine for privacy\")</code> haha\n";

  size_t link_count = 0;
  MDLinkRegex *links =
      parse_markdown_links(ref_head, (const char *)subject, &link_count);

  for (size_t i = 0; i < link_count; i++) {
    printf("Link %zu:\n", i + 1);
    printf("  Label : \"%s\"\n", links[i].label);
    printf("  URL   : \"%s\"\n", links[i].url);
    if (links[i].title) {
      printf("  Title : \"%s\"\n", links[i].title);
    } else {
      printf("  Title : (none)\n");
    }
    printf("  Start : %d\n", links[i].start);
    printf("  End: %d\n", links[i].end);
    // free(links[i].label);
    // free(links[i].url);
    // free(links[i].title);
  }

  printf("\n");

  // Test parse_markdown_links_tag
  const char *tag_subject = "This is a test for markdown links with tags:\n"
                            "[Duck Duck Go][ddg]\n"
                            "[Google] [google]\n"
                            "[Error link with no reference][error]\n";

  link_count = 0;
  links = parse_markdown_links(ref_head, tag_subject, &link_count);

  for (size_t i = 0; i < link_count; i++) {
    printf("Link %zu:\n", i + 1);
    printf("  Label : \"%s\"\n", links[i].label);
    printf("  URL   : \"%s\"\n", links[i].url);
    if (links[i].title) {
      printf("  Title : \"%s\"\n", links[i].title);
    } else {
      printf("  Title : (none)\n");
    }
    printf("  Start : %d\n", links[i].start);
    printf("  End: %d\n", links[i].end);
    // free(links[i].label);
    // free(links[i].url);
    // free(links[i].title);
  }

  printf("\n");

  // Test parsing simple addresses
  const char *simple_subject = "Some text with <https://www.markdownguide.org> "
                               "and <fake@example.com> inside. "
                               "<www.invalid.com>, <fake#invalid.com>";

  link_count = 0;
  links = parse_markdown_links(ref_head, simple_subject, &link_count);
  for (size_t i = 0; i < link_count; i++) {
    printf("Link %zu:\n", i + 1);
    printf("  Label : \"%s\"\n", links[i].label);
    printf("  URL   : \"%s\"\n", links[i].url);
    if (links[i].title) {
      printf("  Title : \"%s\"\n", links[i].title);
    } else {
      printf("  Title : (none)\n");
    }
    printf("  Start : %d\n", links[i].start);
    printf("  End: %d\n", links[i].end);
    // free(links[i].label);
    // free(links[i].url);
    // free(links[i].title);
  }

  printf("\n");

  // Test parsing images
  const char *image_subject =
      "![Alt text](https://example.com/image.png \"Image Title\")\n"
      "![Linked Image](https://example.com/linked-image.png \"Linked "
      "Image Title\")\n"
      "[![Linked Image](https://example.com/linked-image.png \"Linked "
      "Image Title\")](https://example.com/linked-image-url)\n"
      "With some text ![Alt text](https://example.com/image.png \"Image "
      "Title\") between\n"
      "![Alt text](https://example.com/image.png \"Image Title\") multiple "
      "images link ![Alt text](https://example.com/image.png \"Image "
      "Title\")\n";

  link_count = 0;
  links = parse_markdown_images(image_subject, &link_count);
  for (size_t i = 0; i < link_count; i++) {
    printf("Link %zu:\n", i + 1);
    printf("  Label : \"%s\"\n", links[i].label);
    printf("  Src   : \"%s\"\n", links[i].src);
    if (links[i].url) {
      printf("  Url   : \"%s\"\n", links[i].url);
    } else {
      printf("  Url   : (none)\n");
    }
    if (links[i].title) {
      printf("  Title : \"%s\"\n", links[i].title);
    } else {
      printf("  Title : (none)\n");
    }
    printf("  Start : %d\n", links[i].start);
    printf("  End: %d\n", links[i].end);
    // free(links[i].label);
    // free(links[i].url);
    // free(links[i].title);
  }

  return 0;
}
#endif // TEST_MD_REGEX
