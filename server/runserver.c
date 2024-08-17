#include "server.h"

/**
 * @ Handles the clean up of the server when a SIGINT signal is received.
 *
 * The function handles the clean up of the server when a SIGINT signal is received. The function closes the server socket, all client connections, frees the global table, closes the AOF file, and exits the program.
 *
 * @param signum Signal number
 */

void handle_sigint()
{
    // close the server socket
    close(server_socket);

    // close all client connections
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (fd2conn[i])
        {
            close(fd2conn[i]->fd);
            free(fd2conn[i]);
        }
    }

    // free the global table
    hfree_table(global_table);

    // close the aof file
    aof_close(global_aof);

    // don't need to handle the aof thread, since about to shutdown main process anyway

    // exit the program
    exit(EXIT_SUCCESS);
}

// Event loop for the server
int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);
    int debugMode = 0;

    // Parse command line arguments for debug mode
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug"))
        {
            debugMode = 1;
            break;
        }
    }

    // create aof file if it does not exist
    FILE *file = fopen(AOF_FILE, "a");
    fclose(file);

    // Initialize global structures
    global_table = hcreate(INIT_TABLE_SIZE);
    global_aof = aof_init(AOF_FILE, FLUSH_INTERVAL_SEC, "r");

    // restore state of database from AOF file
    aof_restore_db();

    // change the aof back to append mode, so that new commands are appended to the file
    aof_change_mode(global_aof, AOF_FILE, "a");

    // set up for starting the aof thread in a detached state (so that it cleans up after itself without needing to be joined) since it is in an infinite loop
    pthread_attr_t aof_thread_attr;
    pthread_attr_init(&aof_thread_attr);
    pthread_attr_setdetachstate(&aof_thread_attr, PTHREAD_CREATE_DETACHED);

    // start the aof flushing thread in a detached state
    int ret = pthread_create(&aof_thread, &aof_thread_attr, aof_flush, (void *)global_aof);
    if (ret)
    {
        fprintf(stderr, "Failed to create AOF flush thread\n");
        exit(EXIT_FAILURE);
    }

    // initialize the server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("server socket creation failed");
        exit(EXIT_FAILURE);
    }

    // set the socket options to allow address reuse, only for debugging
    if (debugMode)
    {
        int optval = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        {
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }
    }

    // define the server address
    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVERPORT);

    // allow the server to listen to all network interfaces
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the socket to the specified IP and port
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    };

    // listen for incoming connections, allow maximum number of connections allowed by the OS
    if (listen(server_socket, SOMAXCONN) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // set the server socket to non-blocking
    set_fd_nonblocking(server_socket);

    // set up the poll arguments
    struct pollfd poll_args[MAX_CLIENTS + 1];
    memset(poll_args, 0, sizeof(poll_args));

    printf("Server running in debug mode? : %s\n", debugMode ? "true" : "false");
    printf("Server listening on port %d\n", SERVERPORT);

    // the event loop, note: there is only on server socket responsible for interating with other client fd's
    while (1)
    {
        // prepare the arguments of the poll(), the first argument is the server socket, the events specifies that we are interested in reading from the server socket,
        poll_args[0].fd = server_socket;
        poll_args[0].events = POLLIN;
        poll_args[0].revents = 0;

        // handle the client connections
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (!fd2conn[i])
            {
                continue;
            }

            // set the poll arguments for the client fd
            poll_args[i + 1].fd = fd2conn[i]->fd;

            // set the events to read from the client fd
            poll_args[i + 1].events = fd2conn[i]->state == STATE_REQ ? POLLIN : POLLOUT;

            poll_args[i + 1].revents = 0;

            // make sure thhe OS listens to errors on the client fd
            poll_args[i].events |= POLLERR;
        }

        // call poll() to wait for events
        int ret = poll(poll_args, MAX_CLIENTS + 1, 1000);

        if (ret < 0)
        {
            perror("poll failed");
            exit(1);
        }

        // process ready client connections
        for (int i = 1; i < MAX_CLIENTS + 1; i++)
        {
            if (poll_args[i].revents == 0)
            {
                // no events on this fd, not interested
                continue;
            }

            Conn *conn = fd2conn[i - 1];

            connection_io(conn);

            if (conn->state == STATE_DONE)
            {
                // close the connection
                close(conn->fd);
                free(conn);
                fd2conn[i - 1] = 0;
                memset(&poll_args[i], 0, sizeof(poll_args[i]));
            }
        }

        // try to accept new connections if server socket is ready
        if (poll_args[0].revents)
        {
            accept_new_connection(fd2conn, server_socket);
        }
    }

    return 0;
}
