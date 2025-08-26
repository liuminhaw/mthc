#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

extern FILE *dbg_log;

void log_init(bool debug);
void log_close();

#define LOGF(...)               \
  do {                                      \
    fprintf(dbg_log ? dbg_log : stderr, __VA_ARGS__); \
  } while (0)

#endif
