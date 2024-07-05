#include "server.h"

// Have the in-memory global hash table to store the key-value pairs
HashTable * global_table;

// accepts new connection and adds it to the fd2conn array
int accept_new_connection(Conn * fd2conn[], int server_socket) {
    // accept the new connection
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    int confd = accept(server_socket, (struct sockaddr *) &client_address, &client_address_len);
    if (confd < 0) {
        perror("accept failed");
        return -1;
    }

    // set the new connection to non-blocking
    set_fd_nonblocking(confd);

    // find an empty slot in the fd2conn array
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!fd2conn[i]) {
            break;
        }
    }

    if (i == MAX_CLIENTS) {
        fprintf(stderr, "Too many clients\n");
        close(confd);
        return -1;
    }

    // create a new connection object
    Conn * conn = (Conn *) calloc(1, sizeof(Conn));
    conn->fd = confd;
    conn->state = STATE_REQ;

    // add the connection to the fd2conn array
    fd2conn[i] = conn;

    return 0;
}


void connection_io(Conn * conn) {
    if (conn -> state == STATE_REQ) {
        state_req(conn);
    } else if (conn -> state == STATE_RESP) {
        state_resp(conn);
    } else {
        // Should not be in the done state here
        fprintf(stderr, "Invalid state\n");
        exit(1);
    }
}


char * get_response( ValueType type, char * value) {
    SerialType ser_type = SER_NIL;
    int value_len = 0;

    switch (type) {
        case STRING:
            ser_type = SER_STR;
            value_len = strlen(value);
            break;
        case INTEGER:
            ser_type = SER_INT;
            value_len = sizeof(int);
            break;
    }

    // null terminate the response
    char * response = calloc(1 + 4 + value_len + 1, sizeof(char));

    if (!response) {
        fprintf(stderr, "Failed to allocate memory for get response\n");
        exit(EXIT_FAILURE);
    }

    // write the type of the response, 1 byte
    memcpy(response, &ser_type, 1);

    // write the length of the value, 4 bytes
    memcpy(response + 1, &value_len, 4);

    // write the value
    memcpy(response + 1 + 4, value, value_len);

    return response;
}


char * null_response() {
    SerialType type = SER_NIL;
    int value_len = 0;

    // null terminate the response
    char * response = calloc(1 + 4 + 1, sizeof(char));

    if (!response) {
        fprintf(stderr, "Failed to allocate memory for set response\n");
        exit(EXIT_FAILURE);
    }

    // write the type of the response, 1 byte
    memcpy(response, &type, 1);

    // write the length of the value, 4 bytes
    memcpy(response + 1, &value_len, 4);

    return response;
}
    

char * error_response(char * err_msg) {
    SerialType type = SER_ERR;
    int err_msg_len = strlen(err_msg);

    // null terminate the response
    char * response = calloc(1 + 4 + err_msg_len + 1, sizeof(char));

    if (!response) {
        fprintf(stderr, "Failed to allocate memory for error response\n");
        exit(EXIT_FAILURE);
    }

    // write the type of the response, 1 byte 
    memcpy(response, &type, 1);

    // write the length of the error message, 4 bytes
    memcpy(response + 1, &err_msg_len, 4);

    // write the error message
    memcpy(response + 1 + 4, err_msg, err_msg_len);

    return response;
}



// Parse a request from the client(not null terminated) , need to free the returned command and it's args
Command * parse_cmd_string(char *cmd_string, int size) {
    Command *cmd = calloc(1, sizeof(Command));
    if (!cmd) {
        fprintf(stderr, "Failed to allocate memory for Command\n");
        exit(EXIT_FAILURE);
    }

    // Create a new null-terminated string to hold the command string
    char *n_cmd_string = calloc(size + 1, sizeof(char));
    if (!n_cmd_string) {
        fprintf(stderr, "Failed to allocate memory for n_cmd_string\n");
        exit(EXIT_FAILURE);
    }
    strncpy(n_cmd_string, cmd_string, size);

    // Tokenize the command string by splitting at spaces
    char *token = strtok(n_cmd_string, " ");
    int args = 0;

    while (token != NULL) {
        if (args == 0) {
            cmd->name = strdup(token); // Use strdup for convenience
        } else {
            cmd->args[args - 1] = strdup(token); // Assuming args array is preallocated
        }
        token = strtok(NULL, " ");
        args++;
    }

    cmd->num_args = args - 1;

    free(n_cmd_string);

    return cmd;
}


