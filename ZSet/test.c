// test the Zset
#include "ZSet.h"

int main()
{
    ZSet *zset = zset_init();

    char *key1 = "key1";
    char *key2 = "key2";
    char *key3 = "key3";
    char *key4 = "key4";
    char *key5 = "key5";
    char *key6 = "key6";
    char *key7 = "key7";

    zset_add(zset, key1, 1.0);
    zset_add(zset, key2, 3.0);
    zset_add(zset, key3, 2.0);
    zset_add(zset, key4, 4.0);
    zset_add(zset, key5, 5.0);
    zset_add(zset, key6, -1.0);
    zset_add(zset, key7, 0.0);

    // search for a key
    HashNode *hash_node = zset_search_by_key(zset, "key1");
    if (!hash_node)
    {
        fprintf(stderr, "key not found\n");
        exit(EXIT_FAILURE);
    }

    // test delete
    zset_remove(zset, "key1");
    if (zset_search_by_key(zset, "key1"))
    {
        fprintf(stderr, "key not deleted\n");
        exit(EXIT_FAILURE);
    }

    // free
    zset_free_contents(zset);
    free(zset);

    // All tests passed
    printf("All tests passed\n");
}
