CC = gcc

debug.o: debug.c debug.h
	$(CC) -c debug.c

file_reader.o: file_reader.c file_reader.h
	$(CC) -c file_reader.c

md_parser.o: md_parser.c md_parser.h
	$(CC) -c md_parser.c

main.o: main.c md_parser.h file_reader.h
	$(CC) -c main.c

mthc: main.o md_parser.o file_reader.o debug.o
	$(CC) -o mthc main.o md_parser.o file_reader.o debug.o

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
	$(CC) -DTEST_STR_UTILS -o str_utils str_utils.c



