#include "aof.h"

// initialize the AOF struct
AOF *aof_init(char *aof_file_name, int flush_interval_sec, char *mode)
{
    AOF *new_aof = (AOF *)calloc(1, sizeof(AOF));
    if (new_aof == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    new_aof->file = fopen(aof_file_name, mode);
    if (new_aof->file == NULL)
    {
        fprintf(stderr, "Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    // initialize mutex
    if (pthread_mutex_init(&new_aof->mutex, NULL) != 0)
    {
        fprintf(stderr, "Failed to initialize mutex\n");
        exit(EXIT_FAILURE);
    }

    new_aof->flush_interval_sec = flush_interval_sec;

    return new_aof;
}

// change the mode of the file, this is useful for switching between append and read modes
void aof_change_mode(AOF *aof, char *aof_filename, char *mode)
{
    // lock the mutex
    pthread_mutex_lock(&aof->mutex);

    // close the file, this automatically flushes the buffer
    fclose(aof->file);

    // open the file in the new mode
    aof->file = fopen(aof_filename, mode);
    if (aof->file == NULL)
    {
        fprintf(stderr, "Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    // unlock the mutex
    pthread_mutex_unlock(&aof->mutex);
}

// create a function for flushing file buffer to disk, a worker thread will call this function
void *aof_flush(void *aof)
{
    AOF *aof_ptr = (AOF *)aof;
    while (1)
    {
        // sleep for the flush interval
        sleep(aof_ptr->flush_interval_sec);

        // lock the mutex
        pthread_mutex_lock(&aof_ptr->mutex);

        // flush the file buffer
        fflush(aof_ptr->file);

        // unlock the mutex
        pthread_mutex_unlock(&aof_ptr->mutex);
    }

    return NULL;
}

// ensure the file is closed and the mutex is destroyed
void aof_close(AOF *aof)
{
    // wait for the mutex to be unlocked, to ensure no other thread is using the file
    pthread_mutex_lock(&aof->mutex);

    // close the file resources
    fclose(aof->file);
    pthread_mutex_destroy(&aof->mutex);

    // free aof
    free(aof);
}

// write to the file
void aof_write(AOF *aof, char *message)
{
    pthread_mutex_lock(&aof->mutex);

    // write the message to the file
    if (fprintf(aof->file, "%s", message) < 0)
    {
        fprintf(stderr, "Error writing to file\n");
        exit(EXIT_FAILURE);
    }

    // unlock the mutex
    pthread_mutex_unlock(&aof->mutex);
}

char *aof_read_line(AOF *aof)
{

    char *buffer = calloc(1, MAX_MESSAGE_SIZE);
    if (buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // lock the mutex
    pthread_mutex_lock(&aof->mutex);

    // read a line from the file
    if (fgets(buffer, MAX_MESSAGE_SIZE, aof->file) == NULL)
    {
        // check if the end of file has been reached or error occured
        if (feof(aof->file))
        {
            free(buffer);
            pthread_mutex_unlock(&aof->mutex);
            return NULL;
        }
        else
        {
            perror("Error reading from file");
            exit(EXIT_FAILURE);
        }
    }

    // Replace the newline character with null terminator
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
        buffer[len - 1] = '\0';
    }

    // unlock the mutex
    pthread_mutex_unlock(&aof->mutex);

    return buffer;
}
