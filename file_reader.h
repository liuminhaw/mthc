#include <stdio.h>
#include <stdbool.h>

#ifndef FILE_READER_H
#define FILE_READER_H

#define BUFFER_CHUNK_SIZE 256

#endif

char *read_line(FILE *file, bool remove_newline);
