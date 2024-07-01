#include "hashTable.h"

// time to test the hashtable
int main() {
    HashTable * table = hcreate(16);
    if (table == NULL) {
        fprintf(stderr, "Failed to create hashtable\n");
        return 1;
    }

    HashNode * node = calloc(sizeof(HashNode), 1);
    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // strdup uses malloc
    node->key = strdup("key");
    node->value = strdup("value");

    if (node->key == NULL || node->value == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    node->hashCode = hash(node->key);

    hinsert(table, node);

    HashNode * result = hget(table, "key");
    if (result == NULL) {
        return 1;
    }

    if (strcmp(result->value, "value") != 0) {
        return 1;
    }

    printf("Test 1 passed\n");

    return 0;
}