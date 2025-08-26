#include "logger.h"

FILE *dbg_log = NULL;

void log_init(bool debug) {
  if (debug) {
    dbg_log = stderr;
  } else {
    dbg_log = fopen("/dev/null", "w");
    if (dbg_log == NULL) {
      perror("log_init: fopen failed");
      exit(1);
    }
  }
}

void log_close() {
  if (dbg_log != NULL && dbg_log != stderr) {
    fclose(dbg_log);
  }
  dbg_log = NULL;
}
