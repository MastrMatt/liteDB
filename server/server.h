#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>

// Zset includes AVLTree and HashTable header
#include "../ZSet/ZSet.h"
#include "../list/list.h"
#include "../aof/aof.h"

// protcol header
#include "../protocol.h"

// persistent storage
#define AOF_FILE "AOF.aof"
#define FLUSH_INTERVAL_SEC 5

// should be multiple of two
#define INIT_TABLE_SIZE 1024

// variables/structs for the event loop
enum Conn_State
{
    STATE_REQ,
    STATE_RESP,
    STATE_DONE
};

typedef struct
{
    int fd;
    enum Conn_State state;

    // read buffer
    char read_buffer[4 + MAX_MESSAGE_SIZE + 1];
    int current_read_size;

    // write buffer
    char write_buffer[4 + MAX_MESSAGE_SIZE + 1];
    int need_write_size;
    int current_write_size;
} Conn;

typedef struct
{
    char *name;
    char *args[MAX_ARGS];
    int num_args;

} Command;

// server functions
void set_fd_nonblocking(int fd);
int accept_new_connection(Conn *fd2conn[], int server_socket);
void connection_io(Conn *conn);
bool try_process_single_request(Conn *conn);
bool try_fill_read_buffer(Conn *conn);
bool try_flush_write_buffer(Conn *conn);
void state_req(Conn *conn);
void state_resp(Conn *conn);

char *get_response(ValueType type, void *value);
char *null_response();
char *error_response(char *err_msg);
char *avl_iterate_response(AVLNode *tree, AVLNode *start, long limit);

Command *parse_cmd_string(char *cmd_string, int size);
char *execute_command(Command *cmd, bool aof_restore);

void global_table_del(char *key, char *value, ValueType type);
char *exists_command(Command *cmd);
char *del_command(Command *cmd, bool aof_restore);
char *keys_command();
char *flushall_cmd(Command *cmd, bool aof_restore);

char *get_command(Command *cmd);
char *set_command(Command *cmd, bool aof_restore);

char *hexists_command(Command *cmd);
char *hset_command(Command *cmd, bool aof_restore);
char *hget_command(Command *cmd);
char *hdel_command(Command *cmd, bool aof_restore);
char *hgetall_command(Command *cmd);

char *lexists_command(Command *cmd);
char *lpush_command(Command *cmd, bool aof_restore);
char *rpush_command(Command *cmd, bool aof_restore);
char *lpop_command(Command *cmd, bool aof_restore);
char *rpop_command(Command *cmd, bool aof_restore);
char *lrem_command(Command *cmd, bool aof_restore);
char *llen_cmd(Command *cmd);
char *lrange_cmd(Command *cmd);
char *ltrim_cmd(Command *cmd, bool aof_restore);
char *lset_cmd(Command *cmd, bool aof_restore);

char *zadd_command(Command *cmd, bool aof_restore);
char *zrem_command(Command *cmd, bool aof_restore);
char *zscore_cmd(Command *cmd);
char *zquery_cmd(Command *cmd);

void aof_restore_db();
void handle_aof_write(Command *cmd);

// Global variables (usually avoid, but okay here since no function depends on a specific state of the global table or aof, behaves)
extern HashTable *global_table;
extern AOF *global_aof;
extern pthread_t aof_thread;
extern int server_socket;
extern Conn *fd2conn[MAX_CLIENTS];

#endif