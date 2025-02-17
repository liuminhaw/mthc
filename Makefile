CC = gcc

md_parser.o: md_parser.c md_parser.h
	$(CC) -c md_parser.c

main.o: main.c md_parser.h
	$(CC) -c main.c

mthc: main.o md_parser.o
	$(CC) -o mthc main.o md_parser.o
