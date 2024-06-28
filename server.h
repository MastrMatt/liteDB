#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>


#define SERVERPORT 9003
#define MAX_MESSAGE_SIZE 4096

void check_error(int value);
int read_tcp_socket(int fd, char * buffer, int size);
int write_tcp_socket(int fd, char * buffer, int size);

#endif