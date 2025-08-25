CC = gcc
DEBUG ?= 0

ifeq ($(DEBUG),1)
	CFLAGS = -g
else
	CFLAGS = 
endif

BINARY = mthc
OBJS = main.o md_parser.o file_reader.o debug.o str_utils.o md_regex.o
FLAG_FILE = .build_flags

SHELL := /bin/bash

# ----------------------------
# HELPERS
# ----------------------------

## help: print this help message
.PHONY: help
help:
	@echo "Usage:"
	@sed -n 's/^##//p' $(MAKEFILE_LIST) | column -t -s ':' | sed -e 's/^/ /'

.PHONY: check-flags
check-flags:
	@if [ -f $(FLAG_FILE) ]; then \
		if ! grep -qx -- '$(CFLAGS)' $(FLAG_FILE); then \
			echo "CFLAGS changed. Rebuilding..."; \
			rm -f $(OBJS); \
		fi \
	fi; \
	echo '$(CFLAGS)' > $(FLAG_FILE)

# ---------------------------
# BUILD
# ---------------------------

# all: check-flags mthc

## mthc: build the binary executable of mthc
mthc: check-flags $(OBJS)
	$(CC) $(CFLAGS) -o $(BINARY) $(OBJS) -lunistring -lpcre2-8

debug.o: debug.c debug.h
	$(CC) $(CFLAGS) -c debug.c

file_reader.o: file_reader.c file_reader.h
	$(CC) $(CFLAGS) -c file_reader.c

md_parser.o: md_parser.c md_parser.h
	$(CC) $(CFLAGS) -c md_parser.c

str_utils.o: str_utils.c str_utils.h
	$(CC) $(CFLAGS) -c str_utils.c

md_regex.o: md_regex.c md_regex.h
	$(CC) $(CFLAGS) -c md_regex.c -Wall

main.o: main.c md_parser.h file_reader.h str_utils.h md_regex.h
	$(CC) $(CFLAGS) -c main.c

## debug: build binary with debugging information
.PHONY: debug
debug:
	@$(MAKE) DEBUG=1 mthc

## clean: remove all binary built
.PHONY: clean
clean:
	@echo "Cleaning up..."
	@rm -f *.o mthc str_utils md_regex
	@echo "Done."

# --------------------------
# TESTING
# --------------------------

## check: executing tests files to confirm actual output with expected output
.PHONY: check
check:
	@echo "Running project check with ARGS='$(ARGS)'..."
	@./scripts/check.sh $(ARGS)

## mem-check: use valgrind to on tests files to check for memory leaks
.PHONY: mem-check
mem-check:
	@echo "Running memory leak check..."
	@./scripts/mem_leak_check.sh

# --------------------------
# DEVELOPMENT
# --------------------------

## str_utils: build str_utils binary for functional checking
.PHONY: str_utils
str_utils: str_utils.c str_utils.h
	$(CC) -DTEST_STR_UTILS -o str_utils_test str_utils.c -lunistring

## md_regex: build md_regex binary for functional checking
.PHONY: md_regex
md_regex: md_regex.c md_regex.h file_reader.o
	$(CC) -DTEST_MD_REGEX -o md_regex_test md_regex.c file_reader.o -Wall -lpcre2-8
