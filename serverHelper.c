#include "server.h"

void check_error(int value) {
    if (value < 0) {
        perror("error");
        exit(1);
    }
}

/*
helper function to handle reading from a tcp socket, since tcp sockets can have partial reads
*/

int read_tcp_socket(int fd, char * buffer, int size) {
    
    while (size  > 0) {
        int n = read(fd, buffer, size);

        if (n <= 0) {
            // error or unexpected EOF
            return -1;
        }

        size -= n;
        buffer += n;
    }

    return 0;   
}


/*
helper function to handle writing to a tcp socket, since tcp sockets can have partial writes
*/
int write_tcp_socket(int fd, char * buffer, int size) {
    
    while (size  > 0) {
        int n = write(fd, buffer, size);

        if (n <= 0) {
            // error or unexpected EOF
            return -1;
        }

        size -= n;
        buffer += n;
    }

    return 0;   
}


void set_fd_nonblocking(int fd) {
    // get the current flags for the fd from the OS
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);

    if (errno) {
        perror("fcntl failed");
        exit(1);
    }

    // set the nonblocking flag
    flags |= O_NONBLOCK;

    // set the new flags for the fd
    errno = 0;
    int ret = fcntl(fd, F_SETFL, flags) < 0;

    if (ret < 0) {
        perror("fcntl failed");
        exit(1);
    }

    return;
}

// Parse a request from the client(not null terminated) , need to free the returned command and it's args
Command *parse_cmd_string(char *cmd_string, int size) {
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