// execute the command and return the server response string
char * execute_command(Command * cmd) {
    if (strcmp(cmd->name, "get") == 0) {

        // check if the command has the correct number of arguments
        if (cmd->num_args != 1) {
            return error_response("get command requires 1 argument");
        }

        // get the value from the hash table
        HashNode * fetched_node = hget(global_table, cmd->args[0]);
        if (!fetched_node) {
            return error_response("Key not in database");
        }

        // get the value from the hash node
        ValueType type = fetched_node->valueType;
        char * value = fetched_node->value;

        return get_response(type,value);

    } else if (strcmp(cmd->name, "set") == 0) {

        // obtain type of the value
        if (cmd->num_args != 2) {
            return error_response("set command requires 2 arguments");
        }

        // ! This is the code for ZSETS, move later
        // // use strtol and duck typing to determine the type of the value
        // char * endptr;
        // long value = strtol(cmd->args[1], &endptr, 10);

        // // check if sucessfully converted to an integer
        // if ((*endptr == '\0') && (endptr != cmd->args[1])) {
        //     type = INTEGER;
        //     // convert long to int(database only supports int)
        //     int int_value = (int) value;
        // } else {
        //     type = STRING;
        // }

        // set the value in the hash table
        HashNode * new_node = calloc(1, sizeof(HashNode));
        if (!new_node) {
            fprintf(stderr, "Failed to allocate memory for new node\n");
            exit(EXIT_FAILURE);
        }

        new_node->key = cmd->args[0];
        new_node->value = cmd->args[1];

        // * All data is stored as strings except for the ZSET values
        new_node->valueType = STRING;

        hinsert(global_table, new_node);

        return null_response();
    } else if (strcmp(cmd->name, "del") == 0) {
        if (cmd->num_args != 1) {
            return error_response("del command requires 1 argument");
        }

        // remove the value from the hash table
        HashNode * removed_node = hremove(global_table, cmd->args[0]);

        if (!removed_node) {
            return error_response("Key not in database");
        }

        return null_response();
    } else {
        return error_response("Unknown command");
    }

    // free the command
    free(cmd->name);

    // free all args
    for (int i = 0; i < cmd->num_args; i++) {
        free(cmd->args[i]);
    }
    
}


bool try_process_single_request(Conn * conn) {
    // check if the read buffer has enough data to process a request
    if (conn->current_read_size < 4) {
        // not enough data to process a request
        return false;
    }

    // copy the first 4 bytes of the read buffer, assume the message size is in little endian (this machine is little endian)
    int message_size = 0;
    memcpy(&message_size, conn->read_buffer, 4);

    if (message_size > MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Message size too large\n");
        conn->state = STATE_DONE;
        return false;
    }

    // check if the read buffer has enough data to process a request
    if (conn->current_read_size < 4 + message_size) {
        // not enough data to process a request
        return false;
    }

    // print the message, account for the fact that the message is not null terminated due to pipe-lining
    printf("Client %d says: %.*s\n", conn->fd, message_size, conn->read_buffer + 4);

    // parse the message to extract the command
    Command * cmd = parse_cmd_string(conn->read_buffer + 4, message_size);

    // execute the command, response is a null terminated byte string following the protocol
    char * response = execute_command(cmd);
    int total_size = strlen(response);

    printf("Server response: %.*s\n", total_size, response);

    if (total_size > MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Server sending a message that is too long\n");
        conn->state = STATE_DONE;
        return false;
    }

    // copy the response byte string to the write buffer
    memcpy(conn->write_buffer, &response, total_size);

    // free the response
    free(response);

    conn->need_write_size = total_size;

    // remove the request from the read buffer
    int remaining_size = conn->current_read_size - (4 + message_size);

    // using memmove instead of memcpy to handle overlapping memory regions
    if (remaining_size) {
        memmove(conn->read_buffer, conn->read_buffer + 4 + message_size, remaining_size);
    }

    conn->current_read_size = remaining_size;

    // the request has been processed, move to the response state
    conn->state = STATE_RESP;
    state_resp(conn);

    // continue the outer loop to process pipelined requests
    return (conn->state == STATE_REQ);
}


bool try_fill_read_buffer(Conn * conn) {
    // check if the read buffer overflowed
    if (conn->current_read_size > sizeof(conn->read_buffer)) {
        fprintf(stderr, "Read buffer overflow\n");
        return false;
    }

    // read from the socket
    int read_size = 0;

    // attempt to read from the socket until we have read some characters or a signal has not interrupted the read
    do {
        int max_possible_read = sizeof(conn->read_buffer) - conn->current_read_size;

        read_size = read(conn->fd, conn->read_buffer + conn->current_read_size, max_possible_read);
    } while (read_size < 0 && errno == EINTR);

    if ((read_size < 0) && (errno = EAGAIN)) {
        //  read buffer is full, wait for the next poll event
        return false;
    }

    if (read_size < 0) {
        // an error that is not EINTR or EAGAIN occured
        perror("read failed");
        return false;
    }

    if (read_size == 0) {
        // if read_size is 0 then EOF reached 

        // if the current_read_size is greater than 0, then the EOF was reached before reading the full message
        if (conn->current_read_size > 0) {
            fprintf(stderr, "EOF reached before reading full message\n");
        } else {
            // EOF reached
            printf("EOF reached\n");
        }

        conn->state = STATE_DONE;
        return false;
    }

    conn -> current_read_size += read_size;
    if (conn->current_read_size > sizeof(conn->read_buffer)) {
        fprintf(stderr, "Read buffer overflow\n");
        return false;
    }

    // the loop here is to handle that clients can send multiple requests in one go (pipe-lining)
    while (try_process_single_request(conn)) {

    };

    return (conn->state == STATE_REQ);
}

