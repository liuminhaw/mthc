#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char **content_splitter(const char *content, char splitter, int *split_count) {
  int count = 0;
  int capacity = MTHC_SPLITTER_CAP;
  char **lines = malloc(capacity * sizeof(char *));
  if (!lines) {
    perror("malloc failed");
    return NULL;
  }

  const char *start = content;
  const char *end;

  while ((end = strchr(start, splitter)) != NULL) {
    size_t len = end - start; // len of current line

    char *line = malloc(len + 1);
    if (!line) {
      perror("malloc failed");
      for (int i = 0; i < count; i++) {
        free(lines[i]);
      }
      return NULL;
    }
    memcpy(line, start, len);
    line[len] = '\0';

    if (count >= capacity) {
      capacity *= 2;
      char **tmp_lines = realloc(lines, capacity * sizeof(char *));
      if (!tmp_lines) {
        perror("realloc failed");
        for (int i = 0; i < count; i++) {
          free(lines[i]);
        }
        free(lines);
        return NULL;
      }
      lines = tmp_lines;
    }
    lines[count++] = line;

    start = end + 1;
  }

  // Process the final segment if the string does not end with the splitter
  if (*start != '\0') {
    size_t len = strlen(start);
    char *line = malloc(len + 1);
    if (!line) {
      perror("malloc failed");
      for (int i = 0; i < count; i++) {
        free(lines[i]);
      }
      free(lines);
      return NULL;
    }
    memcpy(line, start, len);
    line[len] = '\0';

    if (count >= capacity) {
      capacity *= 2;
      char **tmp_lines = realloc(lines, capacity * sizeof(char *));
      if (!tmp_lines) {
        perror("realloc failed");
        for (int i = 0; i < count; i++) {
          free(lines[i]);
        }
        free(lines);
        return NULL;
      }
      lines = tmp_lines;
    }
    lines[count++] = line;
  }

  *split_count = count;
  return lines;
}

char *ltrim_space(char *str) {
  char *p = str;
  // skip leading whitespace
  while (*p && isspace((unsigned char)*p)) {
    p++;
  }

  char *trimmed_str = malloc(strlen(p) + 1);
  if (!trimmed_str) {
    perror("malloc failed");
    return NULL;
  }
  strcpy(trimmed_str, p);

  return trimmed_str;
}

PeekReader *new_peek_reader_from_file(FILE *fp, int peek_count) {
  if (peek_count > MAX_PEEK) {
    return NULL;
  }

  PeekReader *reader = malloc(sizeof(PeekReader));
  if (!reader) {
    return NULL;
  }

  reader->source_type = PEEK_SOURCE_FILE;
  reader->source.fp = fp;
  reader->current = 0;
  reader->count = 0;
  reader->total = peek_count + 1;

  for (int i = 0; i < reader->total; i++) {
    reader->buffer[i] = read_line(reader->source.fp, true);
    if (reader->buffer[i]) {
      reader->count++;
    }
  }

  return reader;
}

PeekReader *new_peek_reader_from_lines(char **lines, int total_lines,
                                       int peek_count) {
  if (peek_count > MAX_PEEK) {
    return NULL;
  }

  PeekReader *reader = malloc(sizeof(PeekReader));
  if (!reader) {
    return NULL;
  }

  reader->source_type = PEEK_SOURCE_STRING_ARRAY;
  reader->source.str_array.lines = lines;
  reader->source.str_array.total_lines = total_lines;
  reader->source.str_array.line_idx = 0;
  reader->current = 0;
  reader->count = 0;
  reader->total = peek_count + 1;

  for (int i = 0; i < reader->total; i++) {
    if (reader->source.str_array.line_idx < total_lines) {
      reader->buffer[i] =
          reader->source.str_array.lines[reader->source.str_array.line_idx];
      reader->source.str_array.line_idx++;
      reader->count++;
    } else {
      reader->buffer[i] = NULL;
    }
  }

  return reader;
}

char *peek_reader_current(PeekReader *reader) {
  return reader->buffer[reader->current];
}

char *peek_reader_peek(PeekReader *reader, int i) {
  if (i < 0 || i >= reader->total) {
    return NULL;
  }

  int idx = (reader->current + i) % reader->total;

  return reader->buffer[idx];
}

int peek_reader_advance(PeekReader *reader) {
  int pos = reader->current;

  if (reader->source_type == PEEK_SOURCE_FILE) {
    reader->buffer[pos] = read_line(reader->source.fp, true);
    if (!reader->buffer[pos]) {
      reader->count--;
    }
  } else if (reader->source_type == PEEK_SOURCE_STRING_ARRAY) {
    if (reader->source.str_array.line_idx <
        reader->source.str_array.total_lines) {
      reader->buffer[pos] =
          reader->source.str_array.lines[reader->source.str_array.line_idx];
      reader->source.str_array.line_idx++;
    } else {
      reader->buffer[pos] = NULL;
      reader->count--;
    }
  }

  // move current forward
  reader->current = (reader->current + 1) % reader->total;

  return reader->count > 0;
}
