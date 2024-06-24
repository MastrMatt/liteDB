#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// build a TCP server that listens on port 

int main (int argc, char * argv []) {

    // create the socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed"); 
        exit(1);
    }

    // define the server address
    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);

    // allow the server to listen to all network interfaces
    server_address.sin_addr.s_addr = INADDR_ANY;


    // bind the socket to the specified IP and port
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("bind failed");
        exit(1);
    }

    // listen for incoming connections, allow maximum number of connections allowed by the OS
    if (listen(server_socket, SOMAXCONN) < 0) {
        perror("listen failed");
        exit(1);
    }

    // accept the incoming connection
    int client_socket = accept(server_socket, NULL, NULL);
    

    // process client request
    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);

        char buffer[1024];

        int confd = accept(server_socket, (struct sockaddr *) &client_address, &client_address_len);

        if (confd < 0) {
            perror("accept failed");
            exit(1);
        }

        process(confd);

        close(confd);
    }

}

void process(int confd) {
    char buffer[1024];
    int n = read(confd, buffer, sizeof(buffer));
    if (n < 0) {
        perror("read failed");
        exit(1);
    }

    printf("Client says: %s\n", buffer);

    // plus one to account for the null terminator
    n = write(confd, "Hello from server", 18);
    if (n < 0) {
        perror("write failed");
        exit(1);
    }
}