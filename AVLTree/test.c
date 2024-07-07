#include "AVLTree.c"

// time to test the avl tree
int main() {
    // test init
    AVLNode * tree = avl_init(5.0);
    if (tree->value != 5.0) {
        printf("avl_init failed\n");
    }

    // ll insertions
    tree = avl_insert(tree, 4.0);
    tree = avl_insert(tree, 3.0);
    tree = avl_insert(tree, 2.0);
    tree = avl_insert(tree, 1.0);

    // lr insertions
    tree = avl_insert(tree, 6.0);
    tree = avl_insert(tree, 7.0);

    // rr insertions
    tree = avl_insert(tree, 8.0);
    tree = avl_insert(tree, 9.0);
    tree = avl_insert(tree, 10.0);

    // rl insertions
    tree = avl_insert(tree, 5.5);
    tree = avl_insert(tree, 5.25);
    
    // test search
    if (avl_search(tree, 5.0) == NULL) {
        printf("avl_search failed\n");
    }

    // test delete
    tree = avl_delete(tree, 5.0);
    if (avl_search(tree, 5.0) != NULL) {
        printf("avl_delete failed\n");
    }

    // All tests passed
    printf("All tests passed\n");

    avl_print(tree);    
}
