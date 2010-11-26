
CPPFLAGS = -Isrc
all:
	gcc -Wall $(CPPFLAGS) -g -std=c99 -o arc src/arc.c main.c
