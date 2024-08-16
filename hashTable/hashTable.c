// This module is different from the other modules, it expects to receive the key and value as they are, and it does not duplicate the key or value. The caller is responsible for freeing the key and value using hfree(), hremove() will NOT free the key and value, it will only remove the node from the hashtable. might change later?

#include "hashTable.h"

/**
 * @brief Hash function
 *
 * This function is the djb2 hash function, which is a simple hash function that is used to hash strings.
 *
 * @param key The key to hash
 *
 * @return int The hash value
 */
int hash(char *key)
{
    int hash = 0;
    for (int i = 0; key[i] != '\0'; i++)
    {
        hash = 31 * hash + key[i];
    }

    return hash;
}

/**
 * @brief Initializes a new hash node
 *
 * This function initializes a new hash node with the specified key, value, and type. Does NOT duplicate the key, the caller is responsible for passing a heap allocated key.
 *
 * @param key The key of the node
 * @param type The type of the value
 * @param value The value of the node
 *
 * @return HashNode* The initialized node
 */
HashNode *hinit(char *key, ValueType type, void *value)
{
    HashNode *node = calloc(sizeof(HashNode), 1);

    if (node == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // does not dupliate the key, caller is responsible for freeing the key
    node->key = key;
    node->valueType = type;
    node->value = value;

    return node;
}

/**
 * @brief Free a single hash node
 *
 * This function frees a single hash node, including the key and value.
 *
 * @param node The node to free
 *
 * @return void
 */
void hfree(HashNode *node)
{
    free(node->key);
    free(node->value);
    free(node);
}

/**
 * @brief Creates a new hashtable
 *
 * This function creates a new hashtable with the specified size. The size must be a power of 2.
 *
 * @param size The size of the hashtable
 *
 * @return HashTable* The created hashtable
 */
HashTable *hcreate(int size)
{
    // check if the size is a power of 2, since we are using a mask to calculate the index (faster than modulo)
    if (size <= 0 || (size & (size - 1)) != 0)
    {
        return 0;
    }

    HashTable *table = calloc(sizeof(HashTable), 1);
    if (table == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    table->nodes = calloc(sizeof(HashNode *), size);
    if (table->nodes == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    table->size = 0;
    table->mask = size - 1;

    return table;
}

/**
 * @brief Inserts a node into the hashtable
 *
 * This function inserts a node into the hashtable. If the key already exists in the hashtable, it returns NULL. If the hashtable is full or the load factor is too high, it resizes the hashtable.
 *
 * @param table The hashtable to insert the node into
 * @param node The node to insert
 *
 * @return HashNode* The inserted node
 */
HashNode *hinsert(HashTable *table, HashNode *node)
{
    // check if the table is full or if load factor is too high
    if ((table->size == table->mask + 1) || (table->size / (table->mask + 1) > table->loadFactor))
    {

        table->size == table->mask + 1 ? printf("Table is full, resizing\n") : printf("Load factor is too high\n");

        // replace old table with new table
        table = hresize(table);
        if (table == NULL)
        {
            fprintf(stderr, "Table resize has failed\n");
            return NULL;
        }
    }

    // calculate the index
    int hashCode = hash(node->key);
    node->hashCode = hashCode;
    int index = hashCode & table->mask;

    // make sure the key is unique
    if (hget(table, node->key) != NULL)
    {
        fprintf(stderr, "Key already exists in the table\n");
        return NULL;
    }

    // insert the node at the head of the linked list
    node->next = table->nodes[index];
    table->nodes[index] = node;

    table->size++;

    // update the load factor
    table->loadFactor = (float)table->size / (table->mask + 1);

    return node;
}

/**
 * @brief Retrieves a node from the hashtable given a key
 *
 * This function retrieves a node from the hashtable given a key. If the key is not found, it returns NULL.
 *
 * @param table The hashtable to search
 * @param key The key of the node to retrieve
 *
 * @return HashNode* The retrieved node
 */
HashNode *hget(HashTable *table, char *key)
{
    // calculate the hash value for the key
    int hashCode = hash(key);

    // calculate the index
    int index = hashCode & table->mask;

    // search for the node in the linked list
    HashNode *traverseList = table->nodes[index];

    while (traverseList != NULL)
    {
        // use lazy evaluation to potentially avoid the strcmp call
        if (traverseList->hashCode == hashCode && strcmp(traverseList->key, key) == 0)
        {
            return traverseList;
        }

        traverseList = traverseList->next;
    }

    return NULL;
}

/**
 * @brief Removes a node from the hashtable given a key
 *
 * This function removes a node from the hashtable given a key. It does not free the key and value, it only removes the node from the hashtable. The caller is responsible for freeing by calling hfree() on the node.
 *
 * @param table The hashtable to remove the node from
 * @param key The key of the node to remove
 *
 * @return HashNode* The removed node
 */
HashNode *hremove(HashTable *table, char *key)
{
    // calculate the hash value for the key
    int hashCode = hash(key);

    // calculate the index
    int index = hashCode & table->mask;

    // search for the node in the linked list
    HashNode *traverseList = table->nodes[index];
    HashNode *prev = NULL;

    while (traverseList != NULL)
    {
        if (traverseList->hashCode == hashCode && strcmp(traverseList->key, key) == 0)
        {
            // first node in the linked list
            if (prev == NULL)
            {
                table->nodes[index] = traverseList->next;
            }
            else
            {
                prev->next = traverseList->next;
            }

            table->size--;

            return traverseList;
        }

        prev = traverseList;
        traverseList = traverseList->next;
    }

    return NULL;
}

/**
 * @brief resizes the hashtable to double the size
 *
 * This function is called when the load factor exceeds a certain threshold, and the hashtable is resized to double the size.
 *
 * @param table The hashtable to free
 *
 * @return HashTable* The resized hashtable
 */
HashTable *hresize(HashTable *table)
{

    int newSize = (table->mask + 1) * 2;
    if (newSize % 2 != 0)
    {
        fprintf(stderr, "new size is not a multiple of 2, not resized\n");
        return table;
    }

    HashTable *newTable = hcreate(newSize);

    // iterate through the old table and reinsert the nodes into the new table
    for (int i = 0; i <= table->mask; i++)
    {
        HashNode *traverseList = table->nodes[i];

        while (traverseList != NULL)
        {
            HashNode *temp = traverseList->next;

            // insert the node into the new table
            hinsert(newTable, traverseList);

            traverseList = temp;
        }
    }

    // free the old nodes
    free(table->nodes);

    // copy the new table into the old table
    table->nodes = newTable->nodes;
    table->loadFactor = newTable->loadFactor;
    table->size = newTable->size;
    table->mask = newTable->mask;

    // free old table
    free(newTable);

    return table;
}

/**
 * @brief Free the hashtable and all its contents
 *
 * @param table The hashtable to free
 *
 * @return void
 */
void hfree_table(HashTable *table)
{
    for (int i = 0; i <= table->mask; i++)
    {
        HashNode *traverseList = table->nodes[i];

        while (traverseList != NULL)
        {
            HashNode *temp = traverseList->next;
            free(traverseList->key);
            free(traverseList->value);
            free(traverseList);
            traverseList = temp;
        }
    }

    free(table->nodes);
    free(table);
}

/**
 * @brief Free the contents of the hashtable, but not the hashtable itself
 *
 * @param table The hashtable to free
 *
 * @return void
 */
void hfree_table_contents(HashTable *table)
{
    for (int i = 0; i <= table->mask; i++)
    {
        HashNode *traverseList = table->nodes[i];

        while (traverseList != NULL)
        {
            HashNode *temp = traverseList->next;
            free(traverseList->key);
            free(traverseList->value);
            free(traverseList);
            traverseList = temp;
        }
    }

    free(table->nodes);
}

// Print the hashtable
void hprint(HashTable *table)
{
    for (int i = 0; i <= table->mask; i++)
    {
        HashNode *traverseList = table->nodes[i];

        printf("--------------------\n");
        printf("Index: %d\n", i);

        while (traverseList != NULL)
        {
            // printf("Key: %s, Value: %s\n", traverseList->key, traverseList->value);
            if (traverseList->valueType == STRING)
            {
                printf("Key: %s, Value: %s\n", traverseList->key, (char *)traverseList->value);
            }
            else if (traverseList->valueType == FLOAT)
            {
                printf("Key: %s, Value: %f\n", traverseList->key, *(float *)traverseList->value);
            }
            else
            {
                fprintf(stderr, "Value type not supported\n");
            }

            traverseList = traverseList->next;
        }

        printf("--------------------\n");
    }
}
