#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct AVLNode {
    int depth;
    int sub_tree_size;

    struct AVLNode *left;
    struct AVLNode *right;
    struct AVLNode *parent;
    
    // key is a string, used for implementing sorted set later
    float value;
} AVLNode;

AVLNode * avl_init(float value);
AVLNode * avl_insert(AVLNode * tree, float value);
AVLNode * avl_search(AVLNode * tree, float value);
AVLNode * avl_delete(AVLNode * tree, float value);
void avl_free(AVLNode * tree);

void avl_print(AVLNode * tree);


