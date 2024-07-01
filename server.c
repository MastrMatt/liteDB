#include "server.h"


int process_single_request(int confd) {
    // the buffer size has 4 bytes for the message size, and 1 byte for the null terminator
    char buffer[4 + MAX_MESSAGE_SIZE + 1];
    
    // read the 4 bytes for the message size
    errno = 0;
    int err = read_tcp_socket(confd, buffer, 4);

    if (err) {
        if (err < 0) {
            perror("read failed");
        } else {
            fprintf(stderr, "EOF reached before reading full message size\n");
        }
        return err;
    }

    // copy the first 4 bytes of buffer, assume the message size is in little endian (this machine is little endian)
    int message_size = 0;
    memcpy(&message_size, buffer, 4);

    if (message_size > MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Message size too large\n");
        return -1;
    }

    // read the message
    err = read_tcp_socket(confd, buffer+4, message_size);
    if (err) {
        if (err < 0) {
            perror("read failed");
        } else {
            fprintf(stderr, "EOF reached before reading full message size\n");
        }
        return err;
    }

    printf("Message size: %d\n", message_size);
    printf("Buffer: %s", buffer);

    // add the null terminator
    buffer[4 + message_size] = '\0';

    // print the message
    printf("Client says: %s\n", buffer+4);


    char * response = "Hi server ;), from server";
    char response_buffer[4 + MAX_MESSAGE_SIZE + 1];

    int ret_message_size = strlen(response);

    // write the message size
    memcpy(response_buffer, &ret_message_size, 4);

    // write the message to the buffer
    memcpy(response_buffer + 4, response, ret_message_size);

    // send the response
    err = write_tcp_socket(confd, response_buffer, 4 + ret_message_size);
    if (err) {
        if (err < 0) {
            perror("write failed");
        } else {
            fprintf(stderr, "EOF reached before writing full message size\n");
        }
        return err;
    }

    return 0;
}

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
        // error
        perror("read failed");
        return false;
    }

    if (read_size == 0) {
        if (conn->current_read_size > 0) {
            fprintf(stderr, "EOF reached before reading full message\n");
        } else {
            // EOF reached
            pritnf("EOF reached\n");
        }

        conn->state = STATE_DONE;
        return false;
    }

    conn -> current_read_size += read_size;
    if (conn->current_read_size > sizeof(conn->read_buffer)) {
        fprintf(stderr, "Read buffer overflow\n");
        return false;
    }

    // the loop here is to handle that clients can send multiple requests in one go
    while (try_process_single_request(conn)) {

    };

    return (conn->state == STATE_REQ);
}



void state_req(Conn * conn) {
    while(try_fill_read_buffer(conn)) {

    };
}



void connection_io(Conn * conn) {
    if (conn -> state == STATE_REQ) {
        state_req(conn);
    } else if (conn -> state == STATE_RESP) {
        state_resp(conn);
    } else {
        // Should not be in the done state here
        fpritnf(stderr, "Invalid state\n");
        exit(1);
    }
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
                fd2conn[i-1] = NULL;
            }
        }

        // try to accept new connections if server socket is ready
        if (poll_args[0].revents) {
           accept_new_connection(fd2conn, server_socket);
        }

    }

    return 0;

}


