CC = gcc
CC_FLAGS = -Wall -std=c99


all: server client

server: server.c serverHelper.o
	$(CC) $(CC_FLAGS) -o server server.c serverHelper.o

client: client.c serverHelper.o
	$(CC) $(CC_FLAGS) -o client client.c serverHelper.o

serverHelper.o: serverHelper.c server.h
	$(CC) $(CC_FLAGS) -c serverHelper.c