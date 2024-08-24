CC = gcc
CC_FLAGS = -Wall -Werror -g
HASH_TABLE_LIB = ../hashTable/hashTable.o
AVL_TREE_LIB = ../AVLTree/AVLTree.o
ZSet_LIB = ../ZSet/ZSet.o
list_LIB = ../list/list.o
aof_LIB = ../aof/aof.o
PROTOCOL_HEADER = ../protocol.h


all: runserver testserver test 

test:
	./testserver

runserver: runserver.c server.o $(ZSet_LIB) $(HASH_TABLE_LIB) $(AVL_TREE_LIB)  $(list_LIB) $(aof_LIB)
	$(CC) $(CC_FLAGS) -o runserver runserver.c server.o $(ZSet_LIB) $(HASH_TABLE_LIB) $(AVL_TREE_LIB)  $(list_LIB) $(aof_LIB) -lpthread 

server.o: server.c server.h $(PROTOCOL_HEADER)
	$(CC) $(CC_FLAGS) -c server.c

testserver: testserver.c server.o $(ZSet_LIB) $(HASH_TABLE_LIB) $(AVL_TREE_LIB)  $(list_LIB) $(aof_LIB)
	$(CC) $(CC_FLAGS) -o testserver testserver.c server.o $(ZSet_LIB) $(HASH_TABLE_LIB) $(AVL_TREE_LIB)  $(list_LIB) $(aof_LIB) -lpthread


