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
    Conn * fd2conn[MAX_FDS] = {0};

    // set the server socket to non-blocking
    set_fd_nonblocking(server_socket);

    // set up the poll arguments
    struct pollfd poll_args[MAX_FDS];
    
    // the event loop, note: there is only on server socket responsible for interating with other client fd's
    while (1) {
        // prepare the arguments of the poll(), the first argument is the server socket, the events specifies that we are interested in reading from the server socket, 
        poll_args[0].fd = server_socket;
        poll_args[0].events = POLLIN;
        poll_args[0].revents = 0;

        // handle the client connections
        for (int i = 0; i < MAX_FDS; i++) {
            if (!fd2conn[i]) {
                continue;
            }

            int c = i + 1;
            if (c >= MAX_FDS) {
                return;
            }

            // set the poll arguments for the client fd
            poll_args[i].fd = fd2conn[i]->fd;

            // set the events to read from the client fd
            poll_args[i].events = fd2conn[i]->state == STATE_REQ ? POLLIN : POLLOUT;

            // make sure thhe OS listens to errors on the client fd
            poll_args[i].events |= POLLERR;
        }









        printf("Waiting for clients to connect\n");

        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);

        int confd = accept(server_socket, (struct sockaddr *) &client_address, &client_address_len);
        check_error(confd);

        while (1) {
            int err = process_single_request(confd);
            if (err <  0) {
                break;
            }
        }

        close(confd);
    }

    close(server_socket);
}

