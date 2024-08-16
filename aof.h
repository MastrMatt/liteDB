#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_MESSAGE_SIZE 4096

typedef struct AOF
{
    FILE *file;
    int flush_interval_sec;

    // mutex for file access
    pthread_mutex_t mutex;
} AOF;

// aof functions
AOF *aof_init(char *aof_file_name, int flush_interval_sec, char *mode);
void aof_change_mode(AOF *aof, char *aof_filename, char *mode);
void *aof_flush(void *aof);
void aof_close(AOF *aof);
void aof_write(AOF *aof, char *message);
char *aof_read_line(AOF *aof);