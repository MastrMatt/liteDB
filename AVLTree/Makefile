CC = gcc
CC_FLAGS = -Wall -Werror -g


all: test AVLTree.o

test: AVLTree.o test.c
	$(CC) $(CC_FLAGS) -o test test.c AVLTree.o

AVLTree.o: AVLTree.c AVLTree.h
	$(CC) $(CC_FLAGS) -c AVLTree.c
	



