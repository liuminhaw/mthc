CC = gcc
DEBUG ?= 0

ifeq ($(DEBUG),1)
	CFLAGS = -g
else
	CFLAGS = 
endif

BINARY = mthc
OBJS = main.o md_parser.o file_reader.o debug.o str_utils.o md_regex.o style_css.o logger.o
FLAG_FILE = .build_flags

# SHELL := /bin/bash

# ----------------------------
# DEFAULT
# ----------------------------

all: mthc

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

## mthc: build the binary executable of mthc
mthc: check-flags $(OBJS)
	$(CC) $(CFLAGS) -o $(BINARY) $(OBJS) -lunistring -lpcre2-8

style_css.o: style_css.c
	$(CC) $(CFLAGS) -c style_css.c

debug.o: debug.c debug.h
	$(CC) $(CFLAGS) -c debug.c

logger.o: logger.c logger.h
	$(CC) $(CFLAGS) -c logger.c

file_reader.o: file_reader.c file_reader.h
	$(CC) $(CFLAGS) -c file_reader.c

md_parser.o: md_parser.c md_parser.h
	$(CC) $(CFLAGS) -c md_parser.c

str_utils.o: str_utils.c str_utils.h
	$(CC) $(CFLAGS) -c str_utils.c

md_regex.o: md_regex.c md_regex.h
	$(CC) $(CFLAGS) -c md_regex.c -Wall

main.o: main.c file_reader.h md_regex.h style_css.h debug.h logger.h
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

## styles: create style_css.c and style_css.h for css style embedding
ASSET_PAIRS := default_dark_css:css/catppuccin-mocha.css default_light_css:css/catppuccin-latte.css
.PHONY: styles
styles: $(foreach p,$(ASSET_PAIRS),$(word 2,$(subst :, ,$(p))))
	@mkdir -p build include
	@echo "/* auto-generated: do not edit */" > style_css.c
	@echo "/* auto-generated: do not edit */" > style_css.h
	@echo "#ifndef STYLE_CSS_H" >> style_css.h
	@echo "#define STYLE_CSS_H" >> style_css.h
	@for p in $(ASSET_PAIRS); do \
	  name=$${p%%:*}; file=$${p#*:}; \
	  echo "/* $$file -> $$name */" >> style_css.c; \
	  xxd -i -n "$$name" "$$file" \
	    | sed -E 's/^unsigned /const unsigned /' >> style_css.c; \
	  echo "extern const unsigned char $$name[];"   >> style_css.h; \
	  echo "extern const unsigned int  $$name"_len";" >> style_css.h; \
	  echo >> style_css.c; \
	done
	@echo "#endif" >> style_css.h

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
str_utils: str_utils.c str_utils.h logger.o
	$(CC) -DTEST_STR_UTILS -o str_utils_test str_utils.c logger.o -lunistring

## md_regex: build md_regex binary for functional checking
.PHONY: md_regex
md_regex: md_regex.c md_regex.h file_reader.o logger.o
	$(CC) -DTEST_MD_REGEX -o md_regex_test md_regex.c file_reader.o logger.o -Wall -lpcre2-8
