#include "server.h"


int send_request(int confd, char * message) {
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

    // read the response message size
    err = read_tcp_socket(confd, buffer, 4);
    if (err) {
        if (err < 0) {
            perror("read failed");
        } else {
            fprintf(stderr, "EOF reached before reading full message size\n");
        }
        return err;
    }

    memcpy(&message_size, buffer, 4);

    if(message_size > MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Message too long\n");
        return -1;
    }

    // read the response message
    err = read_tcp_socket(confd, buffer + 4, message_size);
    if (err) {
        if (err < 0) {
            perror("read failed");
        } else {
            fprintf(stderr, "EOF reached before reading full message size\n");
        }
        return err;
    }

    printf("Message size: %d\n", message_size);

    // add the null terminator
    buffer[4 + message_size] = '\0';

    // print the message
    printf("Server says: %s\n", buffer + 4);

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
        char message[100];
        printf("Enter a short message: ");
        fgets(message, 100, stdin);
    
        int err = send_request(client_socket, message);
        if (err < 0) {
            break;
        }
    }

    close(client_socket);
    return 0;
}
