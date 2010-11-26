
CPPFLAGS = -Isrc
all:
	gcc -Wall $(CPPFLAGS) -g -std=c99 -o test src/arc.c test.c
