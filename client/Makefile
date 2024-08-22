
CC = gcc
CC_FLAGS = -Wall -Werror -g
PROTOCOL_HEADER = ../protocol.h

all: runclient

runclient: client.o
	$(CC) $(CC_FLAGS) -o runclient client.o

client.o: client.c client.h $(PROTOCOL_HEADER)
	$(CC) $(CC_FLAGS) -c client.c