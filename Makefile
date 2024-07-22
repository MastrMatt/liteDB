CC = gcc
CC_FLAGS = -Wall -g
HASH_TABLE_LIB = hashTable/hashTable.o
AVL_TREE_LIB = AVLTree/AVLTree.o
ZSet_LIB = ZSet/ZSet.o


all: server client

server: server.c serverHelper.o $(ZSet_LIB) $(HASH_TABLE_LIB) $(AVL_TREE_LIB) 
	$(CC) $(CC_FLAGS) -o server server.c serverHelper.o $(ZSet_LIB) $(HASH_TABLE_LIB) $(AVL_TREE_LIB) 

client: client.c serverHelper.o  
	$(CC) $(CC_FLAGS) -o client client.c serverHelper.o

serverHelper.o: serverHelper.c server.h
	$(CC) $(CC_FLAGS) -c serverHelper.c 
