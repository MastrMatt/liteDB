CC = gcc
CC_FLAGS = -Wall -g

all: test AVLTree.o

test: test.c
	$(CC) $(CC_FLAGS) -o test test.c

AVLTree.o: AVLTree.c AVLTree.h
	$(CC) $(CC_FLAGS) -c AVLTree.c
	



