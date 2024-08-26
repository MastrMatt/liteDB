CC = gcc
CC_FLAGS = -Wall -Werror -g


all: test hashTable.o  

hashTable.o: hashTable.c hashTable.h
	$(CC) $(CC_FLAGS) -c hashTable.c
	
test: test.c hashTable.o
	$(CC) $(CC_FLAGS) -o test test.c hashTable.o
	./test || rm hashTable.o






