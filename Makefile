CC = gcc
CC_FLAGS = -Wall -std=c99 -g
HASH_TABLE_LIB = hashTable/hashTable.o


all: server client

server: server.c serverHelper.o $(HASH_TABLE_LIB) 
	$(CC) $(CC_FLAGS) -o server server.c serverHelper.o $(HASH_TABLE_LIB)

client: client.c serverHelper.o 
	$(CC) $(CC_FLAGS) -o client client.c serverHelper.o

serverHelper.o: serverHelper.c server.h
	$(CC) $(CC_FLAGS) -c serverHelper.c 
