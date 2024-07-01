CC = gcc
CC_FLAGS = -Wall -std=c99 -g

hashTable.o: hashTable.c hashTable.h
	$(CC) $(CC_FLAGS) -c hashTable.c

test: test.c hashTable.o
	$(CC) $(CC_FLAGS) -o test test.c hashTable.o



