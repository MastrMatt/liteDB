typedef struct {
    // Array of HashNode pointers
    HashNode ** nodes;
    
    int size;
    int mask;
} HashTable;

typedef struct {
    char * key;
    char * value;
    HashNode * next; 
    // cache the hash value for the key
    int hash;
} HashNode;





