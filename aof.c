#include "aof.h"


// initialize the AOF struct
AOF * aof_init(char * aof_file_name, int flush_interval_sec) {
    AOF * new_aof = (AOF *) calloc(1,sizeof(AOF));
    if (new_aof == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    new_aof->file = fopen(aof_file_name, "a");
    if (new_aof->file == NULL) {
        fprintf(stderr, "Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    // initialize mutex
    if (pthread_mutex_init(&new_aof->mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutex\n");
        exit(EXIT_FAILURE);
    }

    return new_aof;
}

// create a function for flushing file buffer to disk, a worker thread will call this function
void aof_flush(void * aof) {
    AOF * aof_ptr = (AOF *) aof;
    while (1) {
        // sleep for the flush interval
        sleep(aof_ptr->flush_interval_sec);

        // lock the mutex
        pthread_mutex_lock(&aof_ptr->mutex);

        // flush the file buffer
        fflush(aof_ptr->file);
    
        // unlock the mutex
        pthread_mutex_unlock(&aof_ptr->mutex);
    }
}       

// ensure the file is closed and the mutex is destroyed
void aof_close(AOF * aof) {

    // wait for the mutex to be unlocked, to ensure no other thread is using the file
    pthread_mutex_lock(&aof->mutex);

    // close the file
    fclose(aof->file);
    free(aof);

    // destroy the mutex
    pthread_mutex_destroy(&aof->mutex);
}

// write to the file
void aof_write(AOF * aof, char * message) {
    // lock the mutex
    pthread_mutex_lock(&aof->mutex);

    // write to the file, remeber to add a newline character
    fprintf(aof->file, "%s\n", message);

    // unlock the mutex
    pthread_mutex_unlock(&aof->mutex);
}