bool try_flush_write_buffer(Conn * conn) {
    
    int write_size = 0;

    // attempt to write to the socket until we have written some characters or a signal has not interrupted the write
    do {
        int max_possible_write = conn->need_write_size - conn->current_write_size;

        write_size = write(conn->fd, conn->write_buffer + conn->current_write_size, max_possible_write); 
    } while (write_size < 0 && errno == EINTR);

    if (write_size < 0) {
        if (errno == EAGAIN) {
            // write buffer is full, wait for the next poll event
            return false;
        }
        // an error that is not EINTR or EAGAIN occured, exit the connection
        perror("write failed");
        conn->state = STATE_DONE;
        return false;
    }

    conn->current_write_size += write_size;

    if (conn->current_write_size > conn->need_write_size) {
        fprintf(stderr, "Write buffer overflow\n");
        conn->state = STATE_DONE;
        return false;
    };

    if (conn->current_write_size == conn->need_write_size) {
        // the response has been fully written, move to the request state
        conn->state = STATE_REQ;
        conn->current_write_size = 0;
        conn->need_write_size = 0;

        // exit the outer write loop
        return false;
    }

    // data still in the write buffer, try to write again in the outer loop
    return true;
}

void state_req(Conn * conn) {
    while(try_fill_read_buffer(conn)) {

    };
}


void state_resp(Conn * conn) {
    while (try_flush_write_buffer(conn)) {

    };
}


// build a TCP server that listens on port 
int main (int argc, char * argv []) {

    // create the socket
    int server_socket= socket(AF_INET, SOCK_STREAM, 0);
    check_error(server_socket);
    
    // Set the socket options to allow address reuse, only for debugging
    int optval = 1;
    check_error(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)));

    // define the server address
    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = ntohs(SERVERPORT);

    // allow the server to listen to all network interfaces
    server_address.sin_addr.s_addr = ntohl(INADDR_ANY);


    // bind the socket to the specified IP and port
    check_error(bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)));

    // listen for incoming connections, allow maximum number of connections allowed by the OS
    check_error(listen(server_socket, SOMAXCONN));

    // a map of all client connections, keyed by fd(Use array to avoid hash table for simplicity)
    Conn * fd2conn[MAX_CLIENTS] = {0};

    // set the server socket to non-blocking
    set_fd_nonblocking(server_socket);

    // set up the poll arguments
    struct pollfd poll_args[MAX_CLIENTS + 1];
    memset(poll_args, 0, sizeof(poll_args));

    printf("Server listening on port %d\n", SERVERPORT);
    
    // the event loop, note: there is only on server socket responsible for interating with other client fd's
    while (1) {
        // prepare the arguments of the poll(), the first argument is the server socket, the events specifies that we are interested in reading from the server socket, 
        poll_args[0].fd = server_socket;
        poll_args[0].events = POLLIN;
        poll_args[0].revents = 0;

        // handle the client connections
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!fd2conn[i]) {
                continue;
            }

            // set the poll arguments for the client fd
            poll_args[i+1].fd = fd2conn[i]->fd;

            // set the events to read from the client fd
            poll_args[i+1].events = fd2conn[i]->state == STATE_REQ ? POLLIN : POLLOUT;

            poll_args[i+1].revents = 0;

            // make sure thhe OS listens to errors on the client fd
            poll_args[i].events |= POLLERR;
        }

        // call poll() to wait for events
        int ret = poll(poll_args, MAX_CLIENTS + 1, 1000);

        if (ret < 0) {
            perror("poll failed");
            exit(1);
        }

        // process ready client connections
        for (int i = 1; i < MAX_CLIENTS + 1; i++) {
            if (poll_args[i].revents == 0) {
                // no events on this fd, not interested
                continue;
            }

            Conn * conn = fd2conn[i-1];
            
            connection_io(conn);


            if (conn->state == STATE_DONE) {
                // close the connection
                close(conn->fd);
                free(conn);
                fd2conn[i-1] = 0;
                memset(&poll_args[i], 0, sizeof(poll_args[i]));
            }
        }

        // try to accept new connections if server socket is ready
        if (poll_args[0].revents) {
           accept_new_connection(fd2conn, server_socket);
        }

    }

    return 0;

}


