CC = gcc
CC_FLAGS = -Wall -Werror -g


all: test list.o

test: test.c list.o
	$(CC) $(CC_FLAGS) -o test test.c list.o
	./test || rm list.o

list.o: list.c list.h
	$(CC) $(CC_FLAGS) -c list.c


