#ifndef PARSER_H
#define PARSER_H

#define PSR_H1_PATTERN "^#\\s+(.*)$"
#define PSR_H2_PATTERN "^##\\s+(.*)$"
#define PSR_H3_PATTERN "^###\\s+(.*)$"
#define PSR_H4_PATTERN "^####\\s+(.*)$"
#define PSR_H5_PATTERN "^#####\\s+(.*)$"
#define PSR_H6_PATTERN "^######\\s+(.*)$"

#endif

char* heading_parser(char *target_str, char *pattern, int match_count, int target_match);
