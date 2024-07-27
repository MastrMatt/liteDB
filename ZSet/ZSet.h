#include "../hashTable/hashTable.h"
#include "../AVLTree/AVLTree.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// should be multiple of two
#define INIT_TABLE_SIZE 1024


typedef struct {
    HashTable * hash_table;
    AVLNode * avl_tree;
} ZSet;


ZSet * zset_init();
HashNode * zset_search_by_key(ZSet * zset, char * key);
int zset_add(ZSet * zset, char * key, float value);
int zset_remove(ZSet * zset, char * key);
void zset_free_contents(ZSet * zset);
void zset_print(ZSet * zset);

