CC = gcc
CC_FLAGS = -Wall -Werror -g
VALGRIND = valgrind
VALGRIND_FLAGS = --leak-check=full --error-exitcode=1


all: test hashTable.o  

test: test.c hashTable.o
	$(CC) $(CC_FLAGS) -o $@ $^
	($(VALGRIND) $(VALGRIND_FLAGS) ./$@ && echo "All tests passed")|| (rm hashTable.o && exit 1)

hashTable.o: hashTable.c hashTable.h
	$(CC) $(CC_FLAGS) -c $<
	





