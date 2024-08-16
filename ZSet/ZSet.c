// * This file contains the implementation of the ZSet data structure. The ZSet is a collection of key-value pairs, where each key is unique and maps to a float value. The ZSet is implemented using a hash table and an AVL tree. The hash table is used to store the key-value pairs, and the AVL tree is used to store the key-value pairs sorted by the value. The ZSet supports adding, removing, and searching for key-value pairs.

#include "ZSet.h"

/**
 * @brief Compare two secondary indexes
 *
 * This function compares two secondary indexes (expected to be strings) and returns 0 if they are equal, 1 otherwise. Necessary since importing AVL Tree module.
 *
 * @param scnd_index1 The first secondary index
 * @param scnd_index2 The second secondary index
 *
 * @return int 0 if the secondary indexes are equal, 1 otherwise
 */
int compare_scnd_index(void *scnd_index1, void *scnd_index2)
{
    if (strcmp((char *)scnd_index1, (char *)scnd_index2) == 0)
    {
        // check if the strings are equal
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * @brief Initializes a new ZSet
 *
 * This function initializes a new ZSet with an empty hash table and an empty AVL tree.
 *
 * @return ZSet* The initialized ZSet
 */
ZSet *zset_init()
{
    ZSet *zset = (ZSet *)calloc(1, sizeof(ZSet));
    if (!zset)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    zset->hash_table = hcreate(INIT_TABLE_SIZE);
    zset->avl_tree = NULL;

    return zset;
}

// adds/updates a key in the ZSet
/**
 * @brief Add a key to the ZSet
 *
 * This function adds a key to the ZSet with the specified value. If the key already exists, the value is updated.
 *
 * @param zset The ZSet to add the key to
 * @param key The key to add
 * @param value The value to add
 *
 * @return int 0 if successful, -1 if failed
 */
int zset_add(ZSet *zset, char *key, float value)
{

    HashNode *hash_node = hget(zset->hash_table, key);

    if (hash_node)
    {
        // if the key already exists, update

        // /deserialize the value from the hash node
        int type = hash_node->valueType;
        if (type != FLOAT)
        {
            fprintf(stderr, "value in zset is somehow not a float\n");
            return -1;
        }

        float score = *(float *)hash_node->value;

        // delete the hash node from the hash table and the AVL tree
        hremove(zset->hash_table, key);
        hfree(hash_node);

        zset->avl_tree = avl_delete(zset->avl_tree, key, score);
    }

    char *key_alloc = strdup(key);
    if (key_alloc == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    float *value_alloc = (float *)calloc(1, sizeof(float));
    if (value_alloc == NULL)
    {
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

/**
 * @brief Remove a key from the ZSet
 *
 * This function removes a key from the ZSet. If the key does not exist, return -1.
 *
 * @param zset The ZSet to remove the key from
 * @param key The key to remove
 *
 * @return int 0 if successful, -1 if failed
 */
int zset_remove(ZSet *zset, char *key)
{
    HashNode *hash_node = hremove(zset->hash_table, key);
    if (!hash_node)
    {
        fprintf(stderr, "Key does not exist in the ZSet\n");
        return -1;
    }

    // deserialize the value from the hash node
    int type = hash_node->valueType;
    if (type != FLOAT)
    {
        fprintf(stderr, "value in zset is not a float\n");
        return -1;
    }

    float score = *(float *)hash_node->value;

    // delete the hash node from the hash table and the AVL tree, delete the hash node from the avl_tree first since hash node frees the key
    zset->avl_tree = avl_delete(zset->avl_tree, key, score);
    hfree(hash_node);

    return 0;
}

/**
 * @brief Search for a key in the ZSet
 *
 * This function searches for a key in the ZSet and returns the hash node if found, NULL otherwise.
 *
 * @param zset The ZSet to search in
 * @param key The key to search for
 *
 * @return HashNode* The hash node if found, NULL otherwise
 */
HashNode *zset_search_by_key(ZSet *zset, char *key)
{
    HashNode *hash_node = hget(zset->hash_table, key);
    if (!hash_node)
    {
        return NULL;
    }

    return hash_node;
}

/**
 * @brief Free the contents of the ZSet
 *
 * This function frees the contents of the ZSet, but not the ZSet itself.
 *
 * @param zset The ZSet to free
 *
 * @return void
 */
void zset_free_contents(ZSet *zset)
{
    hfree_table(zset->hash_table);
    avl_free(zset->avl_tree);
}

// print the ZSet
void zset_print(ZSet *zset)
{
    hprint(zset->hash_table);
    avl_print(zset->avl_tree);
}
