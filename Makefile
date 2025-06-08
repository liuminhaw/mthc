CC = gcc

debug.o: debug.c debug.h
	$(CC) -c debug.c

file_reader.o: file_reader.c file_reader.h
	$(CC) -c file_reader.c

md_parser.o: md_parser.c md_parser.h
	$(CC) -c md_parser.c

str_utils.o: str_utils.c str_utils.h
	$(CC) -c str_utils.c

main.o: main.c md_parser.h file_reader.h str_utils.h
	$(CC) -c main.c

mthc: main.o md_parser.o file_reader.o debug.o str_utils.o
	$(CC) -o mthc main.o md_parser.o file_reader.o debug.o str_utils.o -lunistring

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

str_utils:
	$(CC) -DTEST_STR_UTILS -o str_utils str_utils.c -lunistring



