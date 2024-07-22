// This is a simple implementation of a hashtable in C, with the ability to store strings and integers by specifying the ValueType. The hash function used is the djb2 hash function. The hashtable automatically resizes when the load factor exceeds a certain threshold. The hprint function is used to print the contents of the hashtable for debugging purposes. Insert hashnodes into the hashtable using hinsert, retrieve them using hget, and remove them using hremove. The hresize function is used to double the size of the hashtable when it is full.

// !Change this module, should not be freeing memory it did not allocate (hremove)

#include "hashTable.h"

// this is the djb2 hash function
int hash(char * key) {
    int hash = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        hash = 31 * hash + key[i];
    }

    return hash;
}

// initialize a single hash node
HashNode *  hinit(char * key, ValueType type, void * value) {
    HashNode * node = calloc(sizeof(HashNode), 1);

    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    node->key = key;
    node->valueType = type;
    node->value = value;    

    return node;
}

// free a single hash node
HashNode * hfree(HashNode * node) {
    free(node->key);
    free(node->value);
    free(node);

    return NULL;
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

    table->nodes = calloc(sizeof(HashNode *), size);
    if (table->nodes == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    table->size = 0;
    table->mask = size - 1;

    return table;
}


// Inserts a node into the hashtable
HashNode * hinsert(HashTable * table, HashNode * node) {
    // check if the table is full or if load factor is too high
    if ((table->size == table->mask + 1) || (table->size / (table->mask + 1) > table->loadFactor)) {
        
        table->size == table -> mask + 1 ? printf("Table is full, resizing\n") : printf("Load factor is too high\n");

        // replace old table with new table
        table = hresize(table);
        if (table == NULL) {
            fprintf(stderr, "Table resize has failed\n");
            return NULL;
        }
    }

    // calculate the index
    int hashCode = hash(node->key);
    node->hashCode = hashCode;
    int index = hashCode & table->mask;

    // make sure the key is unique
    if (hget(table, node->key) != NULL) {
        fprintf(stderr, "Key already exists in the table\n");
        return NULL;
    }

    // insert the node at the head of the linked list
    node->next = table->nodes[index];
    table->nodes[index] = node;

    table->size++;

    // update the load factor
    table->loadFactor = (float) table->size / (table->mask + 1);

    return node;
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
        // use lazy evaluation to potentially avoid the strcmp call
        if (traverseList->hashCode == hashCode && strcmp(traverseList->key, key) == 0) {
            return traverseList;
        }

        traverseList = traverseList->next;
    }

    return NULL;
}
    

// Removes a node from the hashtable given a key
HashNode * hremove(HashTable * table, char * key) {
    // calculate the hash value for the key
    int hashCode = hash(key);

    //calculate the index
    int index = hashCode & table->mask;

    // search for the node in the linked list
    HashNode * traverseList = table->nodes[index];
    HashNode * prev = NULL;

    while (traverseList != NULL) {
        if (traverseList->hashCode == hashCode && strcmp(traverseList->key, key) == 0) {
            // first node in the linked list
            if (prev == NULL) {
                table->nodes[index] = traverseList->next;
            } else {
                prev->next = traverseList->next;
            }

          table->size--;
        

            // return the node that was removed, don't try to acess later will get segfault
            return traverseList;
        }

        prev = traverseList;
        traverseList = traverseList->next;
    }

    return NULL;
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

    // free the old nodes
    free(table->nodes);

    // copy the new table into the old table
    table -> nodes = newTable -> nodes;
    table->loadFactor = newTable->loadFactor;
    table->size = newTable->size;
    table -> mask = newTable -> mask;

    // free old table
    free(newTable);

    return table;
}

//Print the hashtable
void hprint(HashTable * table) {
    for (int i = 0; i <= table->mask; i++) {
        HashNode * traverseList = table->nodes[i];

        printf("--------------------\n");
        printf("Index: %d\n", i);

        while (traverseList != NULL) {
            // printf("Key: %s, Value: %s\n", traverseList->key, traverseList->value);
            if (traverseList->valueType == STRING) {
                printf("Key: %s, Value: %s\n", traverseList->key, (char *)traverseList->value);
            } else if (traverseList->valueType == FLOAT) {
                printf("Key: %s, Value: %f\n", traverseList->key, *(float *)traverseList->value);
            } else {
                fprintf(stderr, "Value type not supported\n");
            }

            traverseList = traverseList->next;
        }

        printf("--------------------\n");

    }
}

