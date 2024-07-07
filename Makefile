CC = gcc
CC_FLAGS = -Wall -g
HASH_TABLE_LIB = hashTable/hashTable.o
AVLTree_LIB = AVLTree/AVLTree.o


all: server client

server: server.c serverHelper.o $(HASH_TABLE_LIB) $(AVLTree_LIB)
	$(CC) $(CC_FLAGS) -o server server.c serverHelper.o $(HASH_TABLE_LIB) $(AVLTree_LIB)

client: client.c serverHelper.o 
	$(CC) $(CC_FLAGS) -o client client.c serverHelper.o

serverHelper.o: serverHelper.c server.h
	$(CC) $(CC_FLAGS) -c serverHelper.c 
