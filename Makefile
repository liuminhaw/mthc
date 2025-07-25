CC = gcc

debug.o: debug.c debug.h
	$(CC) -c debug.c

file_reader.o: file_reader.c file_reader.h
	$(CC) -c file_reader.c

md_parser.o: md_parser.c md_parser.h
	$(CC) -c md_parser.c

str_utils.o: str_utils.c str_utils.h
	$(CC) -c str_utils.c

md_regex.o: md_regex.c md_regex.h
	$(CC) -c md_regex.c -Wall

main.o: main.c md_parser.h file_reader.h str_utils.h md_regex.h
	$(CC) -c main.c

mthc: main.o md_parser.o file_reader.o debug.o str_utils.o md_regex.o
	$(CC) -o mthc main.o md_parser.o file_reader.o debug.o str_utils.o md_regex.o -lunistring -lpcre2-8

.PHONY: check clean
SHELL := /bin/bash

check:
	@echo "Running project check with ARGS='$(ARGS)'..."
	@./scripts/check.sh $(ARGS)

clean:
	@echo "Cleaning up..."
	@rm -f *.o mthc str_utils
	@echo "Done."

.PHONY: str_utils

str_utils: str_utils.c str_utils.h
	$(CC) -DTEST_STR_UTILS -o str_utils_test str_utils.c -lunistring

md_regex: md_regex.c md_regex.h file_reader.o
	$(CC) -DTEST_MD_REGEX -o md_regex_test md_regex.c file_reader.o -Wall -lpcre2-8



