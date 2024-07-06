#include <stdlib.h>

typedef struct AVLNode {
    int depth;
    int sub_tree_size;

    AVLNode *left;
    AVLNode *right;
    AVLNode *parent;
    
    // key is a string, used for implementing sorted set later
    char * key; 
    float value;
} AVLNode;