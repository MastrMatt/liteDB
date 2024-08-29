CC = gcc
CC_FLAGS = -Wall -Werror -g
VALGRIND = valgrind
VALGRIND_FLAGS = --leak-check=full --error-exitcode=1


all: AVLTree.o test

test: AVLTree.o test.c
	$(CC) $(CC_FLAGS) -o $@ $^
	($(VALGRIND) $(VALGRIND_FLAGS) ./$@ && echo "All tests passed")|| (rm AVLTree.o && exit 1)

AVLTree.o: AVLTree.c AVLTree.h
	$(CC) $(CC_FLAGS) -c AVLTree.c
	



