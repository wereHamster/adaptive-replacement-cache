
all:
	gcc -I. -g -ggdb -O0 -std=c99 -o arc arc.c main.c