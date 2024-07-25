#include "ZSet.h"


// configure the compare function for the secondary index, for AVL tree
int compare_scnd_index(void * scnd_index1, void * scnd_index2) {
    if (strcmp((char *) scnd_index1, (char *) scnd_index2) == 0) {
        // check if the strings are equal
        return 0;
    } else {
        return 1;
    }
}

// initialize the ZSet
ZSet * zset_init() {
    ZSet * zset = (ZSet *) calloc(1,sizeof(ZSet));

    zset->hash_table = hcreate(INIT_TABLE_SIZE);
    zset->avl_tree = NULL;

    return zset;
}


// adds/updates a key in the ZSet
int zset_add(ZSet * zset, char * key, float value) {

    HashNode * hash_node = hget(zset->hash_table, key);

    if (hash_node) {
        // if the key already exists, update

        // /deserialize the value from the hash node
        int type = hash_node->valueType;
        if (type != FLOAT) {
            fprintf(stderr, "value in zset is somehow not a float\n");
            return -1;
        }

        float score = *(float *) hash_node->value;
    
        // delete the hash node from the hash table and the AVL tree
        hremove(zset->hash_table, key);
        hfree(hash_node);

        zset->avl_tree = avl_delete(zset->avl_tree, key, score);
    }

    char * key_alloc = strdup(key);
    if (key_alloc == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    float * value_alloc = (float *) calloc(1, sizeof(float));
    if (value_alloc == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    *value_alloc = value;
    
    hash_node = hinit(key_alloc, FLOAT, value_alloc);

    // insert the hash node into the hash table
    hinsert(zset->hash_table, hash_node);

    // insert the hash node into the AVL tree
    zset->avl_tree = avl_insert(zset->avl_tree, key, value);

    return 0;
}

// remove a key from the ZSet
int zset_remove(ZSet * zset, char * key) {
    HashNode * hash_node = hremove(zset->hash_table, key);
    if (!hash_node) {
        fprintf(stderr, "Key does not exist in the ZSet\n");
        return -1;
    }

    // deserialize the value from the hash node
    int type = hash_node->valueType;
    if (type != FLOAT) {
        fprintf(stderr, "value in zset is not a float\n");
        return -1;
    }

    float score = *(float *) hash_node->value;

    // delete the hash node from the hash table and the AVL tree, delete the hash node from the avl_tree first since hash node frees the key
    zset->avl_tree = avl_delete(zset->avl_tree, key, score);
    hfree(hash_node);

    return 0;
}


// search for a value in the ZSet by key
HashNode * zset_search_by_key(ZSet * zset, char * key) {
    HashNode * hash_node = hget(zset->hash_table, key);
    if (!hash_node) {
        return NULL;
    }

    return hash_node;
}


// print the ZSet
void zset_print(ZSet * zset) {
    hprint(zset->hash_table);
    avl_print(zset->avl_tree);
}











