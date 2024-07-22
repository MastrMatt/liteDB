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

// Zset includes AVLTree and HashTable header
#include "ZSet/ZSet.h"

#define SERVERPORT 9000
#define MAX_MESSAGE_SIZE 4096
#define MAX_CLIENTS 2047
#define INIT_TABLE_SIZE 1024

// Protocol information
#define MAX_ARGS 10

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


typedef struct {
    char * name;
    char * args[MAX_ARGS];
    int num_args;

} Command;

typedef enum {
    SER_NIL,
    SER_ERR,
    SER_STR,
    SER_INT,
    SER_FLOAT,
    SER_ARR,

} SerialType;

// helper functions
void check_error(int value);
int read_tcp_socket(int fd, char * buffer, int size);
int write_tcp_socket(int fd, char * buffer, int size);
void set_fd_nonblocking(int fd);

// server functions
int accept_new_connection(Conn * fd2conn[], int server_socket);
void connection_io(Conn * conn);
bool try_process_single_request(Conn * conn);
bool try_fill_read_buffer(Conn * conn);
bool try_flush_write_buffer(Conn * conn);
void state_req(Conn * conn);
void state_resp(Conn * conn);


#endif