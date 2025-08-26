#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>
#include "md_parser.h"
#include "logger.h"

char *literal_newline_substitution(char *str);
void traverse_block(MDBlock *block);


#endif
