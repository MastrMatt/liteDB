#include "AVLTree.c"

// in this AVLTree implementation, the secondary index is a string
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

int main()
{
    char *strings[10];

    // dynamically allocate memory for the strings, may cause mem leak if avl function duplicates the string
    for (int i = 0; i < 10; i++)
    {
        strings[i] = (char *)malloc(10 * sizeof(char));
        sprintf(strings[i], "test%d", i);
    }

    // test init
    AVLNode *tree = avl_init(strings[0], 0);

    // test insert
    tree = avl_insert(tree, strings[1], 1);
    tree = avl_insert(tree, strings[2], 2);
    tree = avl_insert(tree, strings[3], 3);
    tree = avl_insert(tree, strings[4], 4);
    tree = avl_insert(tree, strings[5], -1);
    tree = avl_insert(tree, strings[6], -2);
    tree = avl_insert(tree, strings[7], -3);
    tree = avl_insert(tree, strings[8], -3);
    tree = avl_insert(tree, strings[9], -4);

    // test search by pair
    AVLNode *pair1 = avl_search_pair(tree, strings[7], -3);
    AVLNode *pair2 = avl_search_pair(tree, strings[8], -3);

    if (!pair1 || !pair2)
    {
        printf("Search by pair failed, did not fetch a node\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(pair1->scnd_index, strings[7]) != 0 || pair1->value != -3)
    {
        printf("Search by pair failed, fetched wrong node\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(pair2->scnd_index, strings[8]) != 0 || pair2->value != -3)
    {
        printf("Search by pair failed, fetched wrong node\n");
        exit(EXIT_FAILURE);
    }

    // test search
    AVLNode *node = avl_search_float(tree, 3);
    if (!node)
    {
        printf("Search for 3 failed\n");
        exit(EXIT_FAILURE);
    }

    AVLNode *node2 = avl_search_float(tree, -4);
    if (!node2)
    {
        printf("Search for -4 failed\n");
        exit(EXIT_FAILURE);
    }

    // test delete
    tree = avl_delete(tree, node->scnd_index, node->value);

    tree = avl_delete(tree, node2->scnd_index, node2->value);

    if (avl_search_float(tree, 3) || avl_search_float(tree, -4))
    {
        printf("Delete failed\n");
        exit(EXIT_FAILURE);
    }

    // deleting nodes with same score but different secondary index
    AVLNode *node4 = avl_search_pair(tree, strings[7], -3);
    AVLNode *node5 = avl_search_pair(tree, strings[8], -3);

    if (!node4 || !node5)
    {
        printf("Search by pair failed for -3 and -3\n");
        exit(EXIT_FAILURE);
    }

    // test delete
    tree = avl_delete(tree, node4->scnd_index, node4->value);
    tree = avl_delete(tree, node5->scnd_index, node5->value);

    if (avl_search_float(tree, -3))
    {
        printf("Delete failed\n");
        exit(EXIT_FAILURE);
    }

    // time to test offset
    AVLNode *new_tree = avl_init("test", 0);
    new_tree = avl_insert(new_tree, "test", 1);
    new_tree = avl_insert(new_tree, "test", 2);
    new_tree = avl_insert(new_tree, "test", -1);
    new_tree = avl_insert(new_tree, "test", -2);

    AVLNode *offset_node = avl_offset(new_tree, 1);

    if (offset_node->value != 2)
    {
        printf("Offset failed\n");
        exit(EXIT_FAILURE);
    }

    AVLNode *origin_node = avl_search_float(new_tree, 0);
    if (origin_node->value != 0)
    {
        printf("AVL search failed\n");
        exit(EXIT_FAILURE);
    }

    offset_node = avl_offset(origin_node, 1);
    // make sure 1 is the node that was fetched from the offset
    if (offset_node->value != 1)
    {
        printf("Offset failed\n");
        exit(EXIT_FAILURE);
    }

    // passed all tests
    printf("All tests passed\n");

    // test free
    avl_free(tree);
}
