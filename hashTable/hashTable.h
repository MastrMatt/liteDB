#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Define the value type enum
typedef enum { STRING, INTEGER, FLOAT, ZSET} ValueType;

// Define the HashNode structure
typedef struct HashNode {
    char *key;
    
    ValueType valueType;
    // value is a pointer that can be cast to the appropriate type based on the valueType
    void * value;

    struct HashNode *next;
    int hashCode;
} HashNode;

typedef struct {
    // Array of HashNode pointers
    HashNode ** nodes;

    float loadFactor;
    int size;
    int mask;
} HashTable;

// Function prototypes
int hash(char * key);

// size is the initial size of the hash table, must be a power of 2
HashTable * hcreate(int size);

// To insert a node, need to dynamically allocate memory for the node, key, and value. Then, calculate the hash code for the key and store it in the node
HashNode *  hinit(char * key, ValueType type, void * value);
HashNode * hfree(HashNode * node);
HashTable * hcreate(int size);
HashTable * hresize(HashTable * table);
HashNode * hinsert(HashTable * table, HashNode * node);
HashNode * hget(HashTable * table, char * key);
HashNode * hremove(HashTable * table, char * key);
void hprint(HashTable * table);

