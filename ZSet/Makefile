CC = gcc
CC_FLAGS = -Wall -Werror -g


HASH_TABLE_LIB = ../hashTable/hashTable.o
AVL_TREE_LIB = ../AVLTree/AVLTree.o


all: ZSet.o test

ZSet.o: ZSet.c ZSet.h
	$(CC) $(CC_FLAGS) -c ZSet.c

test: test.c ZSet.o $(HASH_TABLE_LIB) $(AVL_TREE_LIB)
	$(CC) $(CC_FLAGS) -o test test.c ZSet.o $(HASH_TABLE_LIB) $(AVL_TREE_LIB)
	./test


