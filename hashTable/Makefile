CC = gcc
CC_FLAGS = -Wall -Werror -g


all: hashTable.o test 

hashTable.o: hashTable.c hashTable.h
	$(CC) $(CC_FLAGS) -c hashTable.c
	
test: test.c hashTable.o
	$(CC) $(CC_FLAGS) -o test test.c hashTable.o
	./test







