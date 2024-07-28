#include "server.h"

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
