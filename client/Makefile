
CC = gcc
CC_FLAGS = -Wall -Werror -g

all: runclient

runclient: client.o
	$(CC) $(CC_FLAGS) -o runclient client.o

client.o: client.c client.h
	$(CC) $(CC_FLAGS) -c client.c