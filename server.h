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
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>


#define SERVERPORT 9003
#define MAX_MESSAGE_SIZE 4096
#define MAX_CLIENTS 2047

void check_error(int value);
int read_tcp_socket(int fd, char * buffer, int size);
int write_tcp_socket(int fd, char * buffer, int size);
void set_nonblocking(int fd);


// variables/structs for the event loop
enum Conn_State{
    STATE_REQ,
    STATE_RESP,
    STATE_DONE
};

typedef struct {
    int fd;
    enum Conn_State state;

    // read buffer
    char read_buffer[4 + MAX_MESSAGE_SIZE + 1];
    int current_read_size;

    // write buffer
    char write_buffer[4 + MAX_MESSAGE_SIZE + 1];
    int need_write_size;
    int current_write_size;
} Conn ;

#endif