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

// execute the command and return the response
char * execute_command(Command * cmd) {
    if (strcmp(cmd->name, "get") == 0) {
        // get the value from the hash table
        HashNode * fetched_node = hget(global_table, cmd->args[0]);

        if (fetched_node) {
            return fetched_node->value;
        } else {
            return "ERR Key not found";
        }
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

    // parse the message to extract the command
    Command * cmd = parse_cmd_string(conn->read_buffer + 4, message_size);

    // execute the command
    char * response = execute_command(conn);


    // print the message, account for the fact that the message is not null terminated due to pipe-lining
    printf("Client %d says: %.*s\n", conn->fd, message_size, conn->read_buffer + 4);

    // Echo the message back to the client with some prefix from the server
    char * prefix = "Server echoes: ";
    int prefix_size = strlen(prefix);
    int total_size = prefix_size + message_size;

    if (total_size > MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Server prefixed message too large\n");
        conn->state = STATE_DONE;
        return false;
    }

    // write the size of full message
    memcpy(conn->write_buffer, &total_size, 4);

    // write the message to the buffer
    memcpy(conn->write_buffer + 4, prefix, prefix_size);
    memcpy(conn->write_buffer + 4 + prefix_size, conn->read_buffer + 4, message_size);

    conn->need_write_size = 4 + total_size;

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


