// create a hashtable to store the client connections to the server,
// use fd as the key and the connection struct as the value

#include "hashtable.h"

// using a fixed size hashtable for simplicity
HashTable * create_hashtable(int size) {
    // check if the size is a power of 2, since we are using a mask to calculate the index (faster than modulo)
    if (size <= 0 || (size & (size - 1)) != 0){
        return 0;
    }

    HashTable * table = calloc(sizeof(HashTable), 1);
    table->size = 0;
    table->mask = size - 1;

    return table;
}


void insert_hashtable(HashTable * table, HashNode * node) {
    // check if the table is full
    if (table->size == table->mask) {
        return;
    }

    // create a new node
    HashNode * node = calloc(sizeof(HashNode), 1);
    node->key = key;
    node->value = value;

    // calculate the index
    int index = key & table->mask;

    // insert the node at the head of the linked list
    node->next = table->nodes[index];
    table->nodes[index] = node;

    table->size++;
}
