#include "Zset.h"


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

    zset->hash_table = hcreate(INITIAL_SIZE);
    zset->avl_tree = NULL;

    return zset;
}

// search for a value in the ZSet by key
HashNode * zset_search_by_key(ZSet * zset, char * key) {
    HashNode * hash_node = hsearch(zset->hash_table, key);
    if (!hash_node) {
        return NULL;
    }

    return hash_node;
}

// key should be dynamically allocated
void zset_add(ZSet * zset, char * key, float value) {
    HashNode * hash_node = hsearch(zset->hash_table, key);
    if (hash_node) {
        // if the key already exists, update

        // /deserialize the value from the hash node
        int type = hash_node->valueType;
        if (type != FLOAT) {
            fprintf(stderr, "value in zset is not a float\n");
            exit(EXIT_FAILURE);
        }

        float score = *(float *) hash_node->value;
    
        // delete the hash node from the hash table and the AVL tree
        hremove(zset->hash_table, key);
        zset->avl_tree = avl_delete(zset->avl_tree, hash_node->key, score);
    }

    // create a new hash node
    hash_node = (HashNode *) calloc(1, sizeof(HashNode));
    if (hash_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    hash_node->key = key;
    hash_node->valueType = FLOAT;
    hash_node->value = (float *) calloc(1, sizeof(float));
    if (hash_node->value == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    *(float *) hash_node->value = value;

    // insert the hash node into the hash table
    hinsert(zset->hash_table, hash_node);

    // insert the hash node into the AVL tree
    zset->avl_tree = avl_insert(zset->avl_tree, key, value);

}

// remove a key from the ZSet
void zset_remove(ZSet * zset, char * key) {
    HashNode * hash_node = hsearch(zset->hash_table, key);
    if (!hash_node) {
        fprintf(stderr, "Key does not exist in the ZSet\n");
        return;
    }

    // deserialize the value from the hash node
    int type = hash_node->valueType;
    if (type != FLOAT) {
        fprintf(stderr, "value in zset is not a float\n");
        exit(EXIT_FAILURE);
    }

    float score = *(float *) hash_node->value;

    // delete the hash node from the hash table and the AVL tree, delete the hash node from the avl_tree first since hash node frees the key
    zset->avl_tree = avl_delete(zset->avl_tree, key, score);
    hremove(zset->hash_table, key);
}












