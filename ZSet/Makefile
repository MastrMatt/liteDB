CC = gcc
CC_FLAGS = -Wall -Werror -g
VALGRIND = valgrind
VALGRIND_FLAGS = --leak-check=full --error-exitcode=1


HASH_TABLE_LIB = ../hashTable/hashTable.o
AVL_TREE_LIB = ../AVLTree/AVLTree.o


all: ZSet.o test

ZSet.o: ZSet.c ZSet.h
	$(CC) $(CC_FLAGS) -c $<

test: test.c ZSet.o $(HASH_TABLE_LIB) $(AVL_TREE_LIB)
	$(CC) $(CC_FLAGS) -o $@ $^
	($(VALGRIND) $(VALGRIND_FLAGS) ./$@ && echo "All tests passed")|| (rm ZSet.o && exit 1)


