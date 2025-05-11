#ifndef FILE_READER_H
#define FILE_READER_H

#include <stdio.h>
#include <stdbool.h>

#define BUFFER_CHUNK_SIZE 256
#define MTHC_SPLITTER_CAP 10

char *read_line(FILE *file, bool remove_newline);
char **content_splitter(const char *content, char splitter, int *split_count);
char *ltrim_space(char *str);

#define DEFAULT_PEEK_COUNT 5
#define MAX_PEEK 10

typedef enum {
  PEEK_SOURCE_FILE,
  PEEK_SOURCE_STRING_ARRAY,
} PeekSourceType;

// Using a circular buffer to store each peek lines
typedef struct {
  PeekSourceType source_type;
  union {
    FILE *fp;
    struct {
      char **lines;
      int total_lines;
      int line_idx; // index into string array source type
    } str_array;
  } source;

  char *buffer[MAX_PEEK + 1]; // circular buffer
  int current;
  int count; // number of valid lines in buffer
  int total; // total capacity (PEEK + 1)
} PeekReader;

PeekReader *new_peek_reader_from_file(FILE *fp, int peek_count);
PeekReader *new_peek_reader_from_lines(char **lines, int total_lines,
                                     int peek_count);
// Get current line
char *peek_reader_current(PeekReader *reader);
// Gets the i-th line ahead
char *peek_reader_peek(PeekReader *reader, int i);
// Advances to next line, returns 1 if successful, 0 on EOF
int peek_reader_advance(PeekReader *reader);

#endif


