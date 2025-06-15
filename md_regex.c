#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#endif

#include <pcre2.h>
#include <stdio.h>
#include <string.h>

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
    printf("  Length: %d\n", links[i].length);
    free(links[i].label);
    free(links[i].url);
    free(links[i].title);
  }

  return 0;
}
#endif // TEST_MD_REGEX
