#include "AVLTree.c"

// in this AVLTree implementation, the secondary index is a string
int compare_scnd_index(void * scnd_index1, void * scnd_index2) {
    if (strcmp((char *) scnd_index1, (char *) scnd_index2) == 0) {
        // check if the strings are equal
        return 0;
    } else {
        return 1;
    }
}


int main() {
    char * test_string = "test";
    // test init
    AVLNode * tree = avl_init(test_string, 0);

    // test insert
    tree = avl_insert(tree, test_string, 1);
    tree = avl_insert(tree, test_string, 2);
    tree = avl_insert(tree, test_string, 3);
    tree = avl_insert(tree, test_string, 4);
    tree = avl_insert(tree, test_string, -1);
    tree = avl_insert(tree, test_string, -2);
    tree = avl_insert(tree, test_string, -3);
    tree = avl_insert(tree, test_string, -4);
    

    // test search
    AVLNode * node = avl_search_float(tree, 3);
    if (!node) {
        printf("Search failed\n");
        exit(EXIT_FAILURE);
    } 

    AVLNode * node2 = avl_search_float(tree, -4);
    if (!node2) {
        printf("Search failed\n");
        exit(EXIT_FAILURE);
    }   

    // test delete
    tree = avl_delete(tree, node->scnd_index, node->value);
    tree = avl_delete(tree, node2->scnd_index, node2->value);
    if (avl_search_float(tree, 3) || avl_search_float(tree, -4)) {
        printf("Delete failed\n");
        exit(EXIT_FAILURE);
    }

    // deleting nodes with same score but different secondary index
    AVLNode * node4 = avl_init("node4", 3);
    AVLNode * node5 = avl_init("node5", 3);

    tree = avl_insert(tree, node5->scnd_index, node5->value);
    tree = avl_insert(tree, node4->scnd_index, node4->value);

    // test delete
    tree = avl_delete(tree, node4->scnd_index, node4->value);

    printf("----------------\n");
    avl_print(tree);
    printf("----------------\n");

    tree = avl_delete(tree, node5->scnd_index, node5->value);

    printf("----------------\n");
    avl_print(tree);
    printf("----------------\n");

    if (avl_search_float(tree, 3)) {
        printf("Delete failed\n");
        exit(EXIT_FAILURE);
    }
    
    // passed all tests
    printf("All tests passed\n");

    // test free
    avl_free(tree);
}
