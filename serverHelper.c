#include "server.h"

/**
 * @brief Read from a tcp socket
 *
 * Since tcp sockets can have partial reads, this function will keep reading until the buffer is full
 *
 * @param fd file descriptor of the socket
 * @param buffer buffer to read into
 * @param size size of the data to read into the buffer, in bytes
 *
 * @return int 0 on success, -1 on failure to read or unexpected EOF
 */
int read_tcp_socket(int fd, char *buffer, int size)
{

    while (size > 0)
    {
        int n = read(fd, buffer, size);

        if (n <= 0)
        {
            // error or unexpected EOF
            return -1;
        }

        size -= n;
        buffer += n;
    }

    return 0;
}


/**
 * @brief Write to a tcp socket
 *
 * Since tcp sockets can have partial writes, this function will keep writing until the buffer is empty
 *
 * @param fd file descriptor of the socket
 * @param buffer buffer to write from
 * @param size size of the data to write into the socket, in bytes
 *
 * @return int 0 on success, -1 on failure to write or unexpected EOF
 */
int write_tcp_socket(int fd, char *buffer, int size)
{

    while (size > 0)
    {
        int n = write(fd, buffer, size);

        if (n <= 0)
        {
            // error or unexpected EOF
            return -1;
        }

        size -= n;
        buffer += n;
    }

    return 0;
}

/**
 * @brief Set a file descriptor to nonblocking mode
 *
 * @param fd file descriptor to set to nonblocking mode
 */
void set_fd_nonblocking(int fd)
{
    // get the current flags for the fd from the OS
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);

    if (errno)
    {
        perror("fcntl failed");
        exit(1);
    }

    // set the nonblocking flag
    flags |= O_NONBLOCK;

    // set the new flags for the fd
    errno = 0;
    int ret = fcntl(fd, F_SETFL, flags) < 0;

    if (ret < 0)
    {
        perror("fcntl failed");
        exit(1);
    }

    return;
}
