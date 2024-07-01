// create a hashtable to store the client connections to the server,
// use fd as the key and the connection struct as the value

#include "hashTable.h"

// this is the djb2 hash function
int hash(char * key) {
    int hash = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        hash = 31 * hash + key[i];
    }

    return hash;
}


// Creates a hashtable of the given size
HashTable * hcreate(int size) {
    // check if the size is a power of 2, since we are using a mask to calculate the index (faster than modulo)
    if (size <= 0 || (size & (size - 1)) != 0){
        return 0;
    }

    HashTable * table = calloc(sizeof(HashTable), 1);
    if (table == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    table->size = 0;
    table->mask = size - 1;

    return table;
}


// Inserts a node into the hashtable
void hinsert(HashTable * table, HashNode * node) {
    // check if the table is full
    if (table->size == table->mask + 1) {
        fprintf(stderr, "Table is full\n");
        return;
    }

    // calculate the index
    int index = node->hashCode & table->mask;

    // insert the node at the head of the linked list
    node->next = table->nodes[index];
    table->nodes[index] = node;

    table->size++;
}


// Retrieves a node from the hashtable given a key
HashNode * hget(HashTable * table, char * key) {
    // calculate the hash value for the key
    int hashCode = hash(key);

    //calculate the index
    int index = hashCode & table->mask;

    // search for the node in the linked list
    HashNode * traverseList = table->nodes[index];

    while (traverseList != NULL) {
        // use lazy evaluation to potentiallyavoid the strcmp call
        if (traverseList->hashCode == hashCode && strcmp(traverseList->key, key) == 0) {
            return traverseList;
        }

        traverseList = traverseList->next;
    }

    fprintf(stderr, "Key not found\n");
    return NULL;
}
    

// Removes a node from the hashtable given a key
void hremove(HashTable * table, char * key) {
    // calculate the hash value for the key
    int hashCode = hash(key);

    //calculate the index
    int index = hashCode & table->mask;

    // search for the node in the linked list
    HashNode * traverseList = table->nodes[index];

    while (traverseList != NULL) {
        if (traverseList->hashCode == hashCode && strcmp(traverseList->key, key) == 0) {
            // remove the node from the linked list
            HashNode * deleteNode = traverseList;
            traverseList = traverseList->next;

            free(deleteNode->key);
            free(deleteNode->value);
            free(deleteNode);

            table->size--;
            return;
        }
    }

    fprintf(stderr, "Key not found to delete\n");
    return;
}


// Automatically doubles the size of the hashtable
HashTable * hresize(HashTable * table) {

    int newSize = (table->mask + 1) * 2;
    if (newSize % 2 != 0) {
        fprintf(stderr, "new size is not a multiple of 2, not resized\n");
        return table;
    }

    HashTable * newTable = hcreate(newSize);

    // iterate through the old table and reinsert the nodes into the new table
    for (int i = 0; i <= table->mask; i++) {
        HashNode * traverseList = table->nodes[i];

        while (traverseList != NULL) {
            HashNode * temp = traverseList->next;

            // insert the node into the new table
            hinsert(newTable, traverseList);  

            traverseList = temp;
        }
    }

    return table;
 
}

//Print the hashtable
void hprint(HashTable * table) {
    for (int i = 0; i <= table->mask; i++) {
        HashNode * traverseList = table->nodes[i];

        while (traverseList != NULL) {
            printf("Key: %s, Value: %s\n", traverseList->key, traverseList->value);
            traverseList = traverseList->next;
        }
    }
}