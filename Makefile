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

.PHONY: check
SHELL := /bin/bash

check:
	@echo "Running project check..."
	@./scripts/check.sh
