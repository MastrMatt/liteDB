#include "server.h"
#include <pthread.h>

#define AOF_FILE "aof.txt"

typedef struct AOF {
    FILE * file;
    int flush_interval_sec;
    
    // mutex for file access
    pthread_mutex_t mutex;
} AOF;
