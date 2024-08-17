
CC = gcc
CC_FLAGS = -Wall -Werror -g

all: client

client: client.o
	$(CC) $(CC_FLAGS) -o client client.o

client.o: client.c client.h
	$(CC) $(CC_FLAGS) -c client.c