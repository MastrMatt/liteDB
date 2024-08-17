
#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "../protocol.h"

int read_tcp_socket(int fd, char *buffer, int size);
int write_tcp_socket(int fd, char *buffer, int size);

#endif