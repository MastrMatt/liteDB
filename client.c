#include "server.h"


int process_server_response(int confd) {
    char buffer[5 + MAX_MESSAGE_SIZE];

    int err = 0;
    int type = 0;
    int message_size = 0;

    // read the type of the response message
    err = read_tcp_socket(confd, buffer, 1);
    if (err) {
        if (err < 0) {
            perror("read failed");
        } else {
            fprintf(stderr, "EOF reached before reading full message size\n");
        }

        return err;
    }

    // read the message size
    err = read_tcp_socket(confd, buffer + 1, 4);

    if (err) {
        if (err < 0) {
            perror("read failed");
        } else {
            fprintf(stderr, "EOF reached before reading full message size\n");
        }
        return err;
    }

    memcpy(&message_size, buffer + 1, 4);

    if(message_size > MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Message too long\n");
        return -1;
    }

    type = 0;
    memcpy(&type, buffer, 1);

    message_size = 0;
    memcpy(&message_size, buffer + 1, 4);

    switch (type) {
        case SER_NIL:
            printf("(nil)\n");
            return 0;
            break;
        case SER_ERR:
            // read the response message
            err = read_tcp_socket(confd, buffer + 5, message_size);
            if (err) {
                if (err < 0) {
                    perror("read failed");
                } else {
                    fprintf(stderr, "EOF reached before reading full message size\n");
                }
                return err;
            }

            printf("(err) %.*s\n", message_size, buffer + 5);
            return 0;
            break;
        case SER_STR:
            // read the response message
            err = read_tcp_socket(confd, buffer + 5, message_size);
            if (err) {
                if (err < 0) {
                    perror("read failed");
                } else {
                    fprintf(stderr, "EOF reached before reading full message size\n");
                }
                return err;
            }
        
            printf("(str) %.*s\n", message_size, buffer + 5);
            return 0;
            break;
        case SER_INT:
            // read the response message
            err = read_tcp_socket(confd, buffer + 5, message_size);
            if (err) {
                if (err < 0) {
                    perror("read failed");
                } else {
                    fprintf(stderr, "EOF reached before reading full message size\n");
                }
                return err;
            }

            int value = *(int *) (buffer + 5);
            printf("(int) %d\n", value);
            return 0;
            break;
        case SER_FLOAT:
            // read the response message
            err = read_tcp_socket(confd, buffer + 5, message_size);
            if (err) {
                if (err < 0) {
                    perror("read failed");
                } else {
                    fprintf(stderr, "EOF reached before reading full message size\n");
                }
                return err;
            }

            float fvalue = *(float *) (buffer + 5);
            printf("(float) %f\n", fvalue);
            return 0;
            break;

        case SER_ARR:
            printf("(arr) len = %d\n", message_size);

            // message size is length of the array
            for (int i = 0; i < message_size; i++) {
                err = process_server_response(confd);
                if (err) {
                    return err;
                }
            }

            printf("(arr) end\n");
            return 0;
            break;

        default:
            fprintf(stderr, "Unknown response type\n");
            return -1;
    }
}


int handle_request(int confd, char * message) {
    char buffer[4 + MAX_MESSAGE_SIZE + 1];

    // write the message size
    int message_size = strlen(message);
    if (message_size > MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Message too long\n");
        return -1;
    }

    memcpy(buffer, &message_size, 4);

    // write the message to the buffer
    memcpy(buffer + 4, message, message_size);

    // send the message
    int err = write_tcp_socket(confd, buffer, 4 + message_size);
    if (err) {
        if (err < 0) {
            perror("write failed");
        } else {
            fprintf(stderr, "EOF reached before writing full message size\n");
        }
        return err;
    }

    err = process_server_response(confd);
    if (err) {
        fprintf(stderr, "Error processing server response\n");
        return err;
    }

    return 0;

}

int main() {
    int client_socket;
    struct sockaddr_in server_address;

    // create the client socket fd
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // define the server address information
    server_address.sin_family = AF_INET;
    server_address.sin_port = ntohs(SERVERPORT);

    // connect to the server
    server_address.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

    printf("Connecting to server\n");

    check_error(connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address)));

    while (1) {
        char message[200];
        printf("Enter a cmd: ");
        fgets(message, 200, stdin);

        // remove the newline character
        message[strlen(message) - 1] = '\0';
    
        int err = handle_request(client_socket, message);

        if (err < 0) {
            break;
        }
    }

    close(client_socket);
    return 0;
}
