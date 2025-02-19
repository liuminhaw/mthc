#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "file_reader.h"

char *read_line(FILE *file, bool remove_newline) {
  size_t size = BUFFER_CHUNK_SIZE;
  size_t len = 0;

  char *buffer = malloc(size);
  if (!buffer) {
    perror("Unable to allocate buffer");
    return NULL;
  }
  
  while (fgets(buffer + len, size - len, file)) {
    len += strlen(buffer + len);

    // Reading of the line is complete
    if (len > 0 && buffer[len - 1] == '\n') {
      if (remove_newline) {
        buffer[len - 1] = '\0';
      }
      return buffer;
    }

    // Buffer size is not enough, double the size and reallocate
    size *= 2;
    char *tmp_buffer = realloc(buffer, size);
    if (!tmp_buffer) {
      free(buffer);
      perror("Unable to reallocate buffer");
      return NULL;
    }
    buffer = tmp_buffer;
  }

  // Nothing was read from the file
  if (len == 0) {
    free(buffer);
    return NULL;
  }

  return buffer;
}
