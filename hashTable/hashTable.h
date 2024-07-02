#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct HashNode {
    char *key;
    char *value;
    struct HashNode * next;
    int hashCode;
} HashNode;


typedef struct {
    // Array of HashNode pointers
    HashNode ** nodes;

    int size;
    int mask;
} HashTable;

// Function prototypes
int hash(char * key);
HashTable * hcreate(int size);

// To insert a node, need to dynamically allocate memory for the node, key, and value. Then, calculate the hash code for the key and store it in the node
HashNode * hinsert(HashTable * table, HashNode * node);
HashNode * hget(HashTable * table, char * key);
HashNode * hremove(HashTable * table, char * key);
HashTable * hresize(HashTable * table);
void hprint(HashTable * table);

