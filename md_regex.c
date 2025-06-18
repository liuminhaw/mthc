#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#endif

#include <ctype.h>
#include <pcre2.h>
#include <stdio.h>
#include <string.h>

#include "file_reader.h"
#include "md_regex.h"

MDLinkRegex *parse_markdown_links(const char *str, size_t *result_count) {
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
    fprintf(stderr, "PCRE2 compilation failed at offset %zu: %s\n", erroroffset,
            buffer);
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

    arr[count].label = label;
    arr[count].url = url;
    arr[count].title = title;
    arr[count].start = (int)ov[0];
    arr[count].end = (int)ov[1];
    count++;

    offset = ov[1];
  }

  pcre2_match_data_free_8(md);
  pcre2_code_free_8(re);

  *result_count = count;
  return arr;
}

MDLinkReference *parse_markdown_links_reference(PeekReader *reader) {
  if (reader == NULL) {
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
    fprintf(stderr, "PCRE2 compilation failed at offset %zu: %s\n", erroroffset,
            buffer);
    return NULL;
  }

  MDLinkReference *head = NULL;

  do {
    char *line = peek_reader_current(reader);

    pcre2_match_data_8 *md = pcre2_match_data_create_from_pattern_8(re, NULL);
    PCRE2_SIZE line_len = strlen(line);
    PCRE2_SIZE offset = 0;
    int rc;

    if ((rc = pcre2_match_8(re, (PCRE2_SPTR8)line, line_len, offset, 0, md,
                            NULL)) < 0) {
      pcre2_match_data_free_8(md);
      continue; // No match found
    }

    PCRE2_SIZE *ov = pcre2_get_ovector_pointer_8(md);
    char *label = malloc(ov[3] - ov[2] + 1);
    char *url = malloc(ov[5] - ov[4] + 1);
    memcpy(label, line + ov[2], ov[3] - ov[2]);
    memcpy(url, line + ov[4], ov[5] - ov[4]);
    label[ov[3] - ov[2]] = '\0';
    url[ov[5] - ov[4]] = '\0';
    str_to_lower(label); // Normalize label to lowercase

    char *title = NULL;
    if (rc >= 4 && ov[7] != PCRE2_UNSET) {
      title = malloc(ov[7] - ov[6] + 1);
      memcpy(title, line + ov[6], ov[7] - ov[6]);
      title[ov[7] - ov[6]] = '\0';
    }

    MDLinkReference *ref = new_md_link_reference(label, url, title);
    // if (ref != NULL) {
    //   printf("Found reference link: label='%s', url='%s', title='%s'\n",
    //          ref->label, ref->url, ref->title ? ref->title : "(none)");
    // }

    if (head == NULL) {
      head = ref; // First link reference
    } else {
      ref->next = head;
      head = ref;
    }

    free(label);
    free(url);
    free(title);

    pcre2_match_data_free_8(md);

  } while (peek_reader_advance(reader));

  pcre2_code_free_8(re);
  return head;
}

void free_md_links(MDLinkRegex *links, size_t count) {
  if (links == NULL) {
    return;
  }

  for (size_t i = 0; i < count; i++) {
    free(links[i].label);
    free(links[i].url);
    free(links[i].title);
  }
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
    printf("Label: %s, URL: %s", current->label, current->url);
    if (current->title) {
      printf(", Title: %s", current->title);
    }
    printf("\n");
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

#ifdef TEST_MD_REGEX
int main(void) {
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
  MDLinkRegex *links = parse_markdown_links((const char *)subject, &link_count);

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
    free(links[i].label);
    free(links[i].url);
    free(links[i].title);
  }

  printf("\n");

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
      "(Hobbit lifestyles)"};
  PeekReader *reader = new_peek_reader_from_lines(lines, 7, DEFAULT_PEEK_COUNT);
  if (!reader) {
    fprintf(stderr, "Failed to create peek reader\n");
    return 1;
  }

  MDLinkReference *ref_head = parse_markdown_links_reference(reader);
  print_md_links_reference(ref_head);

  return 0;
}
#endif // TEST_MD_REGEX
