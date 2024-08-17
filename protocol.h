#define MAX_MESSAGE_SIZE 4096
#define MAX_CLIENTS 2047
#define MAX_ARGS 10
#define SERVERPORT 9000

typedef enum
{
    SER_NIL,
    SER_ERR,
    SER_STR,
    SER_INT,
    SER_FLOAT,
    SER_ARR,

} SerialType;
