# include "hashTable.c"

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

    node->key = calloc(sizeof(char), 5);
    node->value = calloc(sizeof(char), 6);

    if (node->key == NULL || node->value == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    strcpy(node->key, "key1");
    strcpy(node->value, "value1");

    node->hashCode = hash(node->key);

    hinsert(table, node);

    HashNode * result = hget(table, "key1");
    if (result == NULL) {
        return 1;
    }

    if (strcmp(result->value, "value1") != 0) {
        fprintf(stderr, "Test 1 (Create and Insert One Node) failed\n");
        return 1;
    }

    // insert another node
    HashNode * node2 = calloc(sizeof(HashNode), 1);
    if (node2 == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    node2->key = calloc(sizeof(char), 5);

    if (node2->key == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    node2->value = calloc(sizeof(char), 7);


    strcpy(node2->key, "key20");
    strcpy(node2->value, "value2");

    node2->hashCode = hash(node2->key);

    hinsert(table, node2);

    result = hget(table, "key20");
    if (result == NULL) {
        return 1;
    }

    if (strcmp(result->value, "value2") != 0) {
        fprintf(stderr, "Test 2 failed (Insert 2 nodes to one slot)\n");
        return 1;
    }    

    HashNode * node3 = calloc(sizeof(HashNode), 1);

    if (node3 == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    node3->key = calloc(sizeof(char), 5);
    node3->value = calloc(sizeof(char), 7);

    if (node3->key == NULL || node3->value == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    strcpy(node3->key, "key2@");
    strcpy(node3->value, "value3");

    // manipulate hash key for testing
    node3->hashCode = hash(node3->key);

    hinsert(table, node3);

    // test remove
    hremove(table, "key20");

    result = hget(table, "key20");
    if (result != NULL) {
        fprintf(stderr, "Test 3 failed (Delete a node from a 2 slot spot)\n");
        return 1;
    }

    // reinsert node2
    node2 = calloc(sizeof(HashNode), 1);
    if (node2 == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    node2->key = calloc(sizeof(char), 5);

    if (node2->key == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    strcpy(node2->key, "key20");

    // store an integer value
    node2->value = calloc(sizeof(int), 1);
    *(int *)node2->value = 43;

    node2->valueType = INTEGER;

    node2->hashCode = hash(node2->key);

    hinsert(table, node2);

    // hprint(table);

    // test resize
    int oldSize = table->size;
    table = hresize(table);

    if (oldSize != table->size || table->mask != 31) {
        fprintf(stderr, "Test 4 (Resizing hashtable) failed\n");
        return 1;
    }

    hprint(table);

    printf("All tests passed\n");

    return 0;
}