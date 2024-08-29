CC = gcc
CC_FLAGS = -Wall -Werror -g
VALGRIND = valgrind
VALGRIND_FLAGS = --leak-check=full --error-exitcode=1



all: test list.o

test: test.c list.o
	$(CC) $(CC_FLAGS) -o $@ $^
	($(VALGRIND) $(VALGRIND_FLAGS) ./$@ && echo "All tests passed")|| (rm list.o && exit 1)

list.o: list.c list.h	
	$(CC) $(CC_FLAGS) -c $<
