CC = gcc
CC_FLAGS = -Wall -Werror -g


all: AVLTree.o test

test: AVLTree.o test.c
	$(CC) $(CC_FLAGS) -o test test.c AVLTree.o
	./test || rm AVLTree.o

AVLTree.o: AVLTree.c AVLTree.h
	$(CC) $(CC_FLAGS) -c AVLTree.c
	



