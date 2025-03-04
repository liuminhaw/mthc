CC = gcc

md_parser.o: md_parser.c md_parser.h
	$(CC) -c md_parser.c

file_reader.o: file_reader.c file_reader.h
	$(CC) -c file_reader.c

main.o: main.c md_parser.h file_reader.h
	$(CC) -c main.c

mthc: main.o md_parser.o file_reader.o
	$(CC) -o mthc main.o md_parser.o file_reader.o
